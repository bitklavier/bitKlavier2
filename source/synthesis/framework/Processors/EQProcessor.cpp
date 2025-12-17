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
                    // todo make this a function or just set all parameters here?
                    // resetToDefault() doesn't do anything about activeFilters
                    state.params.loCutFilterParams.resetToDefault();
                    state.params.peak1FilterParams.resetToDefault();
                    state.params.peak2FilterParams.resetToDefault();
                    state.params.peak3FilterParams.resetToDefault();
                    state.params.hiCutFilterParams.resetToDefault();
                    state.params.resetEq->setParameterValue(false);
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
}