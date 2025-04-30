//
// Created by Joshua Warner on 6/27/24.
//

#ifndef BITKLAVIER2_TUNINGPROCESSOR_H
#define BITKLAVIER2_TUNINGPROCESSOR_H

#pragma once

#include <chowdsp_plugin_base/chowdsp_plugin_base.h>
#include <chowdsp_plugin_utils/chowdsp_plugin_utils.h>
#include <chowdsp_sources/chowdsp_sources.h>
#include <chowdsp_plugin_state/chowdsp_plugin_state.h>

#include "PluginBase.h"
// TODO change params

struct TuningParams : chowdsp::ParamHolder
{

    // Adds the appropriate parameters to the Tuning Processor
    TuningParams() : chowdsp::ParamHolder("tuning")
    {
    }

};

struct TuningNonParameterState : chowdsp::NonParamState
{
    TuningNonParameterState()
    {
    }

};

class TuningProcessor : public bitklavier::PluginBase<chowdsp::PluginStateImpl<TuningParams,TuningNonParameterState,chowdsp::XMLSerializer>>
{
public:
    TuningProcessor(SynthBase* parent,const juce::ValueTree& v);

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processAudioBlock (juce::AudioBuffer<float>& buffer) override {};
    bool acceptsMidi() const override
    {
        return true;
    }
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    juce::AudioProcessor::BusesProperties tuningBusLayout()
    {
        return BusesProperties()
            .withOutput ("Output1", juce::AudioChannelSet::stereo(), false)
                .withInput("input",juce::AudioChannelSet::stereo(),false);
    }
    bool hasEditor() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }

private:

    chowdsp::Gain<float> gain;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TuningProcessor)
};


#endif //BITKLAVIER2_TUNINGPROCESSOR_H
