//
// Created by Joshua Warner on 6/27/24.
//

#ifndef BITKLAVIER2_BLENDRONICPROCESSOR_H
#define BITKLAVIER2_BLENDRONICPROCESSOR_H

#pragma once

#include <chowdsp_plugin_base/chowdsp_plugin_base.h>
#include <chowdsp_plugin_utils/chowdsp_plugin_utils.h>
#include <chowdsp_sources/chowdsp_sources.h>
#include <chowdsp_plugin_state/chowdsp_plugin_state.h>
#include "array_to_string.h"
#include "MultiSliderState.h"

//struct BlendronicState : bitklavier::StateChangeableParameter
//{
//    /*
//     * Multisliders for:
//     *      - beat lengths          [0, 8]      => FloatParam multipliers of beat length set by Tempo
//     *      - delay lengths         [0, 8]      => FloatParam multipliers of beat length set by Tempo
//     *      - smoothing             [0, 500]    => ms TimeMsParameter
//     *      - feedback coefficients [0, 1]      => FloatParam
//     *
//     * - max these at MAXMULTISLIDERLENGTH vals for now (will need to be higher for Synchronic multisliders)
//     * - de/serialize these
//     * - keep track of their actual sizes using params as well, set indirectly by the user
//     */
//
//    MultiSliderState beatLengths;
//    MultiSliderState delayLengths;
//    MultiSliderState smoothingTimes;
//    MultiSliderState feedbackCoeffs;
//
//};

struct BlendronicParams : chowdsp::ParamHolder
{
    // gain slider params, for all gain-type knobs
    float rangeStart = -60.0f;
    float rangeEnd = 6.0f;
    float skewFactor = 2.0f;

    // Adds the appropriate parameters to the Blendronic Processor
    BlendronicParams()
    {
        add (beatLengths.numSlidersActual,
            outputGain,
            outputSendParam);
    }

     /*
      * Multisliders for:
      *      - beat lengths          [0, 8]      => FloatParam multipliers of beat length set by Tempo
      *      - delay lengths         [0, 8]      => FloatParam multipliers of beat length set by Tempo
      *      - smoothing             [0, 500]    => ms TimeMsParameter
      *      - feedback coefficients [0, 1]      => FloatParam
      *
      * - max these at MAXMULTISLIDERLENGTH vals for now (will need to be higher for Synchronic multisliders)
      * - de/serialize these
      * - keep track of their actual sizes using params as well, set indirectly by the user
      */
     MultiSliderState beatLengths;
     MultiSliderState delayLengths;
     MultiSliderState smoothingTimes;
     MultiSliderState feedbackCoeffs;

    // Gain for output send (for other blendronics, VSTs, etc...)
    chowdsp::GainDBParameter::Ptr outputSendParam {
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
        juce::NormalisableRange { -80.0f, rangeEnd, 0.0f, skewFactor, false },
        0.0f,
        true
    };

    /*
     * serializers are used for more complex params
     *      - here we need arrays and indexed arrays for circular and absolute tunings, for instance
     */
    /* Custom serializer */
    template <typename Serializer>
    static typename Serializer::SerializedType serialize (const BlendronicParams& paramHolder);

    /* Custom deserializer */
    template <typename Serializer>
    static void deserialize (typename Serializer::DeserializedType deserial, BlendronicParams& paramHolder);
};

struct BlendronicNonParameterState : chowdsp::NonParamState
{
    BlendronicNonParameterState()
    {
        addStateValues ({ &prepPoint });
    }

    chowdsp::StateValue<juce::Point<int>> prepPoint { "prep_point", { 300, 500 } };
};

class BlendronicProcessor : public chowdsp::PluginBase<chowdsp::PluginStateImpl<BlendronicParams,BlendronicNonParameterState>>
{
public:
    BlendronicProcessor();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processAudioBlock (juce::AudioBuffer<float>& buffer) override {};

    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;


    bool hasEditor() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }

private:

    //chowdsp::experimental::Blendronicillator<float> oscillator;
    chowdsp::Gain<float> gain;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BlendronicProcessor)
};


#endif //BITKLAVIER2_BLENDRONICPROCESSOR_H
