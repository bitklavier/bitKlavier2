//
// Created by Joshua Warner on 6/27/24.
//

#ifndef BITKLAVIER2_BLENDRONICPROCESSOR_H
#define BITKLAVIER2_BLENDRONICPROCESSOR_H

#pragma once

#include "PluginBase.h"
#include "Identifiers.h"
#include "array_to_string.h"
#include "MultiSliderState.h"
#include <PreparationStateImpl.h>
#include <chowdsp_plugin_base/chowdsp_plugin_base.h>
#include <chowdsp_plugin_utils/chowdsp_plugin_utils.h>
#include <chowdsp_sources/chowdsp_sources.h>
#include <chowdsp_plugin_state/chowdsp_plugin_state.h>
#include <chowdsp_serialization/chowdsp_serialization.h>
#include <chowdsp_sources/chowdsp_sources.h>

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
    BlendronicParams() : chowdsp::ParamHolder ("blendronic")
    {
        add (
//            beatLengths_numSlidersActual,
//            delayLengths_numSlidersActual,
//            smoothingTimes_numSlidersActual,
//            feedbackCoeffs_numSlidersActual,
            outputGain,
            inputGain,
            outputSend);
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

     /*
     * how many sliders is the user actually working with (int)
     *      - there will always be at least 1
     *      - and 12 displayed
     *      - but the user might be using any number 1 up to MAXMULTISLIDERLENGTH
     */
//     chowdsp::FloatParameter::Ptr beatLengths_numSlidersActual {
//         juce::ParameterID { "beatLengths_numSlidersActual", 100 },
//         "beatLengths_numSlidersActual",
//         chowdsp::ParamUtils::createNormalisableRange (1.0f, static_cast<float>(MAXMULTISLIDERLENGTH), 64.0f),
//         1.0f,
//         &chowdsp::ParamUtils::floatValToString,
//         &chowdsp::ParamUtils::stringToFloatVal
//     };
//
//     chowdsp::FloatParameter::Ptr delayLengths_numSlidersActual {
//         juce::ParameterID { "delayLengths_numSlidersActual", 100 },
//         "delayLengths_numSlidersActual",
//         chowdsp::ParamUtils::createNormalisableRange (1.0f, static_cast<float>(MAXMULTISLIDERLENGTH), 64.0f),
//         1.0f,
//         &chowdsp::ParamUtils::floatValToString,
//         &chowdsp::ParamUtils::stringToFloatVal
//     };
//
//     chowdsp::FloatParameter::Ptr smoothingTimes_numSlidersActual {
//         juce::ParameterID { "smoothingTimes_numSlidersActual", 100 },
//         "smoothingTimes_numSlidersActual",
//         chowdsp::ParamUtils::createNormalisableRange (1.0f, static_cast<float>(MAXMULTISLIDERLENGTH), 64.0f),
//         1.0f,
//         &chowdsp::ParamUtils::floatValToString,
//         &chowdsp::ParamUtils::stringToFloatVal
//     };
//
//     chowdsp::FloatParameter::Ptr feedbackCoeffs_numSlidersActual {
//         juce::ParameterID { "feedbackCoeffs_numSlidersActual", 100 },
//         "feedbackCoeffs_numSlidersActual",
//         chowdsp::ParamUtils::createNormalisableRange (1.0f, static_cast<float>(MAXMULTISLIDERLENGTH), 64.0f),
//         1.0f,
//         &chowdsp::ParamUtils::floatValToString,
//         &chowdsp::ParamUtils::stringToFloatVal
//     };

     // To adjust the gain of signals coming in to blendronic
     chowdsp::GainDBParameter::Ptr inputGain {
         juce::ParameterID { "InputGain", 100 },
         "Input Gain",
         juce::NormalisableRange { rangeStart, rangeEnd, 0.0f, skewFactor, false },
         0.0f,
         true
     };

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

    /** for storing outputLevels of this preparation for display
     *  because we are using an OpenGL slider for the level meter, we don't use the chowdsp params for this
     *      we simply update this in the processBlock() call
     *      and then the level meter will update its values during the OpenGL cycle
     */
    std::tuple<std::atomic<float>, std::atomic<float>> outputLevels;
};

struct BlendronicNonParameterState : chowdsp::NonParamState
{
    BlendronicNonParameterState()
    {
    }
};

class BlendronicProcessor : public bitklavier::PluginBase<bitklavier::PreparationStateImpl<BlendronicParams, BlendronicNonParameterState>>
//class BlendronicProcessor : public bitklavier::PluginBase<bitklavier::PreparationStateImpl<BlendronicParams, BlendronicNonParameterState>>,
//                            public juce::ValueTree::Listener
{
public:
    BlendronicProcessor (SynthBase& parent, const juce::ValueTree& v);

    static std::unique_ptr<juce::AudioProcessor> create (SynthBase& parent, const juce::ValueTree& v)
    {
        return std::make_unique<BlendronicProcessor> (parent, v);
    }

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processAudioBlock (juce::AudioBuffer<float>& buffer) override {};
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    void processBlockBypassed (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    juce::AudioProcessor::BusesProperties blendronicBusLayout()
    {
        return BusesProperties()
            .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
            .withInput ("Input", juce::AudioChannelSet::stereo(), true)
            .withInput ("Modulation", juce::AudioChannelSet::discreteChannels (9), true);
    }
    bool isBusesLayoutSupported (const juce::AudioProcessor::BusesLayout& layouts) const override { return true; }

    bool hasEditor() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }

private:

    //chowdsp::experimental::Blendronicillator<float> oscillator;
    chowdsp::Gain<float> gain;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BlendronicProcessor)
};


#endif //BITKLAVIER2_BLENDRONICPROCESSOR_H
