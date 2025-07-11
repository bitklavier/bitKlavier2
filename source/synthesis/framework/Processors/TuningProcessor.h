//
// Created by Joshua Warner on 6/27/24.
//

#ifndef BITKLAVIER2_TUNINGPROCESSOR_H
#define BITKLAVIER2_TUNINGPROCESSOR_H

#pragma once

#include "PluginBase.h"
#include "PreparationStateImpl.h"
#include "array_to_string.h"
#include "tuning_systems.h"
#include "utils.h"
#include "TuningUtils.h"
#include "synth_base.h"
#include "SemitoneWidthParams.h"
#include "AdaptiveTuningParams.h"
#include "SpringTuningParams.h"
#include "SpringTuning/SpringTuning.h"
#include <chowdsp_plugin_base/chowdsp_plugin_base.h>
#include <chowdsp_plugin_state/chowdsp_plugin_state.h>
#include <chowdsp_plugin_utils/chowdsp_plugin_utils.h>
#include <chowdsp_sources/chowdsp_sources.h>

/**
 * TuningState is the primary struct that is shared around to get/set tuning information
 */
//class SpringTuning;
struct TuningState : bitklavier::StateChangeableParameter
{
    void setKeyOffset (int midiNoteNumber, float val);
    void setCircularKeyOffset (int midiNoteNumber, float val);
    void setKeyOffset (int midiNoteNumber, float val, bool circular);
    //static std::array<float, 12> rotateValuesByFundamental (std::array<float, 12> vals, int fundamental);
    void processStateChanges() override;

    void setFundamental (int fund);
    int getFundamental() { return fundamental->getIndex(); }
    int getOldFundamental() { return oldFundamental; }
    void setOldFundamental(int newold) { oldFundamental = newold; }
    int getSemitoneWidthFundamental();
    double getSemitoneWidth();
    double getSemitoneWidthOffsetForMidiNote(double midiNoteNumber);
    int getClosestKey(int noteNum, float transp, bool tuneTranspositions);

    double getOverallOffset();
    double getTargetFrequency (int currentlyPlayingNote, double currentTransposition, bool tuneTranspositions);
    double getStaticTargetFrequency (int currentlyPlayingNote, double currentTransposition, bool tuneTranspositions);
    void updateLastFrequency(double lastFreq);

    juce::MidiKeyboardState keyboardState;
    std::array<float, 128> absoluteTuningOffset = { 0.f };
    std::array<float, 12> circularTuningOffset = { 0.f };

    int oldFundamental = 0;
    float A4frequency = 440.;       // set this in gallery preferences
    double lastFrequencyHz = 440.;  // frequency of last getTargetFrequency returned
    double lastIntervalCents = 0.;  // difference between pitch of last two notes returned, in cents
    double lastMidiNote = 69.;      // pitch of last frequency returned

    // ****************************************** PARAMETERS ***************************************** //

    /*
     * some of the params below are not audio-rate modulatable (like tuningSystem, which are arrays),
     * so will need to be handled in a TuningState::processStateChanges() call, called every block
     */

    /**
     * tuningType = which tuning type to use:
     *  - static
     *  - adaptive
     *  - adaptive anchored
     *  - spring
     */
    chowdsp::EnumChoiceParameter<TuningType>::Ptr tuningType {
        juce::ParameterID { "tuningType", 100 },
        "Tuning Type",
        TuningType::Static,
        std::initializer_list<std::pair<char, char>> { { '_', ' ' }, { '1', '/' }, { '2', '-' }, { '3', '\'' }, { '4', '#' }, { '5', 'b' } }
    };

    /**
     * tuningSystem = 12tet system for circular tuning (ET, partial, etc...)
     */
    chowdsp::EnumChoiceParameter<TuningSystem>::Ptr tuningSystem {
        juce::ParameterID { "tuningSystem", 100 },
        "Tuning System",
        TuningSystem::Equal_Temperament,
        std::initializer_list<std::pair<char, char>> { { '_', ' ' }, { '1', '/' }, { '2', '-' }, { '3', '\'' } }
    };

    /**
     * fundamental = fundamental for tuningSystem
     */
    chowdsp::EnumChoiceParameter<PitchClass>::Ptr fundamental {
        juce::ParameterID { "fundamental", 100 },
        "PitchClass",
        PitchClass::C,
        std::initializer_list<std::pair<char, char>> { { '_', ' ' }, { '1', '/' }, { '2', '-' }, { '3', '\'' }, { '4', '#' }, { '5', 'b' } }
    };

    /**
     * offset of tuning system (cents)
     */
    chowdsp::FloatParameter::Ptr offSet {
        juce::ParameterID { "offSet", 100 },
        "Offset",
        chowdsp::ParamUtils::createNormalisableRange (-100.0f, 100.0f, 0.0f),
        0.0f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal,
        true
    };

    /**
     * for keeping track of the tuning of the last note played
     */
    chowdsp::FloatParameter::Ptr lastNote {
        juce::ParameterID { "lastNote", 100 },
        "Last Note",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 128.0f, 64.0f),
        60.0f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };

    AdaptiveTuningParams adaptiveParams;
    SemitoneWidthParams semitoneWidthParams;
    SpringTuningParams springTuningParams;
    std::unique_ptr<SpringTuning> springTuner;

    // ****************************************** OTHER VARS ***************************************** //

    int getAdaptiveClusterTimer();
    void keyReleased(int noteNumber);
    void keyPressed(int noteNumber);
    float adaptiveCalculateRatio(const int midiNoteNumber) const;
    float adaptiveCalculate(int midiNoteNumber);
    void adaptiveReset();
    void updateAdaptiveFundamentalValue(int newFund);

    float getGlobalTuningReference() const { return A4frequency; };
    TuningType getTuningType() const { return tuningType->get(); }
    //std::array<float, 12> getTuningSystem(TuningSystem which) const { return tuningMap[TuningSystem(which)].second; } // broken, see getOffsetsFromTuningSystem()

    inline const bool getAdaptiveInversional() const noexcept { return adaptiveParams.tAdaptiveInversional->get(); }
    inline const int getAdaptiveClusterThresh() const noexcept { return adaptiveParams.tAdaptiveClusterThresh->get(); }
    inline const int getAdaptiveHistory() const noexcept { return adaptiveParams.tAdaptiveHistory->get(); }
    inline const int getAdaptiveAnchorFundamental() const noexcept { return (int)adaptiveParams.tAdaptiveAnchorFundamental->get(); }
    inline const TuningSystem getAdaptiveIntervalScale() const noexcept { return adaptiveParams.tAdaptiveIntervalScale->get(); }
    inline const TuningSystem getAdaptiveAnchorScale() const noexcept { return adaptiveParams.tAdaptiveAnchorScale->get(); }
    float intervalToRatio(float interval) const;

    /*
     * Adaptive vars
     */
    int adaptiveFundamentalNote = 60;
    float adaptiveFundamentalFreq = mtof(adaptiveFundamentalNote);
    int adaptiveHistoryCounter = 0;
    float clusterTimeMS = 0.;
    double lastAdaptiveTarget = 440.;

    std::atomic<bool> setFromAudioThread;
};

/**
 * todo: not totally clear to me we need this wrapper around TuningState
 */
struct TuningParams : chowdsp::ParamHolder
{
    // Adds the appropriate parameters to the Tuning Processor
    TuningParams() : chowdsp::ParamHolder ("tuning")
    {
        add (tuningState.tuningSystem,
            tuningState.fundamental,
            tuningState.tuningType,
            tuningState.semitoneWidthParams,
            tuningState.offSet,
            tuningState.lastNote,
            tuningState.adaptiveParams,
            tuningState.springTuningParams);

        tuningState.springTuner = std::make_unique<SpringTuning>(tuningState.springTuningParams);
    }

    /**
     * todo:
     * params to add:
     * - scala functionality
     * - MTS
     */

    /* this contains the current state of the tuning system, with helper functions */
    TuningState tuningState;

    /*
     * serializers are used for more complex params
     *      - here we need arrays and indexed arrays for circular and absolute tunings, for instance
     */
    /* Custom serializer */
    template <typename Serializer>
    static typename Serializer::SerializedType serialize (const TuningParams& paramHolder);

    /* Custom deserializer */
    template <typename Serializer>
    static void deserialize (typename Serializer::DeserializedType deserial, TuningParams& paramHolder);
};

struct TuningNonParameterState : chowdsp::NonParamState
{
    TuningNonParameterState()
    {
    }
};

class TuningProcessor : public bitklavier::PluginBase<bitklavier::PreparationStateImpl<TuningParams, TuningNonParameterState>>
{
public:
    TuningProcessor (SynthBase& parent, const juce::ValueTree& v);

    static std::unique_ptr<juce::AudioProcessor> create (SynthBase& parent, const juce::ValueTree& v) {
        return std::make_unique<TuningProcessor> (parent, v); }
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processAudioBlock (juce::AudioBuffer<float>& buffer) override {};
    bool acceptsMidi() const override { return true; }
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    void handleMidiEvent (const juce::MidiMessage& m);
    void noteOn (int midiChannel,int midiNoteNumber,float velocity);
    void noteOff (int midiChannel,int midiNoteNumber,float velocity);

    void incrementClusterTime(long numSamples) { state.params.tuningState.clusterTimeMS += numSamples * 1000. / getSampleRate(); }

    bool hasEditor() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }

    juce::AudioProcessor::BusesProperties tuningBusLayout() {
        return BusesProperties()
            .withOutput ("Output1", juce::AudioChannelSet::stereo(), false)
            .withInput ("input", juce::AudioChannelSet::stereo(), false)
        .withOutput("Modulation",juce::AudioChannelSet::discreteChannels(1),true)
        .withInput( "Modulation",juce::AudioChannelSet::discreteChannels(1),true);
    }

private:
    chowdsp::Gain<float> gain;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TuningProcessor)
};

#endif //BITKLAVIER2_TUNINGPROCESSOR_H
