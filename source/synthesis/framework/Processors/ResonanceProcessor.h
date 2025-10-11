//
// Created by Joshua Warner on 6/27/24.
//

#ifndef BITKLAVIER2_RESONANCEPROCESSOR_H
#define BITKLAVIER2_RESONANCEPROCESSOR_H

#pragma once

#include "PluginBase.h"
#include "Synthesiser/BKSynthesiser.h"

#include <PreparationStateImpl.h>
#include <chowdsp_plugin_base/chowdsp_plugin_base.h>
#include <chowdsp_plugin_utils/chowdsp_plugin_utils.h>
#include <chowdsp_sources/chowdsp_sources.h>
#include <chowdsp_plugin_state/chowdsp_plugin_state.h>
#include "utils.h"
#include <utility>

struct ResonanceParams : chowdsp::ParamHolder
{
    // gain slider params, for all gain-type knobs
    float rangeStart = -80.0f;
    float rangeEnd = 6.0f;
    float skewFactor = 2.0f;

    // Adds the appropriate parameters to the Resonance Processor
    ResonanceParams(const juce::ValueTree& v) : chowdsp::ParamHolder ("resonance")
    {
        gainsKeyboardState.setAllAbsoluteOffsets(1.);

        add (outputSendGain,
            outputGain,
            noteOnGain,
            env);

        // params that are audio-rate modulatable are added to vector of all continuously modulatable params
        doForAllParameters ([this] (auto& param, size_t) {
            if (auto* sliderParam = dynamic_cast<chowdsp::ChoiceParameter*> (&param))
                if (sliderParam->supportsMonophonicModulation())
                    modulatableParams.push_back ( sliderParam);

            if (auto* sliderParam = dynamic_cast<chowdsp::BoolParameter*> (&param))
                if (sliderParam->supportsMonophonicModulation())
                    modulatableParams.push_back ( sliderParam);

            if (auto* sliderParam = dynamic_cast<chowdsp::FloatParameter*> (&param))
                if (sliderParam->supportsMonophonicModulation())
                    modulatableParams.push_back ( sliderParam);
        });
    }

    // Gain for output send (for blendronic, VSTs, etc...)
    chowdsp::GainDBParameter::Ptr outputSendGain {
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

    // passed to BKSynth, applies to noteOn msgs
    chowdsp::GainDBParameter::Ptr noteOnGain {
        juce::ParameterID { "noteOnGain", 100 },
        "NoteOn Gain Scalar",
        juce::NormalisableRange { rangeStart, rangeEnd, 0.0f, skewFactor, false },
        0.0f,
        true
    };

    // adsr
    EnvParams env;

    /*
     * a bit overkill using the full TuningState here, but otherwise we'd have to significantly rewrite the AbsoluteKeyboardSlider
     * - don't need to "add" anything, seince we're just going to serialize absoluteTuningOffset for both of these
     */
    TuningState offsetsKeyboardState;
    TuningState gainsKeyboardState;

    KeymapKeyboardState fundamentalKeymap;
    KeymapKeyboardState closestKeymap;

    std::tuple<std::atomic<float>, std::atomic<float>> outputLevels;
    std::tuple<std::atomic<float>, std::atomic<float>> sendLevels;

    /* Custom serializer */
    template <typename Serializer>
    static typename Serializer::SerializedType serialize (const ResonanceParams& paramHolder);

    /* Custom deserializer */
    template <typename Serializer>
    static void deserialize (typename Serializer::DeserializedType deserial, ResonanceParams& paramHolder);

    void processStateChanges() override
    {

//        transpositions.processStateChanges();
//        accents.processStateChanges();
//        sustainLengthMultipliers.processStateChanges();
//        beatLengthMultipliers.processStateChanges();
//
//        // signal the UI to redraw the sliders
//        if( transpositions.updateUI == true || accents.updateUI == true || sustainLengthMultipliers.updateUI == true || beatLengthMultipliers.updateUI == true)
//        {
//            /*
//             * need to actually change the value for the listener to get the message
//             * we're just using updateUIState as a way to notify the UI, and its actual value doesn't matter
//             * so we switch it everything we one of the sliders gets modded.
//             */
//            if(updateUIState->get())
//                updateUIState->setValueNotifyingHost(false);
//            else
//                updateUIState->setValueNotifyingHost(true);
//        }
    }
};

struct ResonanceNonParameterState : chowdsp::NonParamState
{
    ResonanceNonParameterState()
    {
        addStateValues ({ &prepPoint });
    }

    chowdsp::StateValue<juce::Point<int>> prepPoint { "prep_point", { 300, 500 } };
};

class ResonanceProcessor : public bitklavier::PluginBase<bitklavier::PreparationStateImpl<ResonanceParams, ResonanceNonParameterState>>,
                           public juce::ValueTree::Listener
{
public:
    ResonanceProcessor(SynthBase& parent, const juce::ValueTree& v);
    ~ResonanceProcessor(){}

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}

    void processAudioBlock (juce::AudioBuffer<float>& buffer) override {};
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    void processBlockBypassed (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    void processContinuousModulations(juce::AudioBuffer<float>& buffer);

    bool acceptsMidi() const override { return true; }
    bool hasEditor() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }

    /*
     * this is where we define the buses for audio in/out, including the param modulation channels
     *      the "discreteChannels" number is currently just by hand set based on the max that this particularly preparation could have
     *      so if you add new params, might need to increase that number
     */
    juce::AudioProcessor::BusesProperties resonanceBusLayout()
    {
        return BusesProperties()
            .withOutput("Output",       juce::AudioChannelSet::stereo(), true)      // Main Output
            .withInput ("Input",        juce::AudioChannelSet::stereo(), false)     // Main Input (not used here)

            /**
             * IMPORTANT: set discreteChannels below equal to the number of params you want to continuously modulate!!
             *              for Resonance, we have 2: sendGain and outputGain
             */
            .withInput ("Modulation",   juce::AudioChannelSet::discreteChannels (2), true)  // Mod inputs; numChannels for the number of mods we want to enable
            .withOutput("Modulation",   juce::AudioChannelSet::mono(), false)               // Modulation send channel; disabled for all but Modulation preps!
            .withOutput("Send",         juce::AudioChannelSet::stereo(), true);             // Send channel (right outputs)
    }

    bool isBusesLayoutSupported (const juce::AudioProcessor::BusesLayout& layouts) const override;

    void addSoundSet (juce::ReferenceCountedArray<BKSamplerSound<juce::AudioFormatReader>>* s)
    {
        DBG("Resonance addSoundSet called");
        resonanceSynth->addSoundSet (s);
    }

    /*
     * noteOnSpecMap
     * - key      => midiNoteNumber
     * - value    => specs for that key (start time, direction, loop mode)
     *
     * needed in particular for backwards-playing notes
     */
    std::map<int, NoteOnSpec> noteOnSpecMap;

private:

    std::unique_ptr<BKSynthesiser> resonanceSynth;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ResonanceProcessor)
};


#endif //BITKLAVIER2_RESONANCEPROCESSOR_H
