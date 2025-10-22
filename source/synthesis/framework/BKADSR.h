//
// Created by Dan Trueman on 11/7/24.
//

#ifndef BITKLAVIER2_BKADSR_H
#define BITKLAVIER2_BKADSR_H

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_core/juce_core.h>
#include <juce_audio_formats/juce_audio_formats.h>

//==============================================================================
/**
Based on the JUCE ADSR Class, with curvature additions

 performance should be similar to JUCE::ADSR for curvature values close to 0
 otherwise, will have some performance impact

 might want to do some performance tests to see if the lookup table version of powerScale
 is actually worth it, given that it is at least possible that calculating powerScale
 directly might sound better for long release tails and so on.

 better: have a callback here that is called whenever the adsr values are updated, so the
 tables can be calculated then rather than on noteOn

**/

#pragma once

class BKADSR
{
public:
    //==============================================================================
    BKADSR()
    {
        recalculateRates();
        recalculateCurveTables(); // optimization
    }

    //==============================================================================
    /**
        Holds the parameters being used by an ADSR object.

        @tags{Audio}
    */
    struct Parameters
    {
        Parameters() = default;

        Parameters (float attackTimeSeconds,
            float decayTimeSeconds,
            float sustainLevel,
            float releaseTimeSeconds)
            : attack (attackTimeSeconds),
              decay (decayTimeSeconds),
              sustain (sustainLevel),
              release (releaseTimeSeconds)
        {
        }

        Parameters (float attackTimeSeconds,
            float decayTimeSeconds,
            float sustainLevel,
            float releaseTimeSeconds,
            float attackPower,
            float decayPower,
            float releasePower)
            : attack (attackTimeSeconds),
              decay (decayTimeSeconds),
              sustain (sustainLevel),
              release (releaseTimeSeconds),
              attackPower (attackPower),
              decayPower (decayPower),
              releasePower (releasePower)
        {
        }

        float attack = 0.1f, decay = 0.1f, sustain = 1.0f, release = 0.1f;
        float attackPower = 0.0f, decayPower = 0.0f, releasePower = 0.0f;

    };

    /** Sets the parameters that will be used by an ADSR object.

        You must have called setSampleRate() with the correct sample rate before
        this otherwise the values may be incorrect!

        @see getParameters
    */
    void setParameters (const Parameters& newParameters)
    {
        // need to call setSampleRate() first!
        jassert (sampleRate > 0.0);

        parameters = newParameters;
        recalculateRates();
        recalculateCurveTables(); //would happen here, if we optimize
        // gets called every noteOn, so tables shouldn't be too big... 256?
    }

    /** Returns the parameters currently being used by an ADSR object.

        @see setParameters
    */
    const Parameters& getParameters() const noexcept  { return parameters; }

    /** Returns true if the envelope is in its attack, decay, sustain or release stage. */
    bool isActive() const noexcept                    { return state != State::idle; }

    //==============================================================================
    /** Sets the sample rate that will be used for the envelope.

        This must be called before the getNextSample() or setParameters() methods.
    */
    void setSampleRate (double newSampleRate) noexcept
    {
        jassert (newSampleRate > 0.0);
        sampleRate = newSampleRate;
    }

    //==============================================================================
    /** Resets the envelope to an idle state. */
    void reset() noexcept
    {
        envelopeVal = 0.0f;
        state = State::idle;
    }

    /** Starts the attack phase of the envelope. */
    void noteOn() noexcept
    {
        if (attackRate > 0.0f)
        {
            state = State::attack;
        }
        else if (decayRate > 0.0f)
        {
            envelopeVal = 1.0f;
            state = State::decay;
        }
        else
        {
            envelopeVal = parameters.sustain;
            state = State::sustain;
        }
    }

    /** Starts the release phase of the envelope. */
    void noteOff() noexcept
    {
        if (state != State::idle)
        {
            if (parameters.release > 0.0f)
            {
                if (state != State::release) envelopeVal = lastPreReleaseEnvelopeVal;
                releaseRate = (float) (envelopeVal / (parameters.release * sampleRate));
                state = State::release;
            }
            else
            {
                reset();
            }
        }
    }

    //==============================================================================
    /** Returns the next sample value for an ADSR object.
        @see applyEnvelopeToBuffer
    */
    float getNextSample() noexcept
    {
        switch (state)
        {
            case State::idle:
            {
                return 0.0f;
            }

            case State::attack:
            {
                envelopeVal += attackRate;

                if (envelopeVal >= 1.0f)
                {
                    envelopeVal = 1.0f;
                    goToNextState();
                }

//                // unoptimized, calling powerScale directly
//                if (parameters.attackPower != 0.0f)
//                    return lastPreReleaseEnvelopeVal = powerScale(envelopeVal, parameters.attackPower);
//                return lastPreReleaseEnvelopeVal = envelopeVal;

                // optimized, using lookup table
                if (parameters.attackPower != 0.0f)
                    return lastPreReleaseEnvelopeVal = powerScale(envelopeVal, attackCurve);
                return lastPreReleaseEnvelopeVal = envelopeVal;
            }

            case State::decay:
            {
                envelopeVal -= decayRate;

                if (envelopeVal <= parameters.sustain)
                {
                    envelopeVal = parameters.sustain;
                    goToNextState();
                }

                if (parameters.decayPower != 0.0f)
                {
                    if (parameters.sustain < 1.0f)
                    {
//                        // unoptimized
//                        return lastPreReleaseEnvelopeVal = powerScale ((envelopeVal - parameters.sustain) / (1.0f - parameters.sustain), parameters.decayPower)
//                                      * (1.0f - parameters.sustain)
//                                  + parameters.sustain;

//                        // optimized, using lookup table
//                        return lastPreReleaseEnvelopeVal = powerScale((envelopeVal - parameters.sustain) / (1.0f - parameters.sustain), decayCurve)
//                                   * (1.0f - parameters.sustain)
//                               + parameters.sustain;

                        // further optimized
                        return lastPreReleaseEnvelopeVal = powerScale((envelopeVal - parameters.sustain) * oneOverOneMinusSustain, decayCurve)
                                   * oneMinusSustain
                               + parameters.sustain;
                    }
                    return lastPreReleaseEnvelopeVal = 1.0f;
                }
                return lastPreReleaseEnvelopeVal = envelopeVal;
            }

            case State::sustain:
            {
                envelopeVal = parameters.sustain;
                return lastPreReleaseEnvelopeVal = envelopeVal;
            }

            case State::release:
            {
                envelopeVal -= releaseRate;

                if (envelopeVal <= 0.0f)
                {
                    goToNextState();
                }

//                // unoptimized
//                if (parameters.releasePower != 0.0f)
//                    //return powerScale(envelopeVal / parameters.sustain, parameters.releasePower) * envelopeVal;
//                    return powerScale(envelopeVal / lastPreReleaseEnvelopeVal, parameters.releasePower) * envelopeVal;
//                return envelopeVal;

                // optimized, using lookup table
                if (parameters.releasePower != 0.0f)
                    return powerScale(envelopeVal / lastPreReleaseEnvelopeVal, releaseCurve) * envelopeVal;
                    //return powerScale(envelopeVal / parameters.sustain, releaseCurve) * envelopeVal;
                return envelopeVal;

//                // further optimized; don't use right now until fixing oneOverSustain to include lastPreReleaseEnvelopeVal
//                if (parameters.releasePower != 0.0f)
//                    return powerScale(envelopeVal * oneOverSustain, releaseCurve) * envelopeVal;
//                return envelopeVal;

            }
        }

        /**
         * possible factors to pull out for optimization:
         *
         *      1/parameters.sustain, so we can multiple instead of divide in State::release
         *      1.0f - parameters.sustain, for State::decay, just to remove the subtraction
         *      1 / (1.0f - parameters.sustain), for State::decay, to multiple instead of divide
         *
         */

        return 0.0f;
    }

    /** This method will conveniently apply the next numSamples number of envelope values
        to an AudioBuffer.
        @see getNextSample
    */
    template <typename FloatType>
    void applyEnvelopeToBuffer (juce::AudioBuffer<FloatType>& buffer, int startSample, int numSamples)
    {
        jassert (startSample + numSamples <= buffer.getNumSamples());

        if (state == State::idle)
        {
            buffer.clear (startSample, numSamples);
            return;
        }

        if (state == State::sustain)
        {
            buffer.applyGain (startSample, numSamples, parameters.sustain);
            return;
        }

        auto numChannels = buffer.getNumChannels();

        while (--numSamples >= 0)
        {
            auto env = getNextSample();

            for (int i = 0; i < numChannels; ++i)
                buffer.getWritePointer (i)[startSample] *= env;

            ++startSample;
        }
    }

private:
    //==============================================================================
    void recalculateRates() noexcept
    {
        auto getRate = [] (float distance, float timeInSeconds, double sr)
        {
            return timeInSeconds > 0.0f ? (float) (distance / (timeInSeconds * sr)) : -1.0f;
        };

        attackRate  = getRate (1.0f, parameters.attack, sampleRate);
        decayRate   = getRate (1.0f - parameters.sustain, parameters.decay, sampleRate);
        releaseRate = getRate (parameters.sustain, parameters.release, sampleRate);

        if (   (state == State::attack && attackRate <= 0.0f)
            || (state == State::decay && (decayRate <= 0.0f || envelopeVal <= parameters.sustain))
            || (state == State::release && releaseRate <= 0.0f))
        {
            goToNextState();
        }

        // optimizations
        oneMinusSustain = 1.0f - parameters.sustain;

        if (parameters.sustain != 0.)
            oneOverSustain = 1.0f / parameters.sustain;
        else oneOverSustain = 10000.;

        if (oneMinusSustain != 0.)
            oneOverOneMinusSustain = 1.0f / oneMinusSustain;
        else oneOverOneMinusSustain = 10000.;
    }

    void goToNextState() noexcept
    {
        if (state == State::attack)
        {
            state = (decayRate > 0.0f ? State::decay : State::sustain);
            return;
        }

        if (state == State::decay)
        {
            state = State::sustain;
            return;
        }

        if (state == State::release)
        {
            reset();
        }
    }

    /**
     * powerScale, from Vital/Tytel
     * for creating a curved table of values between 0 and 1
     *
     * @param value = [0, 1] linear input
     * @param power = exponent coefficient [0 => linear, generally between +/- 2-10 for useful curves]
     * @return output = [0, 1] curved output
     *
     * could replace with lookup table to see if that is better for performance
     *
     * k_curvature (power) is smoothed to afford click-less transitions from one curvature to another
     * passes input through when power is near zero, minimizing performance hit
     *
     */
    force_inline float powerScale(float value, float power) {

        if (fabsf(power) <= kMinPower) return value;

        float numerator = exp(power * value) - 1.0f;
        float denominator = exp(power) - 1.0f;
        return numerator / denominator;
    }

    /**
     * optimized version of powerScale, using simple lookup table and linear interpolation
     *
     * this should be significantly less expensive than the powerScale above,
     *      especially with such a small lookup table that can be cached
     *
     * @param value = [0, 1] linear input
     * @param power = exponent coefficient [0 => linear, generally between +/- 2-10 for useful curves]
     * @return output = [0, 1] curved output
     */
    force_inline float powerScale(float value, const std::vector<float>& table)
    {
        // ** just truncating here...
        //if(value > 1.0) value = 1.0f;
        //else if(value < 0.) value = 0.f;
        //return table[value * (curveTableSize - 1)]; // just return closest value, probably good enough if table is large enough

        // ** using simple linear interpolation
        // table positions
        float currentSamplePos = static_cast<float>(curveTableSize - 1) * value;
        int pos = static_cast<int>(currentSamplePos);
        if (pos < 0) pos = 0;
        int nextPos = pos + 1;
        if (pos >= curveTableSize) pos = curveTableSize - 1;
        if (nextPos >= curveTableSize) nextPos = curveTableSize - 1;

        // coefficients
        float alpha = currentSamplePos - static_cast<float>(pos);
        float invAlpha = 1.0f - alpha;

        // interpolate
        return table[pos] * invAlpha + table[nextPos] * alpha;
    }

    /**
     * pre-calculates powerScale lookup table
     *
     * @param power
     * @param table
     */
    void createTable(float power, std::vector<float>& table)
    {
        table.clear();
        for (int i=0; i<curveTableSize; i++)
            table.push_back(powerScale(static_cast<float>(i) / curveTableSize, power));
    }

    /**
     * pre-calculate the three tables needed here
     */
    void recalculateCurveTables()
    {
        createTable(parameters.attackPower, attackCurve);
        createTable(parameters.decayPower, decayCurve);
        createTable(parameters.releasePower, releaseCurve);
    }

    //==============================================================================
    enum class State { idle, attack, decay, sustain, release };

    State state = State::idle;
    Parameters parameters;

    double sampleRate = 44100.0;
    float envelopeVal = 0.0f;
    float lastPreReleaseEnvelopeVal = 0.0f;
    float attackRate = 0.0f, decayRate = 0.0f, releaseRate = 0.0f;

    static constexpr float kMinPower = 0.01f; // ignore k values less than this
    std::vector<float> attackCurve, decayCurve, releaseCurve; // for lookup tables
    static constexpr int curveTableSize = 256; // for lookup tables

    // precalculate optimizations
    float oneOverSustain, oneMinusSustain, oneOverOneMinusSustain;


};

#endif //BITKLAVIER2_BKADSR_H