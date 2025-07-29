//
// Created by Dan Trueman on 7/29/25.
//

#ifndef BITKLAVIER0_BLENDRONICDELAY_H
#define BITKLAVIER0_BLENDRONICDELAY_H

#pragma once
#include <juce_dsp/juce_dsp.h>

/*
////////////////////////////////////////////////////////////////////////////////
   bitKlavier replacement for STK DelayL - limited implementation
////////////////////////////////////////////////////////////////////////////////
*/

class BKDelayL
{
public:
    //constructors
    BKDelayL();
    BKDelayL(float delayLength, int bufferSize, float delayGain, double sr);
    ~BKDelayL();

    //accessors
    inline const float getLength() const noexcept { return length; }
    inline const float getBufferSize() const noexcept { return bufferSize; }
    inline const float getGain() const noexcept { return gain; }
    inline const float lastOutLeft() const noexcept { return lastFrameLeft; }
    inline const float lastOutRight() const noexcept { return lastFrameRight; }

    inline const float getSample(int c, int i) const noexcept
    {
        if (loading) return 0.0f;
        return inputs.getSample(c, i);
    }

    inline const juce::AudioBuffer<float>* getBuffer() const noexcept { return &inputs; }

    //mutators
    void setLength(float delayLength);
    void setBufferSize(int size);
    inline void setGain(float delayGain) { gain = delayGain; }
    inline void setFeedback(float fb) { feedback = fb; }
    inline int getInPoint() { return inPoint; }
    inline int getOutPoint() { return outPoint; }

    float nextOutLeft();
    float nextOutRight();
    void addSample(float input, int offset, int channel);
//    void addSamples(float* input, int numSamples, int offset, int channel);
//    void tick(float input, float* outputs, float outGain, bool stereo = true);
    void tick(float* inL, float* inR);
    void clear();
    void reset();

    inline void setSampleRate(double sr) { sampleRate = sr; }

    juce::AudioBuffer<float> inputs;

private:
    juce::CriticalSection lock;

    int inPoint;
    int outPoint;
    int bufferSize;
    float length;
    float gain;
    float lastFrameLeft;
    float lastFrameRight;
    float alpha;
    float omAlpha;
    float feedback;
    float nextOutput;
    bool doNextOutLeft;
    bool doNextOutRight;

    bool loading;
    double sampleRate;
};

/*
////////////////////////////////////////////////////////////////////////////////
   bitKlavier replacement for STK envelope - limited implementation
////////////////////////////////////////////////////////////////////////////////
*/

class BKEnvelope
{
public:
    //constructors
    BKEnvelope();
    BKEnvelope(float bValue, float bDuration, double sr);
    ~BKEnvelope();

    //accessors
    inline const float getValue() const noexcept { return value; }
    inline const float getTarget() const noexcept { return target; }
    inline const int getState() const noexcept { return state; }

    //mutators
    inline void setValue(float envelopeValue) { value = envelopeValue; }
    inline void setTarget(float envelopeTarget) { target = envelopeTarget; state = (target != value) ? 1 : 0; }
    inline void setRate(float sr) { rate = sr; }
    inline void setTime(float time) { rate = 1.0 / ( time * sampleRate * 0.001 ); }//DBG("new rate = " + String(rate));} // time in ms for envelope to go from 0-1. need to update for sampleRate
    inline void setSampleRate(double sr) { sampleRate = sr; }

    //! Set target = 1.
    void keyOn( void ) { this->setTarget( 1.0 ); };

    //! Set target = 0.
    void keyOff( void ) { this->setTarget( 0.0 ); };

    float tick();
    float lastOut() { return lastvalue; }

private:
    float value;
    float lastvalue;
    float target;
    float rate;
    int state;

    double sampleRate;
};

class BlendronicDelay
{
public:
//    BlendronicDelay(std::unique_ptr<BlendronicDelay> d);
    BlendronicDelay(float delayLength, float smoothValue, float smoothDuration, int delayBufferSize, double sr);
    ~BlendronicDelay();

    //accessors
//    inline const std::unique_ptr<BKDelayL> getDelay() const noexcept { return delayLinear; }
//    inline const std::unique_ptr<BKEnvelope> getDSmooth() const noexcept { return dSmooth; }
//    inline const std::unique_ptr<BKEnvelope> getEnvelope() const noexcept { return dEnv; }
    inline const float getBufferSize() const noexcept { return dBufferSize; }
    inline const float getDelayGain() const noexcept { return dDelayGain; }
    inline const float getDelayLength() const noexcept { return dDelayLength; }
    inline const float getSmoothValue() const noexcept { return dSmooth->getValue(); }
    inline const float getSmoothRate() const noexcept { return dSmoothRate; }
//    inline const bool getInputState() const noexcept { return dInputOpen; }
//    inline const bool getOutputState() const noexcept { return dOutputOpen; }
    inline const bool getShouldDuck() const noexcept { return shouldDuck; }
    inline const juce::AudioBuffer<float>* getDelayBuffer() const noexcept { return delayLinear->getBuffer(); }
    inline const int getInPoint() const noexcept { return delayLinear->getInPoint(); }
    inline const int getOutPoint() const noexcept { return delayLinear->getOutPoint(); }
    inline const float getSample(int c, int i) const noexcept { return delayLinear->getSample(c, i); }

    //mutators
//    void addSample(float sampleToAdd, int offset, int channel); //adds input sample into the delay line (first converted to float)
//    void addSamples(float* samplesToAdd, int numSamples, int offset, int channel);

    inline void setBufferSize(int bufferSize)
    {
        dBufferSize = bufferSize;
        delayLinear->setBufferSize(dBufferSize);
    }
    inline void setDelayGain(float delayGain) { dDelayGain = delayGain; }
    inline void setDelayLength(float delayLength) { dDelayLength = delayLength; delayLinear->setLength(delayLength); }
    inline void setDelayTargetLength(float delayLength) { dSmooth->setTarget(delayLength); }
    inline void setSmoothValue(float smoothValue)
    {
        dSmoothValue = smoothValue;
        dSmooth->setValue(dSmoothValue);
    }
    inline void setEnvelopeTarget(float t) { dEnv->setTarget(t); }

    //we want to be able to do this two ways:
    //set a duration for the delay length changes that will be constant, so at the beginning of
    //  each beat we will need to calculate a new rate dependent on this duration and the beat length (rate ~ beatLength / duration)
    //have the rate be constant, regardless of beat length, so we'll use the length of smallest beat (1, as set by Tempo, so the pulseLength)
    //  so rate ~ pulseLength / duration
    inline void setSmoothRate(float smoothRate)
    {
        dSmoothRate = smoothRate;
        dSmooth->setRate(smoothRate);
    }

    inline void setFeedback(float fb) { delayLinear->setFeedback(fb); }
//    inline const void setInputState(bool inputState) { dInputOpen = inputState; }
//    inline const void toggleInput() { dInputOpen = !dInputOpen; }
//    inline const void setOutputState(bool outputState) { dOutputOpen = outputState; }
//    inline const void toggleOutput() { dOutputOpen = !dOutputOpen; }

    void tick(float* outputs, float outGain);
    void tick(float* inL, float* inR);

//    void duckAndClear();

    inline void setSampleRate(double sr) { sampleRate = sr; delayLinear->setSampleRate(sr); dSmooth->setSampleRate(sr); }
    inline double getSampleRate() { return sampleRate; }
    inline void clear() { delayLinear->clear(); /*delayLinear->reset();*/ }

private:
    std::unique_ptr<BKDelayL> delayLinear;
    std::unique_ptr<BKEnvelope> dSmooth;
    std::unique_ptr<BKEnvelope> dEnv;
    float dBufferSize;
    float dDelayGain;
    float dDelayLength;
    float dSmoothValue;
    float dSmoothRate;
//    bool dInputOpen;
//    bool dOutputOpen;
    bool shouldDuck;

    double sampleRate;
};

#endif //BITKLAVIER0_BLENDRONICDELAY_H
