//
// Created by Myra Norton on 11/14/25.
//

#include "CompressorProcessor.h"
#include "Synthesiser/Sample.h"
#include "synth_base.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <simd/common.h>

CompressorProcessor::CompressorProcessor (SynthBase& parent, const juce::ValueTree& vt)
    : PluginBase (parent, vt, nullptr, compressorBusLayout())
{
    parent.getValueTree().addListener(this);
    originalSignal.setSize(2, 512);
    sidechainSignal.resize(512, 0.0f);
    rawSidechainSignal = sidechainSignal.data();

    // to catch presses of the reset button
    compressorCallbacks += {state.getParameterListeners().addParameterListener(
        state.params.resetCompressor,
        chowdsp::ParameterListenerThread::AudioThread,
        [this]() {
            if (state.params.resetCompressor.get()->get()) {
                // todo make this a function or just set all parameters here?
                // state.params.loCutFilterParams.resetToDefault();
                state.params.resetCompressor->setParameterValue(false);
            }
        })
    };

    // calculates attack in s and alphaattack when attack changes
    compressorCallbacks += {state.getParameterListeners().addParameterListener(
        state.params.attack,
        chowdsp::ParameterListenerThread::AudioThread,
        [this]() {
            attackTimeInSeconds = *state.params.attack * 0.001f;
            alphaAttack = exp(-1.0 / (sampleRate * attackTimeInSeconds));
        })
    };

    // calculates release in s and alpharelease when release changes
    compressorCallbacks += {state.getParameterListeners().addParameterListener(
        state.params.release,
        chowdsp::ParameterListenerThread::AudioThread,
        [this]() {
            releaseTimeInSeconds = *state.params.release * 0.001f;
            alphaRelease = exp(-1.0 / (sampleRate * releaseTimeInSeconds));
        })
    };

    // sets parameter values when preset changes
    compressorCallbacks += {state.getParameterListeners().addParameterListener(
        state.params.presets,
        chowdsp::ParameterListenerThread::AudioThread,
        [this]() {
            switch (1 << state.params.presets->getIndex())
            {
                case CompressorPresetComboBox::Default:
                {
                    state.params.attack->setParameterValue (state.params.attack->getDefaultValue());
                    state.params.release->setParameterValue (state.params.release->getDefaultValue());
                    state.params.threshold->setParameterValue (state.params.threshold->getDefaultValue());
                    state.params.makeup->setParameterValue (state.params.makeup->getDefaultValue());
                    state.params.ratio->setParameterValue (state.params.ratio->getDefaultValue());
                    state.params.knee->setParameterValue (state.params.knee->getDefaultValue());
                    break;
                }
                case CompressorPresetComboBox::Piano:
                {
                    state.params.attack->setParameterValue (20);
                    state.params.release->setParameterValue (50);
                    state.params.threshold->setParameterValue (-20);
                    state.params.makeup->setParameterValue (0);
                    state.params.ratio->setParameterValue (1.4);
                    state.params.knee->setParameterValue (0.7);
                    break;
                }
                case CompressorPresetComboBox::Piano_2:
                {
                    state.params.attack->setParameterValue (90);
                    state.params.release->setParameterValue (50);
                    state.params.threshold->setParameterValue (-22);
                    state.params.makeup->setParameterValue (0);
                    state.params.ratio->setParameterValue (5);
                    state.params.knee->setParameterValue (0);
                    break;
                }
                case CompressorPresetComboBox::Brick_Wall:
                {
                    state.params.attack->setParameterValue (0);
                    state.params.release->setParameterValue (5);
                    state.params.threshold->setParameterValue (-3);
                    state.params.makeup->setParameterValue (0);
                    state.params.ratio->setParameterValue (23.5);
                    state.params.knee->setParameterValue (0);
                    break;
                }
                case CompressorPresetComboBox::Aggressive:
                {
                    state.params.attack->setParameterValue (0);
                    state.params.release->setParameterValue (5);
                    state.params.threshold->setParameterValue (-10);
                    state.params.makeup->setParameterValue (0);
                    state.params.ratio->setParameterValue (10);
                    state.params.knee->setParameterValue (0);
                    break;
                }
            }

            DBG("Compressor settings are:");
            DBG("  attack       = " << *state.params.attack);
            DBG("  release      = " << *state.params.release);
            DBG("  threshold    = " << *state.params.threshold);
            DBG("  makeup       = " << *state.params.makeup);
            DBG("  ratio        = " << *state.params.ratio);
            DBG("  knee         = " << *state.params.knee);
        })
    };
}

void CompressorProcessor::prepareToPlay (double sampleRate_, int samplesPerBlock)
{
    const auto spec = juce::dsp::ProcessSpec { sampleRate_, (uint32_t) samplesPerBlock, (uint32_t) getMainBusNumInputChannels() };
    sampleRate = sampleRate_;
    DBG("compressor sample rate: " << sampleRate);
    originalSignal.setSize(2, spec.maximumBlockSize);
    sidechainSignal.resize(spec.maximumBlockSize, 0.0f);
    rawSidechainSignal = sidechainSignal.data();
    originalSignal.clear();
}

bool CompressorProcessor::isBusesLayoutSupported (const juce::AudioProcessor::BusesLayout& layouts) const
{
    return true;
}

void CompressorProcessor::processCrestFactor(const float* src, const int numSamples)
{
    //Init accumulators
    if (!peakState) peakState = src[0];
    if (!rmsState) rmsState = src[0];

    //Reset avg attack/release
    avgAttackTime = 0.0;
    avgReleaseTime = 0.0;

    //Calculate averages of auto - attack/release times for a single buffer
    for (int i = 0; i < numSamples; ++i)
    {
        //Square of input signal
        const double s = static_cast<double>(src[i]) * static_cast<double>(src[i]);

        //Update peak state
        peakState = juce::jmax(s, a1 * peakState + b1 * s);

        //Update rms state
        rmsState = a1 * rmsState + b1 * s;

        //calculate squared crest factor
        const double c = peakState / rmsState;
        cFactor = c > 0.0 ? c : 0.0;

        //calculate ballistics
        if (cFactor > 0.0)
        {
            attackTimeInSeconds = 2 * (maxAttackTime / cFactor);
            releaseTimeInSeconds = 2 * (maxReleaseTime / cFactor) - attackTimeInSeconds;

            //Update avg ballistics
            avgAttackTime += attackTimeInSeconds;
            avgReleaseTime += releaseTimeInSeconds;
        }
    }

    // Calculate average ballistics & crest factor
    avgAttackTime /= numSamples;
    avgReleaseTime /= numSamples;
}

float CompressorProcessor::applyCompression(float& input)
{

    overshoot = input - *state.params.threshold;
    if (overshoot <= -kneeHalf)
        return 0.0f;
    if (overshoot > -kneeHalf && overshoot <= kneeHalf)
        return 0.5f * slope * ((overshoot + kneeHalf) * (overshoot + kneeHalf)) / *state.params.knee;

    return slope * overshoot;
}

void CompressorProcessor::applyCompressionToBuffer(float* src, int numSamples)
{
    slope = 1.0f / *state.params.ratio - 1.0f;
    kneeHalf = *state.params.knee / 2.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        const float level = std::max(abs(src[i]), 1e-6f);
        float levelInDecibels = juce::Decibels::gainToDecibels(level);
        src[i] = applyCompression(levelInDecibels);
    }
}

float CompressorProcessor::processPeakBranched(const float& in)
{
    //Smooth branched peak detector
    if (in < state01)
        state01 = alphaAttack * state01 + (1 - alphaAttack) * in;
    else
        state01 = alphaRelease * state01 + (1 - alphaRelease) * in;

    return static_cast<float>(state01); //y_L
}

void CompressorProcessor::applyBallistics(float* src, int numSamples)
{
    // Apply ballistics to src buffer
    for (int i = 0; i < numSamples; ++i)
        src[i] = processPeakBranched(src[i]);
}

void CompressorProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    state.getParameterListeners().callAudioThreadBroadcasters();
    state.params.processStateChanges();
    int numSamples = buffer.getNumSamples();
    int numChannels = buffer.getNumChannels();

    if (*state.params.activeCompressor)
    {
        // Clear any old samples
        originalSignal.clear();
        juce::FloatVectorOperations::fill(rawSidechainSignal, 0.0f, numSamples);
        state.params.maxGainReduction = 0.0f;

        // Apply input gain
        auto inputgainmult = bitklavier::utils::dbToMagnitude (*state.params.inputGain);
        buffer.applyGain(0, 0, numSamples, inputgainmult);
        buffer.applyGain(1, 0, numSamples, inputgainmult);

        // input level meter update stuff
        std::get<0> (state.params.inputLevels) = buffer.getRMSLevel (0, 0, numSamples);
        std::get<1> (state.params.inputLevels) = buffer.getRMSLevel (1, 0, numSamples);

        // Get max l/r amplitude values and fill sidechain signal
        juce::FloatVectorOperations::abs(rawSidechainSignal, buffer.getReadPointer(0), numSamples);
        juce::FloatVectorOperations::max(rawSidechainSignal, rawSidechainSignal, buffer.getReadPointer(1), numSamples);
        // melatonin::printSparkline (buffer);

        // Calculate crest factor on max. amplitude values of input buffer
        processCrestFactor(rawSidechainSignal, numSamples);

        // Compute attenuation - converts side-chain signal from linear to logarithmic domain
        applyCompressionToBuffer(rawSidechainSignal, numSamples);

        // Smooth attenuation - still logarithmic
        applyBallistics(rawSidechainSignal, numSamples);

        // Get minimum = max. gain reduction from side chain buffer
        state.params.maxGainReduction.store(juce::FloatVectorOperations::findMinimum(rawSidechainSignal, numSamples));

        // Calculate auto makeup
        // autoMakeup = calculateAutoMakeup(rawSidechainSignal, numSamples);

        // Add makeup gain and convert side-chain to linear domain
        for (int i = 0; i < numSamples; ++i)
            sidechainSignal[i] = juce::Decibels::decibelsToGain(sidechainSignal[i] + *state.params.makeup /*+ autoMakeup*/);

        // Copy buffer to original signal
        for (int i = 0; i < numChannels; ++i)
            originalSignal.copyFrom(i, 0, buffer, i, 0, numSamples);

        // Multiply attenuation with buffer - apply compression
        for (int i = 0; i < numChannels; ++i)
            juce::FloatVectorOperations::multiply(buffer.getWritePointer(i), rawSidechainSignal, buffer.getNumSamples());

        // // Mix dry & wet signal
        // for (int i = 0; i < numChannels; ++i)
        // {
        //     float* channelData = buffer.getWritePointer(i); //wet signal
        //     FloatVectorOperations::multiply(channelData, mix, numSamples);
        //     FloatVectorOperations::addWithMultiply(channelData, originalSignal.getReadPointer(i), 1 - mix, numSamples);
        // }
    }

    // Update gain reduction metering
    /*
     * todo: i don't think gainReduction actually gets used?
     */
    gainReduction.set(state.params.maxGainReduction.load());

    // handle the send
    // int sendBufferIndex = getChannelIndexInProcessBlockBuffer (false, 2, 0);
    // auto sendgainmult = bitklavier::utils::dbToMagnitude (state.params.outputSend->getCurrentValue());
    // buffer.copyFrom(sendBufferIndex, 0, buffer.getReadPointer(0), numSamples, sendgainmult);
    // buffer.copyFrom(sendBufferIndex+1, 0, buffer.getReadPointer(1), numSamples, sendgainmult);

    // send level meter update
    // std::get<0> (state.params.sendLevels) = buffer.getRMSLevel (sendBufferIndex, 0, numSamples);
    // std::get<1> (state.params.sendLevels) = buffer.getRMSLevel (sendBufferIndex+1, 0, numSamples);

    // final output gain stage, from rightmost slider in DirectParametersView
    auto outputgainmult = bitklavier::utils::dbToMagnitude (*state.params.outputGain);
    buffer.applyGain(0, 0, numSamples, outputgainmult);
    buffer.applyGain(1, 0, numSamples, outputgainmult);

    // main level meter update
    std::get<0> (state.params.outputLevels) = buffer.getRMSLevel (0, 0, numSamples);
    std::get<1> (state.params.outputLevels) = buffer.getRMSLevel (1, 0, numSamples);
}