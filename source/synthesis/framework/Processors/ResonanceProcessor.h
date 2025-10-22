/*
==============================================================================

  Resonance.cpp
  Created: 12 May 2021 12:41:26pm
  Author:  Dan Trueman and Theodore R Trevisan
  Rewritten: Dan Trueman, October 2025

  Models the sympathetic resonance within the piano, with options for
  static resonances akin to the Hardanger fiddle

==============================================================================
*/

/**
 * Resonance ToDo list:
 *
 * - handleMidiTargetMessages, and updates to MidiTarget
 *
 * - create a way to pull up some standard partial structures
 *      -- up to 19 natural overtones
 *      -- perhaps some other fun variants, like a "minor" overtone series with 6/5 instead of 5/4
 *      -- also undertone series, and combo of undertone/overtone, with fundamental mid-keyboard
 *      -- some other structures; look at Sethares, for instance, for some other instrument partial structures
 *      -- perhaps a menu, like the tuning system menus, where we can call up 4, 8, 12, 16, 19, overtones, and some gongs, stretched spectra etc...
 *          -- could even have a "stretch" parameter that stretches the spectra? and adjusts the "closest" key automatically?
 *          -- ideal bar from Sethares: f, 2.76f, 5.41f, 8.94f, 13.35f, and 18.65f
 *          -- Bell table on Sethares page 117
 *          -- Sarons (p 204 in Sethares): f, 2.39f, 2.78f, 4.75f, 5.08f, 5.96f.
*           -- Gender
 *              - f, 2.01f, 2.57f, 4.05f, 4.8f, 6.27f
 *              - f, 1.97f, 2.78f, 4.49f, 5.33f, 6.97f
 *          -- Bonang: f, 1.52f, 3.46f, 3.92f.
 *          -- Gong: f, 1.49f, 1.67f, 2f, 2.67f, 2.98f, 3.47f, 3.98f, 5.97f, 6.94f
 * - a "stretch" knob that will stretch the given series; constrains to small range, at least for now 0.9 - 1.1 or similar
 *      -- use equation 2 from here: https://pubs.aip.org/asa/jasa/article/138/4/2359/900231/Explaining-the-Railsback-stretch-in-terms-of-the
 *          - OCTOBER 23 2015 "Explaining the Railsback stretch in terms of the inharmonicity of piano tones and sensory dissonance", N. Giordano
 *      -- Fn = nF1 * (1 + alpha * n^2), so our knob would be for alpha.
 *          - Fn is frequency of nth partial, F1 is frequency of fundamental, n is partial #
 *
 * - in UI, have keys offset and gain keys that are not relevant greyed out and not clickable
 * - basic setup like processStateChanges and mods
 * - processBlockBypassed
 *
 */

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

using PartialSpec = std::tuple<bool, float, float>;     // active, offset from ET in cents, gain
static constexpr int MaxHeldKeys = 16;                  // currently constrained by number of MIDI channels; might change if we use threaded synth
static constexpr int TotalNumberOfPartialKeysInUI = 52; //number if keys in UI elements for setting partial structure

//float intervalToRatio(float interval) {
//    return mtof(interval + 60., 440.) / mtof(60., 440.);
//}

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
            presence,
            sustain,
            variance,
            smoothness,
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

    // presence maps to start time, inverted
    chowdsp::FloatParameter::Ptr presence {
        juce::ParameterID { "rpresence", 100 },
        "presence",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 1.f, 0.25f),
        0.25f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal,
        true
    };

    // maps to sustain time
    chowdsp::FloatParameter::Ptr sustain {
        juce::ParameterID { "rsustain", 100 },
        "sustain",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 1.f, 0.5f),
        0.5f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal,
        true
    };

    /*
     * the partials will not always be perfectly aligned, tuning-wise
     * and they also may have varying gains, so we adjust the gain
     * of a particular resonance based on their gains and how
     * far off they are from one another. That adjustment is
     * further scaled by "variance"
     * - if variance = 0, we ignore differences in tuning and the relative
     *      partial gains
     * - if variance is 1, we fully adjust the gain based on these differences
     *
     * gain adjustment = variance * (gain1 * gain2 * fabs(1. - offsetDifference)^2), roughly
     *  - see the code in ringString() for exactly what it is
     */
    chowdsp::FloatParameter::Ptr variance {
        juce::ParameterID { "rvariance", 100 },
        "variance",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 1.f, 0.5f),
        1.0f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal,
        true
    };

    // maps to delay time before resonantString becomes active after held initiated
    chowdsp::FloatParameter::Ptr smoothness {
        juce::ParameterID { "rsmoothness", 100 },
        "smoothness",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 1.f, 0.5f),
        0.5f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal,
        true
    };

    // adsr
    EnvParams env;

    /*
     * a bit overkill using the full TuningState here, but otherwise we'd have to significantly rewrite the AbsoluteKeyboardSlider
     * - don't need to "add" anything, since we're just going to serialize absoluteTuningOffset for both of these
     */
    TuningState offsetsKeyboardState;
    TuningState gainsKeyboardState;

    KeymapKeyboardState fundamentalKeymap;
    KeymapKeyboardState closestKeymap;
    KeymapKeyboardState heldKeymap;
    std::atomic<int> heldKeymap_changedInUI = 0; // note number for key that was changed

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

/*
 * ResonantString Class
 *
 * Every held note on the keyboard will have a ResonantString class, which keeps track
 * of all the partials associated with this held note, sending noteOn and noteOff
 * messages as needed. Each instantiation of ResonantString will be on a separate MIDI channel
 * to keep the behaviors of the held notes disambiguated.
 *
 * About disambiguation: since different held notes might have the same partials that are ringing,
 * releasing one key won't necessarily turn off all the sympathetic resonances at a particular
 * pitch. For instance, if both F3 and C4 are held down, and C3 is struck, then C5 will resonate
 * for BOTH F3 and C4 (and they will be slightly different C5s, 2c off from one another!).
 * If we release the F3 key, we want to turn of the resonating C5 for the F3 string, but not
 * for the C4 string.
 *
 * About the MIDI channels: every ResonantString will be assigned it the next a MIDI channel and
 * all noteOn/Off messages will be sent on that channel. At the moment, we are allocating 16
 * ResonantStrings, one per each MIDI channel and simply noting whether they are "active" or not.
 * When a key is pressed, we look for an inactive ResonantString and use the first available. In
 * practice, 16 should be more than enough, especially given how CPU intensive this can be.
 * Ideally, we will make sure that all the strings have "rung down" (envelopes fully released)
 * before we mark a string as available.
 *
 * Note: when we try the multi-threaded synth, perhaps the MIDI channels can simply be assigned
 * to different threads
 */
class ResonantString
{
public:
    ResonantString(
        ResonanceParams* inparams,
        std::array<PartialSpec, TotalNumberOfPartialKeysInUI>& inPartialStructure,
        std::array<NoteOnSpec, MaxMidiNotes>& inNoteOnSpecMap);

    //~ResonantString() {}
    void addString (int midiNote);
    void ringString(int midiNote, int velocity, juce::MidiBuffer& outMidiMessages);
    void removeString (int midiNote, juce::MidiBuffer& outMidiMessages);
    void incrementTimer_seconds(float blockSize_seconds);
    void finalizeNoteOnMessage(juce::MidiBuffer& outMidiMessages);
    void setTuning(TuningProcessor *tun) {attachedTuning = tun;}

    int heldKey = 0;                // MIDI note value for the key that is being held down
    int channel = 1;                // MIDI channel for this held note
    bool active = false;            // set to false after envelope release time has passed, following removeString()

    bool stringJustRemoved = false;     // set to true when removeString is called, to start timer to release after release time has passed
    float timeToMakeInactive;           // set this to releaseTime when removeString is called, in seconds
    float timeSinceRemoved = 0.0f;      // increment this every block if stringJustRemoved == true, in seconds

    bool stringJustAdded = false;       // set to true when addString is called, to start timer and activate after timeToMakeActive has passed
    float timeToMakeActive = .05f;      // how much time to pass before making active; 50ms by default, but expose to user
    float timeSinceAdded = 0.0f;        // increment this every block if stringJustAdded = true;

private:
    ResonanceParams* _rparams;

    /*
     * partialStructure
     * - active (bool; false => default)
     * - offset from fundamental (fractional MIDI note val; 0. => default)
     * - gains (floats; 1.0 => default)
     */
    std::array<PartialSpec, TotalNumberOfPartialKeysInUI>& _partialStructure;
    std::array<NoteOnSpec, MaxMidiNotes>& _noteOnSpecMap;
    float currentVelocity; // for noteOn message

    TuningProcessor *attachedTuning = nullptr;

    JUCE_LEAK_DETECTOR(ResonantString);
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
    void ProcessMIDIBlock(juce::MidiBuffer& inMidiMessages, juce::MidiBuffer& outMidiMessages, int numSamples);

    void setTuning(TuningProcessor *tun) override;

    void keyPressed(int noteNumber, int velocity, int channel, juce::MidiBuffer& outMidiMessages);
    void keyReleased(int noteNumber, juce::MidiBuffer& outMidiMessages);
    void handleMidiTargetMessages(int channel);
    void ringSympStrings(int noteNumber, float velocity, juce::MidiBuffer& outMidiMessages);
    void addSympStrings(int noteNumber);

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
             *              for Resonance, we have 6: sendGain and outputGain, + the 4 qualities
             */
            .withInput ("Modulation",   juce::AudioChannelSet::discreteChannels (6), true)  // Mod inputs; numChannels for the number of mods we want to enable
            .withOutput("Modulation",   juce::AudioChannelSet::mono(), false)               // Modulation send channel; disabled for all but Modulation preps!
            .withOutput("Send",         juce::AudioChannelSet::stereo(), true);             // Send channel (right outputs)
    }

    bool isBusesLayoutSupported (const juce::AudioProcessor::BusesLayout& layouts) const override;

    void addSoundSet (juce::ReferenceCountedArray<BKSamplerSound<juce::AudioFormatReader>>* s)
    {
        DBG("Resonance addSoundSet called");
        resonanceSynth->addSoundSet (s);
    }

private:
    std::unique_ptr<BKSynthesiser> resonanceSynth;

    /* the two primary modes, set by target msgs
     *  - channel 1 => both are true, default behavior
     *  - channel 2 => only ring the currently held strings
     *  - channel 3 => only add/subtract to/from the currently held strings
     */
    bool doRing = true;
    bool doAdd = true;

    /*
     * noteOnSpecMap
     * - key      => midiNoteNumber
     * - value    => specs for that key (start time, direction, loop mode)
     *
     * needed here to play notes with start time > 0
     * (originally was std::map, but changed to std::array for audio thread safety)
     */
    std::array<NoteOnSpec, MaxMidiNotes> noteOnSpecMap;

    /*
     * partialStructure
     * - active (bool; false => default)
     * - offset from fundamental (fractional MIDI note val; 0. => default)
     * - gains (floats; 1.0 => default)
     */
    std::array<PartialSpec, TotalNumberOfPartialKeysInUI> partialStructure;

    /*
     * will update partialStructure based on parameter settings
     */
    void updatePartialStructure();
    void resetPartialStructure();       // clear it
    void setDefaultPartialStructure();  // set to standard overtone series, first 8 partials
    void printPartialStructure();

    std::array<std::unique_ptr<ResonantString>, MaxHeldKeys> resonantStringsArray;
    int currentHeldKey = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ResonanceProcessor)
};


#endif //BITKLAVIER2_RESONANCEPROCESSOR_H
