//
// Created by Joshua Warner on 6/27/24.
//

#ifndef BITKLAVIER2_NOSTALGICPROCESSOR_H
#define BITKLAVIER2_NOSTALGICPROCESSOR_H

#pragma once

#include <chowdsp_plugin_base/chowdsp_plugin_base.h>
#include <chowdsp_plugin_utils/chowdsp_plugin_utils.h>
#include <chowdsp_sources/chowdsp_sources.h>
#include <chowdsp_plugin_state/chowdsp_plugin_state.h>

// TODO change params
// volume slider (-inf to 24.00) - on the right hand side in all the other preparations, goes out the mains
// blendronic send volume slider (-inf to 24.00) - send gain (second slider on the right, goes out the right two ports)

// note length multiplier (0 to 10.00) - knob
// cluster slider (1 to 10) - knob, how many notes in the cluster
// cluster threshold slider (0 to 1000) - knob, ms that the notes in the cluster are played within

// hold time range slider (0 to 12000) - exists in synchronic, how long the notes need to be held to be considered

// reverse adsr, first note that plays backward (look at new synchronic)
// undertow adsr, if there's an undertow note that plays forward

// transposition slider (-12.00 to 12.00) - exists in direct
// use tuning checkbox - if it's on, look at the attached tuning

// wave distance (0 to 20000), how far back you go, higher wave distance means more gentle wave
// wave section (wrap in an opengl wrapper, transposition slider is done this way)
    // line that goes through it means tracking the playback position (synthesizer status in direct)
// undertow (0 to 9320), goes forward, dynamically shorten?

// key on reset checkbox

// velocity min/max double slider (0 to 127) - ignore because it's being pulled into keymap



struct NostalgicParams : chowdsp::ParamHolder
{

    // Adds the appropriate parameters to the Nostalgic Processor
    NostalgicParams()
    {
        add (outputSend, outputGain);
    }

    // gain slider params, for all gain-type knobs
    float rangeStart = -80.0f;
    float rangeEnd = 6.0f;
    float skewFactor = 2.0f;

    // Gain for output send (for other blendronics, VSTs, etc...)
    chowdsp::GainDBParameter::Ptr outputSend {
        juce::ParameterID { "Send", 100 },
        "Send",
        juce::NormalisableRange { rangeStart, rangeEnd, 0.0f, skewFactor, false },
        0.0f,
        true
    };

    // for the output gain slider, final gain stage for this prep (meter slider on right side of prep)
    chowdsp::GainDBParameter::Ptr outputGain {
        juce::ParameterID { "OutputGain", 100 },
        "Output Gain",
        juce::NormalisableRange { rangeStart, rangeEnd, 0.0f, skewFactor, false },
        0.0f,
        true
    };
};

struct NostalgicNonParameterState : chowdsp::NonParamState
{
    NostalgicNonParameterState()
    {
        addStateValues ({ &prepPoint });
    }

    chowdsp::StateValue<juce::Point<int>> prepPoint { "prep_point", { 300, 500 } };
};

class NostalgicProcessor : public chowdsp::PluginBase<chowdsp::PluginStateImpl<NostalgicParams,NostalgicNonParameterState>>
{
public:
    NostalgicProcessor();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processAudioBlock (juce::AudioBuffer<float>& buffer) override {};

    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;


    bool hasEditor() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }

private:

    chowdsp::Gain<float> gain;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NostalgicProcessor)
};


#endif //BITKLAVIER2_NOSTALGICPROCESSOR_H
