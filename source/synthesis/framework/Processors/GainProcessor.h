//
// Created by Myra Norton on 11/13/25.
//

#pragma once

#include "EnvParams.h"
#include "EnvelopeSequenceParams.h"
#include "PluginBase.h"
#include "utils.h"
#include <PreparationStateImpl.h>
#include <chowdsp_plugin_utils/chowdsp_plugin_utils.h>
#include <chowdsp_sources/chowdsp_sources.h>

// ********************************************************************************************* //
// ***************************************  GainParams  **************************************** //
// ********************************************************************************************* //

struct GainParams : chowdsp::ParamHolder
{
    // gain slider params, for all gain-type knobs
    float rangeStart = -80.0f;
    float rangeEnd = 6.0f;
    float skewFactor = 2.0f;

    // Adds the appropriate parameters to the Nostalgic Processor
    GainParams(const juce::ValueTree& v) : chowdsp::ParamHolder ("nostalgic")
    {
        add (outputGain);
    }

    // for the output gain slider, final gain stage for this prep (meter slider on right side of prep)
    chowdsp::GainDBParameter::Ptr outputGain {
        juce::ParameterID { "OutputGain", 100 },
        "Output Gain",
        juce::NormalisableRange { rangeStart, rangeEnd, 0.0f, skewFactor, false },
        0.0f,
        true
    };

    /*
     * for storing outputLevels of this preparation for display
     *  because we are using an OpenGL slider for the level meter, we don't use the chowdsp params for this
     *      we simply update this in the processBlock() call
     *      and then the level meter will update its values during the OpenGL cycle
     */
    std::tuple<std::atomic<float>, std::atomic<float>> outputLevels;

    /****************************************************************************************/
};

struct GainNonParameterState : chowdsp::NonParamState
{
    GainNonParameterState() {}
};

// ********************************************************************************************* //
// ************************************** GainProcessor ************************************** //
// ********************************************************************************************* //

class GainProcessor : public bitklavier::PluginBase<bitklavier::PreparationStateImpl<GainParams, GainNonParameterState>>

{
public:
    GainProcessor (SynthBase& parent, const juce::ValueTree& v);
    ~GainProcessor()
    {

    }

    static std::unique_ptr<juce::AudioProcessor> create (SynthBase& parent, const juce::ValueTree& v)
    {
        return std::make_unique<GainProcessor> (parent, v);
    }

    void prepareToPlay (double sampleRate, int samplesPerBlock) override {};
    void releaseResources() override {}

    void processAudioBlock (juce::AudioBuffer<float>& buffer) override {};
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    void processBlockBypassed (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override{};

    juce::AudioProcessor::BusesProperties gainBusLayout();
    bool isBusesLayoutSupported (const juce::AudioProcessor::BusesLayout& layouts) const override {return true;};

    bool acceptsMidi() const override { return true; }
    bool hasEditor() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GainProcessor)
};

