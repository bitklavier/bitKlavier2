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

using PartialSpec = std::tuple<bool, float, float>;
static constexpr int MaxHeldKeys = 16;
static constexpr int TotalNumberOfPartialKeysInUI = 52; //number if keys in UI elements for setting partial structure

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

/*
 * Every held note on the keyboard will have an ResonantString class, which keeps track
 * of all the partials associated with this held note, sending noteOn and noteOff
 * messages as needed, on separate channels to keep the behaviors of the held notes
 * disambiguated.
 *
 * About disambiguation: since different hold notes might have the same partials that are ringing,
 * releasing one key won't necessarily turn off all the sympathetic resonances at a particular
 * pitch. For instance, if both F3 and C4 are held down, and C3 is struck, then C5 will resonate
 * for BOTH F3 and C4 (and they will be slightly different C5s!). If we release the F3 key, we
 * want to turn of the resonating C5 for the F3 string, but not for the C4 string.
 *
 * About the MIDI channels: whenever we create a new ResonantString, we will assign it the next
 * available MIDI channel and all noteOn/Off messages will be sent on that channel. We will need to
 * make sure that all the strings have "rung down" (envelopes fully released) before we mark
 * a channel as available.
 *
 * note: when we try the multi-threaded synth, perhaps the MIDI channels can simply be assigned
 * to different threads
 */
class ResonantString
{
public:
    ResonantString(
        ResonanceParams* inparams,
        std::array<PartialSpec, TotalNumberOfPartialKeysInUI>& inPartialStructure,
        std::array<NoteOnSpec, MaxMidiNotes>& inNoteOnSpecMap)
        : _rparams(inparams),
          _partialStructure(inPartialStructure),
          _noteOnSpecMap(inNoteOnSpecMap)
    {
        heldKey = 0;
        active = false;

        for (int i = 0; i < MaxMidiNotes; ++i)
        {
            currentPlayingPartialsFromHeldKey.add(juce::Array<int>{});
            juce::Array<int>& internalArray = currentPlayingPartialsFromHeldKey.getReference(i);
            internalArray.ensureStorageAllocated(MaxMidiNotes);
        }
    }

    //~ResonantString() {}

    int heldKey = 0;                // MIDI note value for the key that is being held down
    int channel = 1;                // MIDI channel for this held note
    bool active = false;            // set to false after envelope release time has passed, following removeString()

    /**
     * when a key is pressed, call initialize string to setup the partials
     */
    void addString (int midiNote)
    {
        heldKey = midiNote;
        active = true;
        //DBG("added sympathetic string, channel = " + juce::String(channel));
    }

    /**
     * when another note is played, call ringString(), which will look for overlapping
     * partials and send the appropriate noteOn messages
     */
    void ringString(int midiNote, int velocity, juce::MidiBuffer& outMidiMessages)
    {
        if(!active) return;

        for (auto& heldPartialToCheck : _partialStructure)
        {
            // if not an active partial from UI, skip
            if(!std::get<0>(heldPartialToCheck)) continue;

            for (auto& struckPartialToCheck : _partialStructure)
            {
                // again, skip if partial is inactive
                if(!std::get<0>(struckPartialToCheck)) continue;

                float heldPartial       = static_cast<float>(heldKey) + std::get<1>(heldPartialToCheck);
                int   heldPartialKey    = std::round(heldPartial);

                float struckPartial       = static_cast<float>(midiNote) + std::get<1>(struckPartialToCheck);
                int   struckPartialKey    = std::round(struckPartial);

                if (heldPartialKey == struckPartialKey)
                {
                    /*
                     * todo: decide how to use these
                     *          - we could have differences between the struck and held offsets impact how much sympathetic resonance is induced
                     *          - and we can have the gain be a multiplier of the velocity, or considered some other way
                     *          - also might be fine to ignore!
                     */
                    float heldPartialOffset = heldPartial - static_cast<float>(heldPartialKey);
                    float heldPartialGain   = std::get<2>(heldPartialToCheck);
                    float struckPartialOffset = struckPartial - static_cast<float>(struckPartialKey);
                    float struckPartialGain   = std::get<2>(struckPartialToCheck);

                    /*
                     * add the held key offset for this partialKey to the transpositions
                     *  - we may have more than one partial attached to this key, with different offsets
                     *  - but we also don't want to add duplicates, so only add if not already there
                     */
                    _noteOnSpecMap[heldPartialKey].transpositions.addIfNotAlreadyThere(heldPartialOffset);

                    /*
                     * start time should be into the sample, to play just its tail
                     * - perhaps modulate with velocity, and/or set range by user as in old bK?
                     * - hard-wired for now, and maybe that's best anyhow...
                     *
                     * also, setting a fixed sustain time to avoid pileup of nearly-silent tails being added to the CPU load
                     * - setting the release time will be crucial for how it all sounds (and the CPU load), since that is in addition to the sustain time here
                     */
                    _noteOnSpecMap[heldPartialKey].channel = channel;
                    _noteOnSpecMap[heldPartialKey].startTime = 400;      // ms
                    _noteOnSpecMap[heldPartialKey].sustainTime = 2000;   // ms
                    _noteOnSpecMap[heldPartialKey].stopSameCurrentNote = false;
                    //DBG("held key associated with partial " + juce::String(heldPartialKey) + " = " + juce::String(heldKeys[heldPartialKey_index]));

                    /*
                     * make the midi message, play it on the channel for this particular resonant string
                     */
                    auto newmsg = juce::MidiMessage::noteOn (channel, heldPartialKey, static_cast<float>(velocity/128.)); // velocity * partialGain?
                    outMidiMessages.addEvent(newmsg, 0);

                    /*
                     * add this partial to the array of partial associated with this heldKey that are currently playing
                     * - for noteOffs when the heldKey is released
                     */
                    auto& cp = currentPlayingPartialsFromHeldKey.getReference(heldKey);
                    cp.addIfNotAlreadyThere(heldPartialKey);

                    //DBG("found overlap of partials, play resonance here: " + juce::String(struckPartial) + " at key " + juce::String(heldPartialKey) + " with offset " + juce::String(heldPartialOffset));

                }
            }
        }
    }

    /**
     * when keys are released, we call removeString to see if this heldKey should been released,
     * and if so, send the appropriate noteOff messages.
     * this should also start a timer of some sort that will set doneRinging to true after
     * the noteOff release time has passed, and release the MIDI channel
     */
    void removeString (int midiNote, juce::MidiBuffer& outMidiMessages)
    {
        /*
         * need to figure out how to delay setting this until release time is done
         */
        active = false;
        //DBG("removed string " + juce::String(midiNote) + " on channel " + juce::String(channel));

        // read through currentPlayingPartialsFromHeldKey and send noteOffs for each
        for (auto pnotes : currentPlayingPartialsFromHeldKey[midiNote])
        {
            _noteOnSpecMap[pnotes].keyState = true; // override the UI controlled envelope and use envParams specified here
            _noteOnSpecMap[pnotes].envParams = {50.0f * .001, 10.0f * .001, 1.0f, 50.0f * .001, 0.0f, 0.0f, 0.0f};
            _noteOnSpecMap[pnotes].channel = channel;

            //DBG("sending noteOffs for partial " + juce::String(pnotes) + " of held note " + juce::String(midiNote) + " on channel " + juce::String(channel));
            auto newmsg = juce::MidiMessage::noteOff (channel, pnotes, 0.0f);
            outMidiMessages.addEvent(newmsg, 0);
        }
        currentPlayingPartialsFromHeldKey.set(midiNote, {});
    }

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

    // held key is index of outer array, inner array includes all partial currently playing from that associated held key
    juce::Array<juce::Array<int>> currentPlayingPartialsFromHeldKey;

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

//    void addPartial(int heldKey, int partialKey, float gain, float offset);
//    void removePartialsForHeldKey(int heldKey);


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
    //std::array<ResonantString*, MaxHeldKeys> resonantStringsArray;

    /*
     * partials are collected here
     * - every partial is associated with a heldKey
     * - and is close to a partialKey
     * - and has an offset from ET (relative to partialKey)
     * - and gain multiplier
     * - newest partials are beginning of arrays, oldest get pushed off the end
     * - these parallel arrays are kept in sync via addPartial and removePartialsForHeldKey
     */
//    std::array<int, MAX_SYMPSTRINGS> heldKeys{};      // midiNoteNumber for key that is held down; for the undamped string that has this partial
//    std::array<int, MAX_SYMPSTRINGS> partialKeys{};   // midiNoteNumber for nearest key to this partial; used to determine whether this partial gets excited
//    std::array<float, MAX_SYMPSTRINGS> gains{};       // gain multiplier for this partial
//    std::array<float, MAX_SYMPSTRINGS> offsets{};     // offset, in cents, from ET for this partial
//    std::array<int, MAX_SYMPSTRINGS> startTimes{};    // time, in ms, that this partial began playing
//
//    // held key is index of outer array, inner array includes all partial currently playing from that associated held key
//    juce::Array<juce::Array<int>> currentPlayingPartialsFromHeldKey;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ResonanceProcessor)
};


#endif //BITKLAVIER2_RESONANCEPROCESSOR_H
