/*
==============================================================================

Synchronic.h
      Created: 22 Nov 2016 3:46:45pm
  Author:  Michael R Mulshine and Dan Trueman

  Completely rewritten by Dan Trueman, 2025

==============================================================================
*/

#ifndef BITKLAVIER2_SYNCHRONICPROCESSOR_H
#define BITKLAVIER2_SYNCHRONICPROCESSOR_H

#pragma once

#include <PreparationStateImpl.h>
#include <chowdsp_plugin_base/chowdsp_plugin_base.h>
#include <chowdsp_plugin_state/chowdsp_plugin_state.h>
#include <chowdsp_plugin_utils/chowdsp_plugin_utils.h>
#include <chowdsp_serialization/chowdsp_serialization.h>
#include <chowdsp_sources/chowdsp_sources.h>
#include <utility>
#include "ClusterMinMaxParams.h"
#include "EnvParams.h"
#include "EnvelopeSequenceParams.h"
#include "HoldTimeMinMaxParams.h"
#include "Identifiers.h"
#include "MultiSliderState.h"
#include "MultiSlider2DState.h"
#include "PluginBase.h"
#include "Synthesiser/BKSynthesiser.h"
#include "TempoProcessor.h"
#include "TransposeParams.h"
#include "TuningProcessor.h"
#include "buffer_debugger.h"
#include "target_types.h"
#include "utils.h"

enum SynchronicPulseTriggerType
{
    First_NoteOn = 1 << 0,
    Any_NoteOn = 1 << 1,
    First_NoteOff = 1 << 2,
    Any_NoteOff = 1 << 3,
    Last_NoteOff = 1 << 4,
};

enum SynchronicClusterTriggerType
{
    Key_On = 1 << 0,
    Key_Off = 1 << 1,
};

// ********************************************************************************************* //
// ************************************  SynchronicParams  ************************************* //
// ********************************************************************************************* //

struct SynchronicParams : chowdsp::ParamHolder
{
    // gain slider params, for all gain-type knobs
    float rangeStart = -80.0f;
    float rangeEnd = 6.0f;
    float skewFactor = 2.0f;

    // we're going to hard-wire the number of envelopes that can be sequenced to 12, like the original bK
    static constexpr int numEnvelopes = 12;

    //    using ParamPtrVariant = std::variant<chowdsp::FloatParameter*, chowdsp::ChoiceParameter*, chowdsp::BoolParameter*>;
    //    std::vector<ParamPtrVariant> modulatableParams;

    // Adds the appropriate parameters to the Synchronic Processor
    SynchronicParams(const juce::ValueTree& v) : chowdsp::ParamHolder("synchronic")
    {
        add(
            numPulses,
            numLayers,           // how many independent synchronic layers can we have simultaneously? usually only 1, but...
            clusterThickness,    // SynchronicClusterCap in the old bK; max number of notes in a cluster (different than cluster min/max below)
            clusterThreshold,    // time (ms) between notes (as played) for them to be part of a cluster
            clusterMinMaxParams, // min/max number of played notes to launch a pulse
            holdTimeMinMaxParams,
            pulseTriggeredBy,
            determinesCluster,
            skipFirst,
            transpositionUsesTuning,
            env,
            envelopeSequence,
            outputSendGain,
            outputGain,
            noteOnGain,
            updateUIState);

        // params that are audio-rate modulatable are added to vector of all continuously modulatable params
        doForAllParameters([this](auto& param, size_t)
        {
            // if (auto* sliderParam = dynamic_cast<chowdsp::ChoiceParameter*> (&param))
            //     if (sliderParam->supportsMonophonicModulation())
            //         modulatableParams.push_back ( sliderParam);
            //
            // if (auto* sliderParam = dynamic_cast<chowdsp::BoolParameter*> (&param))
            //     if (sliderParam->supportsMonophonicModulation())
            //         modulatableParams.push_back ( sliderParam);

            if (auto* sliderParam = dynamic_cast<chowdsp::FloatParameter*> (&param))
                if (sliderParam->supportsMonophonicModulation())
                    modulatableParams.push_back ( sliderParam); });
        }

    // primary multislider params
    MultiSlider2DState transpositions {"transpositions"};
    MultiSliderState accents {"accents"};
    MultiSliderState sustainLengthMultipliers{"SustainLengthMultipliers"};
    MultiSliderState beatLengthMultipliers{"beatLengthMultipliers"};

    /*
     * for keeping track of the current multislider lengths
     * being used by blendrónic, so we can update the UI accordingly
     */
    std::atomic<int> transpositionsCurrent = 0;
    std::atomic<int> accentsCurrent = 0;
    std::atomic<int> sustainLengthMultipliersCurrent = 0;
    std::atomic<int> beatLengthMultipliersCurrent = 0;
    std::atomic<int> envelopesCurrent = 0;

    /*
     * the state of all the adsrs, for the row of 12 sequenced adsrs
     *  similar to MultiSliderState for the multisliders
     */

    chowdsp::EnumChoiceParameter<SynchronicPulseTriggerType>::Ptr pulseTriggeredBy{
        juce::ParameterID{"pulseTriggeredBy", 100},
        "pulse triggered by",
        SynchronicPulseTriggerType::First_NoteOn,
        std::initializer_list<std::pair<char, char>>{{'_', ' '}, {'1', '/'}, {'2', '-'}, {'3', '\''}, {'4', '#'}, {'5', 'b'}}};

    chowdsp::EnumChoiceParameter<SynchronicClusterTriggerType>::Ptr determinesCluster{
        juce::ParameterID{"determinesCluster", 100},
        "determines cluster",
        SynchronicClusterTriggerType::Key_On,
        std::initializer_list<std::pair<char, char>>{{'_', ' '}, {'1', '/'}, {'2', '-'}, {'3', '\''}, {'4', '#'}, {'5', 'b'}}};

    chowdsp::BoolParameter::Ptr skipFirst{
        juce::ParameterID{"skipFirst", 100},
        "skip first",
        true};

    // Transposition Uses Tuning param
    chowdsp::BoolParameter::Ptr transpositionUsesTuning{
        juce::ParameterID{"UseTuning", 100},
        "TranspositionUsesTuning",
        false};

    // Gain for output send (for blendronic, VSTs, etc...)
    chowdsp::GainDBParameter::Ptr outputSendGain{
        juce::ParameterID{"Send", 100},
        "Send",
        juce::NormalisableRange{rangeStart, rangeEnd, 0.0f, skewFactor, false},
        0.0f,
        true};

    // for the output gain slider, final gain stage for this prep (meter slider on right side of prep)
    chowdsp::GainDBParameter::Ptr outputGain{
        juce::ParameterID{"OutputGain", 100},
        "Output Gain",
        juce::NormalisableRange{rangeStart, rangeEnd, 0.0f, skewFactor, false},
        0.0f,
        true};

    // used internally to notify UI to redraw sliders
    chowdsp::BoolParameter::Ptr updateUIState{
        juce::ParameterID{"updateUIState", 100},
        "updateUIState",
        false,
    };

    chowdsp::FloatParameter::Ptr numPulses{
        juce::ParameterID{"numPulses", 100},
        "PULSES",
        chowdsp::ParamUtils::createNormalisableRange(1.0f, 100.f, 50.f, 1.f),
        20.f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal,
        true};

    chowdsp::FloatParameter::Ptr numLayers{
        juce::ParameterID{"numLayers", 100},
        "LAYERS",
        chowdsp::ParamUtils::createNormalisableRange(1.0f, 10.f, 5.f, 1.f),
        1.f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal,
        true};

    chowdsp::FloatParameter::Ptr clusterThickness{
        juce::ParameterID{"cThickness", 100},
        "CLUSTER THICKNESS",
        chowdsp::ParamUtils::createNormalisableRange(1.0f, 20.f, 10.f, 1.f),
        8.f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal,
        true};

    /*
     * NOTE: if the ParameterIDs are too similar, it seems we can run into trouble
     *      - at first, these two params had IDs "clusterThickness" and "clusterThreshold"
     *          and then when they were modulated or reset, their knobs seemed to interact, jitter, etc...
     *          making the IDs more different solved it
     *      - presumably there is some string comparing going on somewhere that stops after a certain number of characters
     */
    chowdsp::TimeMsParameter::Ptr clusterThreshold{
        juce::ParameterID{"cMin", 100},
        "CLUSTER THRESHOLD",
        chowdsp::ParamUtils::createNormalisableRange(20.0f, 2000.f, 1000.f),
        500.f,
        true};

    // passed to BKSynth, applies to noteOn msgs
    chowdsp::GainDBParameter::Ptr noteOnGain{
        juce::ParameterID{"noteOnGain", 100},
        "NoteOn Gain Scalar",
        juce::NormalisableRange{rangeStart, rangeEnd, 0.0f, skewFactor, false},
        0.0f,
        true};

    // the two min/max params
    ClusterMinMaxParams clusterMinMaxParams;
    HoldTimeMinMaxParams holdTimeMinMaxParams;

    // placeholder for the current ADSR, to pass to the synths
    EnvParams env;

    // the 12 individual env params
    EnvelopeSequenceParams envelopeSequence;

    bool isEnvelopeActive(int which)
    {
        for (auto& _ep : *envelopeSequence.getBoolParams())
        {
            //"envelope10"
            if (_ep->getParameterID() == "envelope" + juce::String(which))
            {
                return *_ep;
            }
        }
    }

    /*
     * serializers are used for more complex params, called on save and load
     *  - here we need these for all the multisliders
     */
    /* Custom serializer */
    template <typename Serializer>
    static typename Serializer::SerializedType serialize(const SynchronicParams& paramHolder);

    /* Custom deserializer */
    template <typename Serializer>
    static void deserialize(typename Serializer::DeserializedType deserial, SynchronicParams& paramHolder);

    /*
     * processStateChanges() is used to handle state change modifications, including resets. should be called every block
     *
     * state changes are NOT audio-rate/continuous; those are handled by "modulatableParams" in the parameter definitions
     * examples: TranspParams or velocityMinMaxParams
     *
     * 'stateChanges.change' state will have size() != 0 after a mod or reset is triggered. each change
     * is handled here, and then it is cleared until another mod or reset is triggered
     *
     * in this specific case, if there is a changeState in stateChanges (which will only happen when
     * a mod is triggered) we read through the valueTree and update all the modded transpositions (t0, t1, etc...)
     */
    void processStateChanges() override
    {
        clusterMinMaxParams.processStateChanges();
        holdTimeMinMaxParams.processStateChanges();

        transpositions.processStateChanges();
        accents.processStateChanges();
        sustainLengthMultipliers.processStateChanges();
        beatLengthMultipliers.processStateChanges();

        // signal the UI to redraw the sliders
        if (transpositions.updateUI == true || accents.updateUI == true || sustainLengthMultipliers.updateUI == true || beatLengthMultipliers.updateUI == true)
        {
            /*
             * need to actually change the value for the listener to get the message
             * we're just using updateUIState as a way to notify the UI, and its actual value doesn't matter
             * so we switch it everything we one of the sliders gets modded.
             */
            if (*updateUIState)
                updateUIState->setValueNotifyingHost(false);
            else
                updateUIState->setValueNotifyingHost(true);
        }
    }

    /*
     * for storing outputLevels of this preparation for display
     *  because we are using an OpenGL slider for the level meter, we don't use the chowdsp params for this
     *      we simply update this in the processBlock() call
     *      and then the level meter will update its values during the OpenGL cycle
     */
    std::tuple<std::atomic<float>, std::atomic<float>> outputLevels;
    std::tuple<std::atomic<float>, std::atomic<float>> sendLevels;
    // std::tuple<std::atomic<float>, std::atomic<float>> inputLevels;
};

struct SynchronicNonParameterState : chowdsp::NonParamState
{
    SynchronicNonParameterState()
    {
        addStateValues({&prepPoint});
    }

    chowdsp::StateValue<juce::Point<int>> prepPoint{"prep_point", {300, 500}};
};

// ********************************************************************************************* //
// ************************************  SynchronicCluster  ************************************ //
// ********************************************************************************************* //

/*
 This class enables layers of Synchronic pulses by
 maintaining a set of counters moving through all the
 primary multi-parameters (accents, transpositions, etc...)
 and by updating timers for each Synchronic independently
 */
class SynchronicCluster
{
   public:
    SynchronicCluster(SynchronicParams* inparams) : _sparams(inparams)
    {
        phasor = 0;
        envelopeCounter = 0;
        shouldPlay = false;
    }

    ~SynchronicCluster() {}

    /*
     * increment the timing phasor
     *  - called every block, so numSamples is always the blocksize
     */
    inline void incrementPhasor(int numSamples)
    {
        phasor += numSamples;
    }

    /*
     * we increment all the parameter counters here
     * we also decrement the timing phasor by the amount of time to the next beat
     * - typically called every beat, so the phasor reset is akin to the counter increments
     */
    inline void step(juce::uint64 numSamplesBeat)
    {
        // set the phasor back by the number of samples to the next beat
        // phasor -= numSamplesBeat;
        // phasor -= numSamplesBeat;
        phasor = 0; // the decrement doesn't make sense to me, why not just set the phasor to 0?

        // increment all the counters
        if (++lengthMultiplierCounter >= _sparams->sustainLengthMultipliers.sliderVals_size)
            lengthMultiplierCounter = 0;
        if (++accentMultiplierCounter >= _sparams->accents.sliderVals_size)
            accentMultiplierCounter = 0;
        if (++transpCounter >= _sparams->transpositions.sliderVals_size)
            transpCounter = 0;
        if (++envelopeCounter >= _sparams->numEnvelopes)
            envelopeCounter = 0;

        // skip the inactive envelopes
        while (!_sparams->isEnvelopeActive(envelopeCounter)) // skip untoggled envelopes
        {
            //DBG("looking for next active envelope, envelopeCounter = " << envelopeCounter);
            envelopeCounter++;
            if (envelopeCounter >= _sparams->numEnvelopes)
                envelopeCounter = 0;
        }
    }

    inline void postStep()
    {
        /*
         * the reason we do this separately from step() is because the length of the beats
         * is what determines when the next step() needs to happen, so we increment its counter
         * at the end of each cycle, before we pass time until the next beat
         */

        /* this messy conditional....
         *      in short: we always increment the beat multiplier counter UNLESS we are:
         *                  - on the first beat
         *                  - AND are in a noteOff trigger mode
         *                  - AND are NOT skipping the first beat
         *
         *      this is a special case, but should behave as expected
         */
        auto sMode = _sparams->pulseTriggeredBy->get();
        if (beatCounter > 0 || (sMode == Any_NoteOn || sMode == First_NoteOn) || *_sparams->skipFirst)
        {
            if (++beatMultiplierCounter >= _sparams->beatLengthMultipliers.sliderVals_size)
                beatMultiplierCounter = 0;
        }

        if (++beatCounter > static_cast<int>(*_sparams->numPulses))
        {
            DBG("beatCounter = " << beatCounter << " numPulses = " << static_cast<int>(*_sparams->numPulses));
            shouldPlay = false;
        }
    }

    inline void resetPatternPhase()
    {
        beatMultiplierCounter = 0;
        lengthMultiplierCounter = 0;
        accentMultiplierCounter = 0;
        transpCounter = 0;
        envelopeCounter = 0;
        beatCounter = 0;
    }

    inline void reset()
    {
        DBG("reset called");
        envelopeCounter = 0;
        shouldPlay = false;
        resetPatternPhase();
        cluster.clearQuick();
    }

    inline juce::Array<int> getCluster() { return cluster; }
    inline void setCluster(juce::Array<int> c) { cluster = c; }
    inline void setBeatPhasor(juce::uint64 c)
    {
        phasor = c;
        DBG("resetting beat phasor");
    }
    inline const juce::uint64 getPhasor(void) const noexcept { return phasor; }

    inline void addNote(int note)
    {
        DBG("synchronic cluster adding note: " + juce::String(note));
        cluster.insert(0, note);
    }

    inline bool containsNote(int note)
    {
        return cluster.contains(note);
    }

    inline void setShouldPlay(bool play)
    {
        shouldPlay = play;
    }

    inline bool getShouldPlay(void)
    {
        return shouldPlay;
    }

    int beatCounter; // beat (or pulse) counter; max set by users -- sNumBeats

    // parameter field counters
    int beatMultiplierCounter;   // beat length (time between beats) multipliers
    int accentMultiplierCounter; // accent multipliers
    int lengthMultiplierCounter; // note length (sounding length) multipliers (multiples of 50ms, at least for now)
    int transpCounter;           // transposition offsets
    int envelopeCounter;

    bool doPatternSync = false;

   private:
    SynchronicParams* _sparams;

    juce::Array<int> cluster;
    juce::uint64 phasor;

    bool shouldPlay;

    JUCE_LEAK_DETECTOR(SynchronicCluster);
};

// ********************************************************************************************* //
// ************************************ SynchronicProcessor ************************************ //
// ********************************************************************************************* //

#define MAX_CLUSTERS 10
class SynchronicProcessor : public bitklavier::PluginBase<bitklavier::PreparationStateImpl<SynchronicParams, SynchronicNonParameterState>>,
                            public juce::ValueTree::Listener, public TuningListener
{
   public:
    SynchronicProcessor(SynthBase& parent, const juce::ValueTree& v, juce::UndoManager*);
    ~SynchronicProcessor()
    {
        parent.getValueTree().removeListener(this);
        if(tuning !=nullptr) tuning->removeListener(this);
    }

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processAudioBlock(juce::AudioBuffer<float>& buffer) override {};
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    void processBlockBypassed(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    void ProcessMIDIBlock(juce::MidiBuffer& inMidiMessages, juce::MidiBuffer& outMidiMessages, int numSamples);
    bool acceptsMidi() const override { return true; }

//    void addSoundSet(juce::ReferenceCountedArray<BKSamplerSound<juce::AudioFormatReader>>* s)
//    {
//        synchronicSynth->addSoundSet(s);
//    }

    void addSoundSet (
        juce::ReferenceCountedArray<BKSynthesiserSound > *s, // main samples
        juce::ReferenceCountedArray<BKSynthesiserSound > *h, // hammer samples
        juce::ReferenceCountedArray<BKSynthesiserSound > *r, // release samples
        juce::ReferenceCountedArray<BKSynthesiserSound > *p) // pedal samples
    {
        synchronicSynth->addSoundSet (s);
    }

    void setA4Frequency(double newA4)
    {
        synchronicSynth->setA4Frequency(newA4);
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
//        if (property == IDs::soundset)
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
//        }
//        else {
//            //otherwise set the piano
//            addSoundSet(&(*parent.getSamples())[soundset]);
//        }
//    }

    void loadSamples() {
        juce::String soundset = v.getProperty(IDs::soundset, IDs::syncglobal.toString());
        if (soundset == IDs::syncglobal.toString()) {
            //if global sync read soundset from global valuetree
            soundset = parent.getValueTree().getProperty(IDs::soundset, "");
            auto* samples = parent.getSamples();
            addSoundSet( samples->contains(soundset) ? (*samples)[soundset] : nullptr,
             nullptr,
             nullptr,
             nullptr);
        }else {
            //otherwise set the piano
            auto* samples = parent.getSamples();
            addSoundSet(samples->contains(soundset) ? (*samples)[soundset] : nullptr,
             nullptr,
             nullptr,
             nullptr);
        }
    }

    void setTuning(TuningProcessor*) override;
    void tuningStateInvalidated() override;

    float getBeatThresholdSeconds()
    {
        if (tempo != nullptr)
            return 60.f /
                (*tempo->getState().params.tempoParam *
                *tempo->getState().params.subdivisionsParam *
                tempo->getGlobalTempoMultiplier());
        else
            return 0.5; // 120bpm by default
    }

    /*
     * this is where we define the buses for audio in/out, including the param modulation channels
     *      the "discreteChannels" number is currently just by hand set based on the max that this particularly preparation could have
     *      so if you add new params, might need to increase that number
     */
    juce::AudioProcessor::BusesProperties synchronicBusLayout()
    {
        return BusesProperties()
            .withOutput("Output", juce::AudioChannelSet::stereo(), true) // Main Output
            .withInput("Input", juce::AudioChannelSet::stereo(), false)  // Main Input (not used here)

            /**
             * IMPORTANT: set discreteChannels below equal to the number of params you want to continuously modulate!!
             *              for synchronic, we have 10:
             *                  - the ADSR params: attackParam, decayParam, sustainParam, releaseParam, and
             *                  - the main params: numPulses, numLayers, clusterThickness, clusterThreshold, OutputSendParam, outputGain,
             */
            .withInput("Modulation", juce::AudioChannelSet::discreteChannels(10 * 2), true) // Mod inputs; numChannels for the number of mods we want to enable
            .withOutput("Modulation", juce::AudioChannelSet::mono(), false)             // Modulation send channel; disabled for all but Modulation preps!
            .withOutput("Send", juce::AudioChannelSet::stereo(), true);                 // Send channel (right outputs)
    }

    bool isBusesLayoutSupported(const juce::AudioProcessor::BusesLayout& layouts) const override;
    bool hasEditor() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }

    /*
     * Synchronic Functions
     */
    void keyPressed(int noteNumber, int velocity, int channel);
    void keyReleased(int noteNumber, int channel);
    void handleMidiTargetMessages(int channel);
    bool updateCurrentCluster();
    float getTimeToBeatMS(float beatsToSkip);
    void removeOldestCluster();
    void removeNewestCluster();
    void rotateClusters();

    bool holdCheck(int noteNumber);

    /*
     * Synchronic Params
     */
    bool inCluster;
    bool nextOffIsFirst;

    // temporary, replace with Tempo info
    float tempoTemp = 120.;

    juce::uint64 thresholdSamples;
    juce::uint64 clusterThresholdTimer;
    juce::uint64 syncThresholdTimer;
    juce::Array<juce::uint64> holdTimers;

    /*
     * the `clusters` array holds clusters to manage the 'layers' feature in bK
     *
     * every time we start a new cluster, we move to the next item in this array
     * and turn off a previous cluster, as set by numLayers
     *
     * so, at first, no clusters are playing, so the first cluster will be item 0 in this array
     * then, the next time a cluster is triggered, it will use item 1 in this array
     * and turn off (set shouldPlay = false) the item (1 - numLayers) in this array
     *  - if numLayers is 1, then it will turn off item 0 and we will only have one cluster playing (default behavior)
     *  - if numLayers is 2, then cluster 0 will continue playing, and it will turn off the item -1 (mod the size of the array, so the last element)
     *          - doesn't matter if that layer isn't playing....
     *  - etc... for up to numLayers = MAX_CLUSTERS (or MAX_CLUSTERS - 1?)
     *
     * essentially a circular voice-stealing buffer
     *
     */

    std::array<std::unique_ptr<SynchronicCluster>, MAX_CLUSTERS> clusterLayers;
    int currentLayerIndex = 0; // which cluster is most recent

    juce::Array<int> keysDepressed; // current keys that are depressed
    juce::Array<int> syncKeysDepressed;
    juce::Array<int> clusterKeysDepressed;
    juce::Array<int> patternSyncKeysDepressed;
    juce::Array<juce::uint8> clusterVelocities; // NOT scaled 0-1, as with the old bK

    juce::uint64 numSamplesBeat = 0;   // = beatThresholdSamples * beatMultiplier
    juce::uint64 beatThresholdSamples; // # samples in a beat, as set by tempo

    /*
     * noteOnSpecMap
     * - key      => midiNoteNumber
     * - value    => specs for that key (start time, direction, loop mode)
     *
     * needed in particular for backwards-playing notes and setting note durations
     */
    std::array<NoteOnSpec, MaxMidiNotes> noteOnSpecMap;

   private:
    juce::ScopedPointer<BufferDebugger> bufferDebugger;

    bool doCluster = false;   // primary Synchronic mode
    bool doBeatSync = false;  // resetting beat phase
    bool doAddNotes = false;  // adding notes to cluster
    bool doPausePlay = false; // targeting pause/play
    bool doClear = false;
    bool doDeleteOldest = false;
    bool doDeleteNewest = false;
    bool doRotate = false;

    std::unique_ptr<BKSynthesiser> synchronicSynth;

    juce::Array<int> slimCluster; // cluster without repetitions
    juce::Array<int> clusterNotes;
    bool checkClusterMinMax(int clusterNotesSize);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SynchronicProcessor)
};

#endif // BITKLAVIER2_SYNCHRONICPROCESSOR_H
