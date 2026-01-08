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
 * - change color range display in Gains keyboard
 * - in UI, have keys offset and gain keys that are not relevant greyed out and not clickable
 * - basic setup like processStateChanges and mods
 * - processBlockBypassed
 * - possibly have a display above the ADSR showing the cooked partial structure (including stretch and overlap impacts)?
 *      -- would probably need to be some kind of spectrum display that shows the partial structure for the heldkeys and then
 *          the ringing structure? for all active keys? or could just be the last ring string against all the held strings?
 *      -- not a priority for now....
 *
 */

#ifndef BITKLAVIER2_RESONANCEPROCESSOR_H
#define BITKLAVIER2_RESONANCEPROCESSOR_H

#pragma once

#include "PluginBase.h"
#include "Synthesiser/BKSynthesiser.h"
#include "target_types.h"
#include "TuningUtils.h"
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

//key:gain (dBFS) pairs
    //Overtones8 = 1 << 0,
    //Undertones8 = 1 << 1,
    //Overtones20 = 1 << 2,
    //Undertones20 = 1 << 3,
    //OverUnderTones = 1 << 4,
    //PianoLow = 1 << 5,
    //PianoMid = 1 << 6,
    //PianoHigh = 1 << 7,
    //MetalBar = 1 << 8,
    //MajorBell = 1 << 9,
    //MinorBell = 1 << 10,
    //Saron = 1 << 11,
    //Gender = 1 << 12,
    //Bonang = 1 << 13,
    //Gong = 1 << 14,

// Overtones8
static const std::string OvertoneOffsets_O8   = "0:0 12:0 19:2 24:0 28:-14 31:2 34:-31 36:0";
static const std::string OvertoneGains_O8     = "0:0 12:0 19:0 24:0 28:0 31:0 34:0 36:0";
static const std::string OvertoneKeys_O8      = "0 12 19 24 28 31 34 36";

// Overtone20
static const std::string OvertoneOffsets_O20   = "0:0 12:0 19:2 24:0 28:-14 31:2 34:-31 36:0 38:4 40:-14 42:-49 43:2 44:41 46:-31 47:-12 48:0 49:5 50:4 51:-2.5 52:-14";
static const std::string OvertoneGains_O20     = "0:0 12:0 19:0 24:0 28:0 31:0 34:0 36:0 38:0 40:0 42:0 43:0 44:0 46:0 47:0 48:0 49:0 50:0 51:0 52:0";
static const std::string OvertoneKeys_O20      = "0 12 19 24 28 31 34 36 38 40 42 43 44 46 47 48 49 50 51 52";

// Undertone8
static const std::string UndertoneOffsets_U8   = "16:0 18:31 21:-2 24:14 28:0 33:-2 40:0 52:0";
static const std::string UndertoneGains_U8     = "16:0 18:0 21:0 24:0 28:0 33:0 40:0 52:0";
static const std::string UndertoneKeys_U8      = "16 18 21 24 28 33 40 52";

// Undertone20
static const std::string UndertoneOffsets_U20   = "0:14 1:2.5 2:-4 3:-5 4:0 5:12 6:31 8:-41 9:-2 10:49 12:14 14:-4 16:0 18:31 21:-2 24:14 28:0 33:-2 40:0 52:0";
static const std::string UndertoneGains_U20     = "0:0 1:0 2:0 3:0 4:0 5:0 6:0 8:0 9:0 10:0 12:0 14:0 16:0 18:0 21:0 24:0 28:0 33:0 40:0 52:0";
static const std::string UndertoneKeys_U20      = "0 1 2 3 4 5 6 8 9 10 12 14 16 18 21 24 28 33 40 52";

// OverUndertones ( 7 overtones and 7 undertones, around D (26), for symmetry in UI. Leaving out octave above/below.)
// overtones    "26:0 33:2 38:0 42:-14 45:2 48:-31 50:0 52:4"
// undertones   "0:-4 2:0 4:31 7:-2 10:14 14:0 19:-2"
static const std::string UnderOvertoneOffsets   = "0:-4 2:0 4:31 7:-2 10:14 14:0 19:-2 26:0 33:2 38:0 42:-14 45:2 48:-31 50:0 52:4";
static const std::string UnderOvertoneGains     = "0:0 2:0 4:0 7:0 10:0 14:0 19:0 26:0 33:0 38:0 42:0 45:0 48:0 50:0 52:0";
static const std::string UnderOvertoneKeys      = "0 2 4 7 10 14 19 26 33 38 42 45 48 50 52";

// PianoLow
static const std::string OvertoneOffsets_A0   = "0:0 12:0 19:2 24:0 28:-14 31:2 34:-31 36:0 38:4 40:-14 42:-49 43:2 44:41 46:-31 47:-12 48:0 49:5 50:4 51:-2.5 52:-14";
static const std::string OvertoneGains_A0     = "0:-35 12:-20 19:0 24:-3 28:-6 31:-5 34:-20 36:-28 38:-25 40:-23 42:-30 43:-25 44:-10 46:-15 47:-15 48:-40 49:-41 50:-25 51:-15 52:-7";
static const std::string OvertoneKeys_A0      = "0 12 19 24 28 31 34 36 38 40 42 43 44 46 47 48 49 50 51 52";

// PianoMid
static const std::string OvertoneOffsets_A3   = "0:0 12:0 19:2 24:0 28:-14 31:2 34:-31 36:0";
static const std::string OvertoneGains_A3     = "0:0 12:-40 19:-50 24:-60 28:-18 31:-20 34:-70 36:-60";
static const std::string OvertoneKeys_A3      = "0 12 19 24 28 31 34 36";

// PianoHigh
static const std::string OvertoneOffsets_A7   = "0:0 12:0 ";
static const std::string OvertoneGains_A7     = "0:0 12:-10";
static const std::string OvertoneKeys_A7      = "0 12";

/*
 * the following are based on numbers from Sethares: "Tuning, Timbre, Spectrum, Scale"
 */
// Metal Bar
//f, 2.76f, 5.41f, 8.94f, 13.35f, and 18.65f
// "0:0 18:-42.4 29:23 38:-7.7 45:-13.5 51:-34.7"
static const std::string MetalBarOffsets   = "0:0 18:-42.4 29:23 38:-7.7 45:-13.5 51:-34.7";
static const std::string MetalBarGains     = "0:0 18:0 29:0 38:0 45:0 51:0";
static const std::string MetalBarKeys      = "0 18 29 38 45 51";

// Major Bell
//0.5 1 1.25 1.5 2 2.5 2.95 3.25 4
// "0:0 12:0 16:-14 19:2 24:0 28:-14 31:-27 32:40.5 36"
// fundamental at 12
static const std::string MajorBellOffsets   = "0:0 12:0 16:-14 19:2 24:0 28:-14 31:-27 32:40.5 36:0";
static const std::string MajorBellGains     = "0:0 12:0 16:0 19:0 24:0 28:0 31:0 32:0 36:0";
static const std::string MajorBellKeys      = "0 12 16 19 24 28 31 32 36";

// Minor Bell
//0.5 1 1.2 1.5 2 2.5 2.61 3.0 4
// fundamental at 12
static const std::string MinorBellOffsets   = "0:0 12:0 15:15.6 19:2 24:0 28:-14 29:-31 31:2 36:0";
static const std::string MinorBellGains     = "0:0 12:0 15:0 19:0 24:0 28:0 29:0 31:0 36:0";
static const std::string MinorBellKeys      = "0 12 15 19 24 28 29 31 36";

// Saron
// f, 2.39f, 2.78f, 4.75f, 5.08f, 5.96f.
static const std::string SaronOffsets   = "0:0 15:8.4 18:-30 27:-2.5 28:14 31:-9.6";
static const std::string SaronGains     = "0:0 15:0 18:0 27:0 28:0 31:0";
static const std::string SaronKeys      = "0 15 18 27 28 31";

// Gender
// f, 2.01f, 2.57f, 4.05f, 4.8f, 6.27f
//or f, 1.97f, 2.78f, 4.49f, 5.33f, 6.97f (this one below)
static const std::string GenderOffsets   = "0:0 12:-26 18:-30 26:1 29:-3 34:-38.6";
static const std::string GenderGains     = "0:0 12:0 18:0 26:0 29:0 34:0";
static const std::string GenderKeys      = "0 12 18 26 29 34";

// Bonang:
// f, 1.52f, 3.46f, 3.92f.
static const std::string BonangOffsets   = "0:0 7:25 21:49 42:-35";
static const std::string BonangGains     = "0:0 7:0 21:0 42:0";
static const std::string BonangKeys      = "0:0 7 21 42";

// Gong: f, 1.49f, 1.67f, 2f, 2.67f, 2.98f, 3.47f, 3.98f, 5.97f, 6.94f
static const std::string GongOffsets   = "0:0 7:-9.6 9:-12 12:0 17:0 19:-9.6 22:-46 24:-8.7 31:-6.7 34:-46";
static const std::string GongGains     = "0:0 7:0 9:0 12:0 17:0 19:0 22:0 24:0 31:0 34:0";
static const std::string GongKeys      = "0 7 9 12 17 19 22 24 31 34";

struct ResonanceParams : chowdsp::ParamHolder
{
    // gain slider params, for all gain-type knobs
    float rangeStart = -80.0f;
    float rangeEnd = 6.0f;
    float skewFactor = 2.0f;

    // Adds the appropriate parameters to the Resonance Processor
    ResonanceParams(const juce::ValueTree& v) : chowdsp::ParamHolder ("resonance")
    {
        //gainsKeyboardState.setAllAbsoluteOffsets(-50.);

        add (outputSendGain,
            outputGain,
            noteOnGain,
            presence,
            sustain,
            variance,
            smoothness,
            env,
            stretch,
            spectrum);

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
        "PRESENCE",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 1.f, 0.75f),
        0.75f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal,
        true
    };

    // maps to sustain time
    chowdsp::FloatParameter::Ptr sustain {
        juce::ParameterID { "rsustain", 100 },
        "SUSTAIN",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 1.f, 0.5f),
        0.5f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal,
        true
    };

    /*
     * For the "Overlap Sensitivity" parameter
     * the partials will not always be perfectly aligned, tuning-wise
     * and they also may have varying gains, so we adjust the gain
     * of a particular resonance based on their gains and how
     * far off they are from one another. That adjustment is
     * further scaled by "variance"
     * - if variance = 1, we ignore differences in tuning and the relative
     *      partial gains
     * - if variance is 0, we fully adjust the gain based on these differences
     *
     */
    chowdsp::FloatParameter::Ptr variance {
        juce::ParameterID { "rvariance", 100 },
        "OVERLAP SENSITIVITY",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 1.f, 0.1f),
        0.1f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal,
        true
    };

    /*
     * maps to delay time before resonantString becomes active after held initiated
     * have decided NOT to expose this to the UI, at least not for now. it's just
     * not convincing enough to warrant the users attention.
     */
    chowdsp::FloatParameter::Ptr smoothness {
        juce::ParameterID { "rsmoothness", 100 },
        "smoothness",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 1.f, 0.5f),
        0.5f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal,
        true
    };

    /*
     * a "stretch" knob that will stretch the given partial structure
     *      -- FMn = 1 + alpha * (n - 1)^2, very small range of alpha, like [-.002, .002] so our knob would be for alpha.
     *          - FMn is frequency multiplier for nth partial (n=1 is fundamental), alpha is the stretch factor
     *              - using the 7th partial as a reference; a max of .002 would have it reach almost a M7! with an ET m7 well within that range
     *      -- based on equation 2 from here: OCTOBER 23 2015 "Explaining the Railsback stretch in terms of the inharmonicity of piano tones and sensory dissonance", N. Giordano
     *         - https://pubs.aip.org/asa/jasa/article/138/4/2359/900231/Explaining-the-Railsback-stretch-in-terms-of-the
     *          - there is more here: https://acris.aalto.fi/ws/portalfiles/portal/98111997/Effect_of_inharmonicity_on_pitch_perception_and_subjective_tuning_of_piano_tones.pdf
     */
    chowdsp::FloatParameter::Ptr stretch {
        juce::ParameterID { "rstretch", 100 },
        "STRETCH",
        chowdsp::ParamUtils::createNormalisableRange (-1.0f, 1.0f, 0.0f),
        0.0f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal,
        true
    };

    chowdsp::EnumChoiceParameter<SpectralType>::Ptr spectrum {
        juce::ParameterID { "rspectrum", 100 },
        "SpectralType",
        SpectralType::None,
        std::initializer_list<std::pair<char, char>> { { '_', ' ' }, { '1', '/' }, { '3', '\'' }, { '4', '#' }, { '5', 'b' } }
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

    void setSpectrumFromMenu(int menuChoice);

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
        std::array<PartialSpec, TotalNumberOfPartialKeysInUI + 1>& inPartialStructure,
        std::array<NoteOnSpec, MaxMidiNotes>& inNoteOnSpecMap);

    //~ResonantString() {}
    void addString (int midiNote);
    void ringString(int midiNote, int velocity, juce::MidiBuffer& outMidiMessages);
    void removeString (int midiNote, juce::MidiBuffer& outMidiMessages);
    void incrementTimer_seconds(float blockSize_seconds);
    void finalizeNoteOnMessage(juce::MidiBuffer& outMidiMessages);
    void setTuning(TuningState *tun) {attachedTuning = tun;}

    int heldKey = 0;                // MIDI note value for the key that is being held down
    int channel = 1;                // MIDI channel for this held note
    bool active = false;            // set to false after envelope release time has passed, following removeString()
    bool sendMIDImsg = false;       // set to true whenevern a msg should be sent this block

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
    std::array<PartialSpec, TotalNumberOfPartialKeysInUI + 1>& _partialStructure;
    std::array<NoteOnSpec, MaxMidiNotes>& _noteOnSpecMap;
    float currentVelocity; // for noteOn message

    TuningState *attachedTuning = nullptr;

    JUCE_LEAK_DETECTOR(ResonantString);
};

class ResonanceProcessor : public bitklavier::PluginBase<bitklavier::PreparationStateImpl<ResonanceParams, ResonanceNonParameterState>>,
                           public juce::ValueTree::Listener, public TuningListener
{
public:
    ResonanceProcessor(SynthBase& parent, const juce::ValueTree& v);
    ~ResonanceProcessor(){if(tuning !=nullptr) tuning->removeListener(this);}

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}

    void processAudioBlock (juce::AudioBuffer<float>& buffer) override {};
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    void processBlockBypassed (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    void ProcessMIDIBlock(juce::MidiBuffer& inMidiMessages, juce::MidiBuffer& outMidiMessages, int numSamples);

    void setTuning(TuningProcessor *tun) override;

    void keyPressed(int noteNumber, int velocity, int channel, juce::MidiBuffer& outMidiMessages);
    void keyReleased(int noteNumber, int velocity, int channel, juce::MidiBuffer& outMidiMessages);
    void handleMidiTargetMessages(int noteNumber, int velocity, int channel, juce::MidiBuffer& outMidiMessages);
    void ringSympStrings(int noteNumber, float velocity, juce::MidiBuffer& outMidiMessages);
    void addSympStrings(int noteNumber);
    void toggleSympString(int noteNumber, juce::MidiBuffer& outMidiMessages);

    bool acceptsMidi() const override { return true; }
    bool hasEditor() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    void tuningStateInvalidated() override;
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
             *              for Resonance, we have 7: sendGain and outputGain, + the 5 "qualities"
             */
            .withInput ("Modulation",   juce::AudioChannelSet::discreteChannels (7 * 2), true)  // Mod inputs; numChannels for the number of mods we want to enable
            .withOutput("Modulation",   juce::AudioChannelSet::mono(), false)               // Modulation send channel; disabled for all but Modulation preps!
            .withOutput("Send",         juce::AudioChannelSet::stereo(), true);             // Send channel (right outputs)
    }

    bool isBusesLayoutSupported (const juce::AudioProcessor::BusesLayout& layouts) const override;

//    void addSoundSet (juce::ReferenceCountedArray<BKSamplerSound<juce::AudioFormatReader>>* s)
//    {
//        DBG("Resonance addSoundSet called");
//        resonanceSynth->addSoundSet (s);
//    }

    void addSoundSet (
        juce::ReferenceCountedArray<BKSynthesiserSound > *s, // main samples
        juce::ReferenceCountedArray<BKSynthesiserSound > *h, // hammer samples
        juce::ReferenceCountedArray<BKSynthesiserSound > *r, // release samples
        juce::ReferenceCountedArray<BKSynthesiserSound > *p) // pedal samples
    {
        resonanceSynth->addSoundSet (s);
    }

    void setA4Frequency(float freq)
    {
        resonanceSynth->setA4Frequency(freq);
    }

//    void valueTreePropertyChanged(juce::ValueTree& t, const juce::Identifier& property)
//    {
//        if (t == v && property == IDs::soundset)
//        {
//            juce::String soundset = t.getProperty(property, "");
//            if (soundset == IDs::syncglobal.toString())
//            {
//                juce::String a = t.getProperty(IDs::soundset, "");
//                addSoundSet(&(*parent.getSamples())[a]);
//            }
//            addSoundSet(&(*parent.getSamples())[soundset]);
//
//            return;
//        }
//        if (!v.getProperty(IDs::soundset).equals(IDs::syncglobal.toString()))
//            return;
//        if (property == IDs::soundset && t == parent.getValueTree())
//        {
//            juce::String a = t.getProperty(IDs::soundset, "");
//            addSoundSet(&(*parent.getSamples())[a]);
//        }
//    }

    void valueTreePropertyChanged(juce::ValueTree &t, const juce::Identifier &property) {

        if (t == v && property == IDs::soundset) {
            loadSamples();
            return;
        }
        if (!v.getProperty(IDs::soundset).equals(IDs::syncglobal.toString()))
            return;
        if (property == IDs::soundset && t == parent.getValueTree() ) {
            juce::String soundset = t.getProperty(IDs::soundset, "");
            auto* samples = parent.getSamples();

            addSoundSet(
                samples->contains(soundset) ? (*samples)[soundset] : nullptr,
                nullptr,
                nullptr,
                nullptr
            );
        }
    }

//    void loadSamples() {
//        juce::String soundset = v.getProperty(IDs::soundset, IDs::syncglobal.toString());
//        if (soundset == IDs::syncglobal.toString()) {
//            //if global sync read soundset from global valuetree
//            soundset = parent.getValueTree().getProperty(IDs::soundset, "");
//
//            addSoundSet(&(*parent.getSamples())[soundset]);
//
//        } else {
//            //otherwise set the piano
//            addSoundSet(&(*parent.getSamples())[soundset]);
//
//        }
//    }

    void loadSamples() {
        juce::String soundset = v.getProperty(IDs::soundset, IDs::syncglobal.toString());
        if (soundset == IDs::syncglobal.toString()) {
            //if global sync read soundset from global valuetree
            soundset = parent.getValueTree().getProperty(IDs::soundset, "");

            auto* samples = parent.getSamples();

            addSoundSet(
                samples->contains(soundset) ? (*samples)[soundset] : nullptr,
                nullptr,
                nullptr,
                nullptr
            );
        }else {
            auto* samples = parent.getSamples();

            addSoundSet(
                samples->contains(soundset) ? (*samples)[soundset] : nullptr,
                nullptr,
                nullptr,
                nullptr
            );
        }
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
    std::array<PartialSpec, TotalNumberOfPartialKeysInUI + 1> partialStructure;

    /*
     * will update partialStructure based on parameter settings
     */
    void updatePartialStructure();
    void resetPartialStructure();       // clear it
    void printPartialStructure();
    bool firstNoteOn;  // set this to true every block; if we get a noteOn message, updatePartialStructure and set to false

    std::array<std::unique_ptr<ResonantString>, MaxHeldKeys> resonantStringsArray;
    int currentHeldKey = 0;

    bool bypassed = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ResonanceProcessor)
};


#endif //BITKLAVIER2_RESONANCEPROCESSOR_H
