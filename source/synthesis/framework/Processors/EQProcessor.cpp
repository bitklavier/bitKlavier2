//
// Created by Myra Norton on 11/14/25.
//

#include "EQProcessor.h"
#include "Synthesiser/Sample.h"
#include "common.h"
#include "synth_base.h"

EQProcessor::EQProcessor (SynthBase& parent, const juce::ValueTree& vt)
    : PluginBase (parent, vt, nullptr, eqBusLayout())
{
    // parent.getValueTree().addListener(this);
    // state.params.sampleRate = getSampleRate();
    // recalculate coefficients
    state.params.doForAllParameters ([this] (auto& param, size_t) {
        eqCallbacks += {state.getParameterListeners().addParameterListener(
            param,
            chowdsp::ParameterListenerThread::AudioThread,
            [this]() {
                    this->state.params.updateCoefficients();
            })
        };
    });
     // to catch presses of the reset button
        eqCallbacks += {state.getParameterListeners().addParameterListener(
            state.params.resetEq,
            chowdsp::ParameterListenerThread::AudioThread,
            [this]() {
                if (state.params.resetEq.get()->get()) {
                    resetToDefaults();
                }
            })
        };

        eqCallbacks += {state.getParameterListeners().addParameterListener(
        state.params.presets,
            chowdsp::ParameterListenerThread::AudioThread,
            [this]()
            {
                switch (1 << state.params.presets->getIndex())
                {
                    case EqPresetComboBox::EqOff:
                    {
                        resetToDefaults();
                        break;
                    }
                    case EqPresetComboBox::Lowshelf:
                    {
                        resetToDefaults();
                        state.params.activeEq->setParameterValue(true);
                        state.params.peak1FilterParams.filterFreq->setParameterValue(30);
                        state.params.peak1FilterParams.filterGain->setParameterValue(-12);
                        state.params.peak1FilterParams.filterQ->setParameterValue(0.1);
                        state.params.peak1FilterParams.filterActive->setParameterValue(true);
                        break;
                    }
                    case EqPresetComboBox::Highshelf:
                    {
                        resetToDefaults();
                        state.params.activeEq->setParameterValue(true);
                        state.params.peak3FilterParams.filterFreq->setParameterValue(12000);
                        state.params.peak3FilterParams.filterGain->setParameterValue(12);
                        state.params.peak3FilterParams.filterQ->setParameterValue(0.1);
                        state.params.peak3FilterParams.filterActive->setParameterValue(true);
                        break;
                    }
            }
        })
    };

        // to catch presses of the eq power button
        eqCallbacks += {state.getParameterListeners().addParameterListener(
            state.params.activeEq,
            chowdsp::ParameterListenerThread::AudioThread,
            [this]() {
                // if EQ is off, filters should be off but save which filters were on
                if (!state.params.activeEq.get()->get()) {
                    // todo: is this threadsafe?
                    state.params.activeFilters = {
                        state.params.loCutFilterParams.filterActive->get(),
                        state.params.peak1FilterParams.filterActive->get(),
                        state.params.peak2FilterParams.filterActive->get(),
                        state.params.peak3FilterParams.filterActive->get(),
                        state.params.hiCutFilterParams.filterActive->get()
                    };

                    state.params.loCutFilterParams.filterActive->setParameterValue(false);
                    state.params.peak1FilterParams.filterActive->setParameterValue(false);
                    state.params.peak2FilterParams.filterActive->setParameterValue(false);
                    state.params.peak3FilterParams.filterActive->setParameterValue(false);
                    state.params.hiCutFilterParams.filterActive->setParameterValue(false);
                }
                // if EQ is on, turn on the filters that were on previously
                if (state.params.activeEq.get()->get()) {
                    // todo: is this threadsafe? i shouldn't hardcode the numbers
                    if (state.params.activeFilters.getUnchecked (0)) state.params.loCutFilterParams.filterActive->setParameterValue(true);
                    if (state.params.activeFilters.getUnchecked (1)) state.params.peak1FilterParams.filterActive->setParameterValue(true);
                    if (state.params.activeFilters.getUnchecked (2)) state.params.peak2FilterParams.filterActive->setParameterValue(true);
                    if (state.params.activeFilters.getUnchecked (3)) state.params.peak3FilterParams.filterActive->setParameterValue(true);
                    if (state.params.activeFilters.getUnchecked (4)) state.params.hiCutFilterParams.filterActive->setParameterValue(true);
                }
            })
        };
}

void EQProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const auto spec = juce::dsp::ProcessSpec { sampleRate, (uint32_t) samplesPerBlock, (uint32_t) getMainBusNumInputChannels() };
    state.params.leftChain.prepare(spec);
    state.params.rightChain.prepare(spec);
    state.params.updateCoefficients();
}

bool EQProcessor::isBusesLayoutSupported (const juce::AudioProcessor::BusesLayout& layouts) const
{
    return true;
}

void EQProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    state.getParameterListeners().callAudioThreadBroadcasters();
    state.params.processStateChanges();

    int numSamples = buffer.getNumSamples();

    // apply the input gain multiplier
    auto inputgainmult = bitklavier::utils::dbToMagnitude (state.params.inputGain->getCurrentValue());
    buffer.applyGain(0, 0, numSamples, inputgainmult);
    buffer.applyGain(1, 0, numSamples, inputgainmult);

    // input level meter update stuff
    std::get<0> (state.params.inputLevels) = buffer.getRMSLevel (0, 0, numSamples);
    std::get<1> (state.params.inputLevels) = buffer.getRMSLevel (1, 0, numSamples);

    juce::dsp::AudioBlock<float> block(buffer);
    auto leftBlock = block.getSingleChannelBlock(0);
    juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
    state.params.leftChain.process(leftContext);
    if (buffer.getNumChannels() > 1)
    {
        auto rightBlock = block.getSingleChannelBlock(1);
        juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);
        state.params.rightChain.process(rightContext);
    }

    // handle the send
    // int sendBufferIndex = getChannelIndexInProcessBlockBuffer (false, 2, 0);
    // auto sendgainmult = bitklavier::utils::dbToMagnitude (state.params.outputSend->getCurrentValue());
    // buffer.copyFrom(sendBufferIndex, 0, buffer.getReadPointer(0), numSamples, sendgainmult);
    // buffer.copyFrom(sendBufferIndex+1, 0, buffer.getReadPointer(1), numSamples, sendgainmult);

    // send level meter update
    // std::get<0> (state.params.sendLevels) = buffer.getRMSLevel (sendBufferIndex, 0, numSamples);
    // std::get<1> (state.params.sendLevels) = buffer.getRMSLevel (sendBufferIndex+1, 0, numSamples);

    // final output gain stage, from rightmost slider in DirectParametersView
    auto outputgainmult = bitklavier::utils::dbToMagnitude (state.params.outputGain->getCurrentValue());
    buffer.applyGain(0, 0, numSamples, outputgainmult);
    buffer.applyGain(1, 0, numSamples, outputgainmult);

    // main level meter update
    std::get<0> (state.params.outputLevels) = buffer.getRMSLevel (0, 0, numSamples);
    std::get<1> (state.params.outputLevels) = buffer.getRMSLevel (1, 0, numSamples);
}