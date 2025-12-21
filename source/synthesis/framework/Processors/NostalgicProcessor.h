//
// Created by Davis Polito on 5/2/24.
//

#pragma once

#include "EnvParams.h"
#include "EnvelopeSequenceParams.h"
#include "Identifiers.h"
#include "PluginBase.h"
#include "Synthesiser/BKSynthesiser.h"
#include "TransposeParams.h"
#include "WaveDistUndertowParams.h"
#include "TuningProcessor.h"
#include "VelocityMinMaxParams.h"
#include "buffer_debugger.h"
#include "utils.h"
#include <PreparationStateImpl.h>
#include <chowdsp_plugin_utils/chowdsp_plugin_utils.h>
#include <chowdsp_serialization/chowdsp_serialization.h>
#include <chowdsp_sources/chowdsp_sources.h>
#include "HoldTimeMinMaxParams.h"

// wave distance (0 to 20000), how far back you go, higher wave distance means more gentle wave
// wave section (wrap in an opengl wrapper, transposition slider is done this way)
// line that goes through it means tracking the playback position (synthesizer status in direct)
// undertow (0 to 9320), goes forward, dynamically shorten?

// key on reset checkbox
enum NostalgicComboBox {
    Note_Length = 1 << 0,
    Sync_KeyDown = 1 << 1,
    Sync_KeyUp = 1 << 2
};

// ********************************************************************************************* //
// ************************************  NostalgicParams  ************************************** //
// ********************************************************************************************* //

struct NostalgicParams : chowdsp::ParamHolder
{
    // gain slider params, for all gain-type knobs
    float rangeStart = -80.0f;
    float rangeEnd = 6.0f;
    float skewFactor = 2.0f;

    // using ParamPtrVariant = std::variant<chowdsp::FloatParameter*, chowdsp::ChoiceParameter*, chowdsp::BoolParameter*>;
    // std::vector<ParamPtrVariant> modulatableParams;

    // Adds the appropriate parameters to the Nostalgic Processor
    NostalgicParams(const juce::ValueTree& v) : chowdsp::ParamHolder ("nostalgic")
    {
        add (transpose,
            waveDistUndertowParams,
            outputSendGain,
            outputGain,
            noteLengthMultParam,
            beatsToSkipParam,
            clusterMinParam,
            clusterThreshParam,
            holdTimeMinMaxParams,
            transpositionUsesTuning,
            noteOnGain,
            keyOnReset,
            nostalgicTriggeredBy,
            reverseEnv,
            undertowEnv
            );

        // params that are audio-rate modulatable are added to vector of all continuously modulatable params
        // used in the NostalgicProcessor constructor
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
    // transposition slider
    TransposeParams transpose;

    // wave distance undertow slider
    WaveDistUndertowParams waveDistUndertowParams;

    // Hold time range slider
    HoldTimeMinMaxParams holdTimeMinMaxParams;

    // Reverse and Undertow ADSRs and params
    EnvParams reverseEnv{"Reverse", "Approaching Wave Envelope"};
    EnvParams undertowEnv{"Undertow", "Receding Wave Envelope"};

    // Transposition Uses Tuning param
    chowdsp::BoolParameter::Ptr transpositionUsesTuning {
        juce::ParameterID { "NostalgicUseTuning", 100 },
        "TranspositionUsesTuning",
        false
    };

    // key-on reset toggle
    chowdsp::BoolParameter::Ptr keyOnReset {
        juce::ParameterID { "keyOnReset", 100 },
        "key-on reset",
        false
    };

    // combo box
    chowdsp::EnumChoiceParameter<NostalgicComboBox>::Ptr nostalgicTriggeredBy {
        juce::ParameterID { "nostalgicTriggeredBy", 100 },
        "nostalgic triggered by",
        NostalgicComboBox::Note_Length,
        std::initializer_list<std::pair<char, char>> { { '_', ' ' }, { '1', '/' }, { '2', '-' } }
    };

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

    // Note Length Multiplier param
    chowdsp::FloatParameter::Ptr noteLengthMultParam {
        juce::ParameterID { "NoteLengthMultiplier", 100 },
        "NOTE LENGTH X",
        juce::NormalisableRange { 0.0f, 10.00f, 0.0f, skewFactor, false },
        1.0f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal,
        true
    };

    // Beats To Skip param
    chowdsp::FloatParameter::Ptr beatsToSkipParam {
        juce::ParameterID { "BeatsToSkip", 100 },
        "BEATS TO SKIP",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 10.f, 5.f, 1.f),
        0.0f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal,
        true
    };

    // Cluster Minimum param
    chowdsp::FloatParameter::Ptr clusterMinParam {
        juce::ParameterID { "ClusterMin", 100 },
        "CLUSTER MIN",
        chowdsp::ParamUtils::createNormalisableRange (1.0f, 10.f, 5.f, 1.f),
        1.0f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal,
        true
    };

    // Cluster Threshold param
    chowdsp::FloatParameter::Ptr clusterThreshParam {
        juce::ParameterID { "ClusterThresh", 100 },
        "CLUSTER THRESHOLD",
        juce::NormalisableRange { 0.0f, 1000.0f, 0.0f, skewFactor, false },
        150.0f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal,
        true
    };
    // passed to BKSynth, applies to noteOn msgs
    chowdsp::GainDBParameter::Ptr noteOnGain {
        juce::ParameterID { "noteOnGainNostalgic", 100 },
        "NoteOn Gain Scalar",
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
    std::tuple<std::atomic<float>, std::atomic<float>> sendLevels;
    std::tuple<std::atomic<float>, std::atomic<float>> inputLevels;

    bool synchronicConnected = false;

    /****************************************************************************************/
};

struct NostalgicNonParameterState : chowdsp::NonParamState
{
    NostalgicNonParameterState() {}
};

// ********************************************************************************************* //
// ************************************  NostalgicNoteStuff  ************************************** //
// ********************************************************************************************* //
struct NostalgicNoteData
{
    // Default constructor
    NostalgicNoteData() = default;

    // Delete copy operations (std::atomic can't be copied)
    NostalgicNoteData(const NostalgicNoteData&) = delete;
    NostalgicNoteData& operator=(const NostalgicNoteData&) = delete;

    // Allow move semantics
    NostalgicNoteData(NostalgicNoteData&& other) noexcept
    {
        // Copy non-atomic fields normally
        noteNumber = other.noteNumber;
        noteDurationSamples = other.noteDurationSamples;
        noteDurationMs = other.noteDurationMs;
        noteStart = other.noteStart;
        undertowDurationMs = other.undertowDurationMs;
        undertowDurationSamples = other.undertowDurationSamples;
        waveDistanceMs = other.waveDistanceMs;
        isReverse = other.isReverse;

        // Copy atomic values safely
        reverseTimerSamples.store(other.reverseTimerSamples.load());
        undertowTimerSamples.store(other.undertowTimerSamples.load());
    }

    NostalgicNoteData& operator=(NostalgicNoteData&& other) noexcept
    {
        if (this != &other)
        {
            noteNumber = other.noteNumber;
            noteDurationSamples = other.noteDurationSamples;
            noteDurationMs = other.noteDurationMs;
            noteStart = other.noteStart;
            undertowDurationMs = other.undertowDurationMs;
            undertowDurationSamples = other.undertowDurationSamples;
            waveDistanceMs = other.waveDistanceMs;
            isReverse = other.isReverse;

            reverseTimerSamples.store(other.reverseTimerSamples.load());
            undertowTimerSamples.store(other.undertowTimerSamples.load());
        }
        return *this;
    }
    int noteNumber;
    juce::uint64 noteDurationSamples = 0;
    double noteDurationMs = 0;
    double noteStart = 0;
    std::atomic<juce::uint64> reverseTimerSamples = 0;
    std::atomic<juce::uint64> undertowTimerSamples = 0;
    float undertowDurationMs = 0;
    float undertowDurationSamples = 0;
    float waveDistanceMs = 0;
    bool isReverse = true;
};

/*
 NostalgicNoteStuff is a class for containing a variety of information needed
 to create Nostalgic events. This is needed since the undertow note
 is created after the keys related to this note have been released, so
 we need to store that information to use when the undertow note is created.
 */
class NostalgicNoteStuff
{
public:
    NostalgicNoteStuff(int noteNumber) : notenumber(noteNumber)
    {
        resetReverseTimer();
        resetUndertowTimer();
    }

    ~NostalgicNoteStuff() {}

    void setNoteNumber(int newnote)                         { notenumber = newnote; }
    inline const int getNoteNumber() const noexcept         { return notenumber; }

    void incrementReverseTimer(juce::uint64 numsamples)           { reverseTimer += numsamples; }
    void incrementUndertowTimer(juce::uint64 numsamples)          { undertowTimer += numsamples; }

    void resetReverseTimer()                                { reverseTimer = 0; }
    void resetUndertowTimer()                               { undertowTimer = 0; }

    void setReverseStartPosition(juce::uint64 rsp)                        { reverseStartPosition = rsp; }
    inline const juce::uint64 getReverseStartPosition() const noexcept    { return reverseStartPosition; }

    void setUndertowStartPosition(juce::uint64 usp)                        { undertowStartPosition = usp; }
    inline const juce::uint64 getUndertowStartPosition() const noexcept    { return undertowStartPosition; }

    void setReverseTargetLength(juce::uint64 rtl)                         { reverseTargetLength = rtl; }
    void setUndertowTargetLength(juce::uint64 utl)                        { undertowTargetLength = utl; }
    inline const juce::uint64 getUndertowTargetLength() const noexcept    { return undertowTargetLength; }

    bool reverseTimerExceedsTarget()    { if(reverseTimer > reverseTargetLength) return true; else return false; }
    bool undertowTimerExceedsTarget()   { if(undertowTimer > undertowTargetLength) return true; else return false; }

    inline const juce::uint64 getReversePlayPosition()     { return (reverseStartPosition - reverseTimer); }
    inline const juce::uint64 getUndertowPlayPosition()    { return (undertowStartPosition + undertowTimer); }

    bool isActive() { if(reverseStartPosition < reverseTimer) return false; else return true; }
    juce::uint64 undertowTimer;

private:

    int notenumber;

    juce::uint64 reverseTimer;
    // juce::uint64 undertowTimer;

    juce::uint64 reverseStartPosition;
    juce::uint64 reversePosition;

    juce::uint64 undertowStartPosition;
    juce::uint64 undertowPosition;

    juce::uint64 reverseTargetLength;
    juce::uint64 undertowTargetLength;

    JUCE_LEAK_DETECTOR(NostalgicNoteStuff);
};


// ********************************************************************************************* //
// ************************************** NostalgicProcessor ************************************** //
// ********************************************************************************************* //

class NostalgicProcessor : public bitklavier::PluginBase<bitklavier::PreparationStateImpl<NostalgicParams, NostalgicNonParameterState>>,
                        public juce::ValueTree::Listener, public TuningListener
{
public:
    NostalgicProcessor (SynthBase& parent, const juce::ValueTree& v);
    ~NostalgicProcessor()
    {
        parent.getValueTree().removeListener(this);
        if(tuning !=nullptr) tuning->removeListener(this);
    }

    static std::unique_ptr<juce::AudioProcessor> create (SynthBase& parent, const juce::ValueTree& v)
    {
        return std::make_unique<NostalgicProcessor> (parent, v);
    }

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}

    // void setupModulationMappings();

    void processAudioBlock (juce::AudioBuffer<float>& buffer) override {};
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    void processBlockBypassed (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    void ProcessMIDIBlock(juce::MidiBuffer& inMidiMessages, juce::MidiBuffer& outMidiMessages, int numSamples);
    void updateNoteVisualization();
    void playReverseNote(NostalgicNoteData& noteData, juce::MidiBuffer& outMidiMessages);
    void handleNostalgicNote(int noteNumber, float clusterMin, juce::MidiBuffer& outMidiMessages);
    void processContinuousModulations(juce::AudioBuffer<float>& buffer);
    void updateMidiNoteTranspositions(int noteOnNumber);
    void updateAllMidiNoteTranspositions();
    void handleMidiTargetMessages(int channel);
    bool acceptsMidi() const override { return true; }
      void addSoundSet (
        juce::ReferenceCountedArray<BKSynthesiserSound > *s, // main samples
        juce::ReferenceCountedArray<BKSynthesiserSound > *h, // hammer samples
        juce::ReferenceCountedArray<BKSynthesiserSound > *r, // release samples
        juce::ReferenceCountedArray<BKSynthesiserSound > *p) // pedal samples
    {
        nostalgicSynth->addSoundSet (s);
    }

    void setSynchronic (SynchronicProcessor*) override;
    void setTuning (TuningProcessor*) override;
    void tuningStateInvalidated() override;

    /*
     * this is where we define the buses for audio in/out, including the param modulation channels
     *      the "discreteChannels" number is currently just by hand set based on the max that this particularly preparation could have
     *      so if you add new params, might need to increase that number
     */
    juce::AudioProcessor::BusesProperties nostalgicBusLayout()
    {
        // waveDistUndertowParams,
        // outputSendGain,
        // outputGain,
        // noteLengthMultParam,
        // beatsToSkipParam,
        // clusterMinParam,
        // clusterThreshParam,
        // holdTimeMinMaxParams,
        // transpositionUsesTuning,
        // noteOnGain,
        // keyOnReset,
        // nostalgicTriggeredBy,
        // reverseEnv,
        // undertowEnv

        return BusesProperties()
            .withOutput("Output", juce::AudioChannelSet::stereo(), true) // Main Output
            .withInput ("Input", juce::AudioChannelSet::stereo(), false)  // Main Input (not used here)

            /**
             * IMPORTANT: set discreteChannels below equal to the number of params you want to continuously modulate!!
             *              for nostalgic, we have 14:
             *                  - the ADSR params x 2 (8): attackParam, decayParam, sustainParam, releaseParam, and
             *                  - the gain params: OutputSendParam, outputGain,
             *                  - other params: noteLengthMultParam, beatsToSkipParam, clusterMinParam, clusterThreshParam
             *                  - what about waveDistUndertowParams?
             */
             /**
              * todo: check the number of discrete channels to match needs here
              */
            .withInput ("Modulation", juce::AudioChannelSet::discreteChannels (14), true) // Mod inputs; numChannels for the number of mods we want to enable
            .withOutput("Modulation", juce::AudioChannelSet::mono(),false)  // Modulation send channel; disabled for all but Modulation preps!
            .withOutput("Send",juce::AudioChannelSet::stereo(),true);       // Send channel (right outputs)
    }
    bool isBusesLayoutSupported (const juce::AudioProcessor::BusesLayout& layouts) const override;

    bool hasEditor() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }

    void valueTreePropertyChanged(juce::ValueTree &t, const juce::Identifier &property) {

        if (t == v && property == IDs::soundset) {
            loadSamples();
            return;
        }
        if (!v.getProperty(IDs::soundset).equals(IDs::syncglobal.toString()))
            return;
        if (property == IDs::soundset && t == parent.getValueTree()) {
            juce::String a = t.getProperty(IDs::soundset, "");
            addSoundSet((*parent.getSamples())[a],
                      nullptr,
                      nullptr,
                        nullptr);
        }
    }

    void loadSamples() {
        juce::String soundset = v.getProperty(IDs::soundset, IDs::syncglobal.toString());
        if (soundset == IDs::syncglobal.toString()) {
            //if global sync read soundset from global valuetree
            soundset = parent.getValueTree().getProperty(IDs::soundset, "");

            addSoundSet((*parent.getSamples())[soundset],
            nullptr,
              nullptr,
                nullptr);
        }else {
            //otherwise set the piano
            addSoundSet((*parent.getSamples())[soundset],
            nullptr,
             nullptr,
               nullptr);
        }
    }

    bool holdCheck(int noteNumber);

    //move timers forward by blocksize
    void incrementTimers(int numSamples);

    juce::Array<juce::uint64> holdTimers;
    /*
     * noteOnSpecMap
     * - key      => midiNoteNumber
     * - value    => specs for that key (start time, direction, loop mode)
     *
     * needed in particular for backwards-playing notes
     */
    std::array<NoteOnSpec, MaxMidiNotes> noteOnSpecMap;
    juce::Array<float> updatedTransps;
    juce::Array<int> keysDepressed;   //current keys that are depressed
    juce::Array<juce::uint8> velocities;
    juce::Array<float> noteLengthTimers;
    juce::Array<NostalgicNoteData> reverseTimers;
    juce::Array<float> undertowTimers;
    juce::Array<NostalgicNoteData> clusterNotes;
    float clusterTimer;
    int clusterCount;
    bool inCluster = false;

private:
    bool doDefault = false;
    bool doClear = false;

    std::unique_ptr<BKSynthesiser> nostalgicSynth;
    BKSynthesizerState lastSynthState;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NostalgicProcessor)
};
