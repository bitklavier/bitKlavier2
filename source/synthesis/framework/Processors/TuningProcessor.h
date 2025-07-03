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
#include "SemitoneWidthParams.h"
#include <chowdsp_plugin_base/chowdsp_plugin_base.h>
#include <chowdsp_plugin_state/chowdsp_plugin_state.h>
#include <chowdsp_plugin_utils/chowdsp_plugin_utils.h>
#include <chowdsp_sources/chowdsp_sources.h>


/**
 * TuningState is the primary struct that is shared around to get/set tuning information
 */
struct TuningState : bitklavier::StateChangeableParameter
{
    void setKeyOffset (int midiNoteNumber, float val);
    void setCircularKeyOffset (int midiNoteNumber, float val);
    void setKeyOffset (int midiNoteNumber, float val, bool circular);
    static std::array<float, 12> rotateValuesByFundamental (std::array<float, 12> vals, int fundamental);
    void processStateChanges() override;

    void setFundamental (int fund);
    int getSemitoneWidthFundamental();
    double getSemitoneWidth();
    double getSemitoneWidthOffsetForMidiNote(double midiNoteNumber);
    int getClosestKey(int noteNum, float transp, bool tuneTranspositions);

    double getOverallOffset();
    double getTargetFrequency (int currentlyPlayingNote, double currentTransposition, bool tuneTranspositions);
    void updateLastFrequency(double lastFreq);

    juce::MidiKeyboardState keyboardState;
    std::array<float, 128> absoluteTuningOffset = { 0.f };
    std::array<float, 12> circularTuningOffset = { 0.f };
    int fundamental = 0;
    float A4frequency = 440.; // set this in gallery preferences
    double lastFrequencyHz = 440.;  // frequency of last getTargetFrequency returned
    double lastIntervalCents = 0.;  // difference between pitch of last two notes returned, in cents
    double lastMidiNote = 69.;      //pitch of last frequency returned

    SemitoneWidthParams semitoneWidthParams;

    // offset of tuning system (cents)
    chowdsp::FloatParameter::Ptr offSet {
        juce::ParameterID { "offSet", 100 },
        "Offset",
        chowdsp::ParamUtils::createNormalisableRange (-100.0f, 100.0f, 0.0f),
        0.0f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal,
        true
    };

    chowdsp::FloatParameter::Ptr lastNote {
        juce::ParameterID { "lastNote", 100 },
        "Last Note",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 128.0f, 64.0f),
        60.0f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };

    std::atomic<bool> setFromAudioThread;

    /** adaptive stuff */

    /**
     * adaptive = which adaptive system to use, anchored or not
     */
    chowdsp::EnumChoiceParameter<AdaptiveSystems>::Ptr adaptive {
        juce::ParameterID { "adaptiveSystem", 100 },
        "Adaptive System",
        AdaptiveSystems::Adaptive,
        std::initializer_list<std::pair<char, char>> { { '_', ' ' }, { '1', '/' }, { '2', '-' }, { '3', '\'' }, { '4', '#' }, { '5', 'b' } }
    };

    /**
     * tAdaptiveIntervalScale = scale to use to determine successive interval tuning
     */
    chowdsp::EnumChoiceParameter<TuningSystem>::Ptr tAdaptiveIntervalScale {
        juce::ParameterID { "tAdaptiveIntervalScale", 100 },
        "tAdaptiveIntervalScale",
        TuningSystem::Partial,
        std::initializer_list<std::pair<char, char>> { { '_', ' ' }, { '1', '/' }, { '2', '-' }, { '3', '\'' } }
    };

    /**
     * tAdaptiveAnchorScale = scale to tune new fundamentals to when in anchored
     */
    chowdsp::EnumChoiceParameter<TuningSystem>::Ptr tAdaptiveAnchorScale {
        juce::ParameterID { "tAdaptiveAnchorScale", 100 },
        "tAdaptiveAnchorScale",
        TuningSystem::Equal_Temperament,
        std::initializer_list<std::pair<char, char>> { { '_', ' ' }, { '1', '/' }, { '2', '-' }, { '3', '\'' } }
    };

    /**
     * tAdaptiveAnchorFundamental = fundamental for anchor scale
     */
    chowdsp::EnumChoiceParameter<Fundamental>::Ptr tAdaptiveAnchorFundamental {
        juce::ParameterID { "tAdaptiveAnchorFundamental", 100 },
        "tAdaptiveAnchorFundamental",
        Fundamental::C,
        std::initializer_list<std::pair<char, char>> { { '_', ' ' }, { '1', '/' }, { '2', '-' }, { '3', '\'' }, { '4', '#' }, { '5', 'b' } }
    };

    /**
     * tAdaptiveInversional = treat the adaptive scale inversionally?
     */
    chowdsp::BoolParameter::Ptr tAdaptiveInversional {
        juce::ParameterID { "tAdaptiveInversional", 100},
        "on_off",
        false
    };

    /**
     * tAdaptiveClusterThresh = ms; max time before fundamental is reset
     */
    chowdsp::TimeMsParameter::Ptr tAdaptiveClusterThresh {
        juce::ParameterID { "tAdaptiveClusterThresh", 100 },
        "tAdaptiveClusterThresh",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 1000.0f, 500.0f),
        0.0f,
        true
    };

    /**
     * tAdaptiveHistory = max number of notes before fundamental is reset
     */
    chowdsp::FloatParameter::Ptr tAdaptiveHistory {
        juce::ParameterID { "tAdaptiveHistory", 100 },
        "tAdaptiveHistory",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 8.0f, 4.0f),
        0.0f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };

    int getAdaptiveClusterTimer();
    void keyReleased(int noteNumber);
    void keyPressed(int noteNumber);
    float adaptiveCalculateRatio(const int midiNoteNumber) const;
    float adaptiveCalculate(int midiNoteNumber);
    void adaptiveReset();

    float getGlobalTuningReference() const { return A4frequency; };
    std::array<float, 12> getTuningSystem(TuningSystem which) const { return tuningMap[TuningSystem(which)].second; }
    int getFundamental() const { return fundamental; };
    AdaptiveSystems getAdaptiveType() const { return adaptive->get(); }
    inline const bool getAdaptiveInversional() const noexcept { return tAdaptiveInversional->get(); }
    inline const int getAdaptiveClusterThresh() const noexcept { return tAdaptiveClusterThresh->get(); }
    inline const int getAdaptiveHistory() const noexcept { return tAdaptiveHistory->get(); }
    inline const int getAdaptiveAnchorFundamental() const noexcept { return tAdaptiveAnchorFundamental->get(); }
    inline const TuningSystem getAdaptiveIntervalScale() const noexcept { return tAdaptiveIntervalScale->get(); }
    inline const TuningSystem getAdaptiveAnchorScale() const noexcept { return tAdaptiveAnchorScale->get(); }
    float intervalToRatio(float interval) const;

    /*
     * Adaptive vars
     */
    int adaptiveFundamentalNote = 60; //moves with adaptive tuning
    float adaptiveFundamentalFreq = mtof(adaptiveFundamentalNote);
    int adaptiveHistoryCounter = 0;
    float clusterTimeMS = 0.;
};

struct TuningParams : chowdsp::ParamHolder
{
    // Adds the appropriate parameters to the Tuning Processor
    TuningParams() : chowdsp::ParamHolder ("tuning")
    {
        add (tuningSystem,
            fundamental,
            tuningState.adaptive,
            tuningState.semitoneWidthParams,
            tuningState.offSet,
            tuningState.lastNote,
            tuningState.tAdaptiveIntervalScale,
            tuningState.tAdaptiveAnchorScale,
            tuningState.tAdaptiveAnchorFundamental,
            tuningState.tAdaptiveInversional,
            tuningState.tAdaptiveClusterThresh,
            tuningState.tAdaptiveHistory);
    }

    /*
     * the params below are not audio-rate modulatable, so will need to be handled
     * in a processStateChanges() call, called every block
     */
    /**
     * tuningSystem = overall system for static tunings
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
    chowdsp::EnumChoiceParameter<Fundamental>::Ptr fundamental {
        juce::ParameterID { "fundamental", 100 },
        "Fundamental",
        Fundamental::C,
        std::initializer_list<std::pair<char, char>> { { '_', ' ' }, { '1', '/' }, { '2', '-' }, { '3', '\'' }, { '4', '#' }, { '5', 'b' } }
    };

//    /**
//     * adaptive = which adaptive system to use, anchored or not
//     */
//    chowdsp::EnumChoiceParameter<AdaptiveSystems>::Ptr adaptive {
//        juce::ParameterID { "adaptiveSystem", 100 },
//        "Adaptive System",
//        AdaptiveSystems::None,
//        std::initializer_list<std::pair<char, char>> { { '_', ' ' }, { '1', '/' }, { '2', '-' }, { '3', '\'' }, { '4', '#' }, { '5', 'b' } }
//    };



    /**
     * params to add:
     *
     * individually:
     * - note and interval text boxes (display only)
     *
     * then:
     * - adaptive tunings
     * - spring tuning
     * - scala functionality
     * - MTS
     */

    /** this contains the current state of the tuning system, with helper functions **/
    TuningState tuningState;

    /**
     * serializers are used for more complex params
     *      - here we need arrays and indexed arrays for circular and absolute tunings, for instance
     */
    /** Custom serializer */
    template <typename Serializer>
    static typename Serializer::SerializedType serialize (const TuningParams& paramHolder);

    /** Custom deserializer */
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
    //std::array<float, 12> getTuningSystem(TuningSystem which) const { return tuningMap[TuningSystem(which)].second; }

    /*
     * Adaptive Tuning functions
     */
//    int getAdaptiveClusterTimer();
//    void keyReleased(int noteNumber);
//    void keyPressed(int noteNumber);
//    float adaptiveCalculateRatio(const int midiNoteNumber) const;
//    float adaptiveCalculate(int midiNoteNumber);
//    void adaptiveReset();
//
//    float getGlobalTuningReference() const { return state.params.tuningState.A4frequency; };
//    int getFundamental() const { return state.params.tuningState.fundamental; };
//    AdaptiveSystems getAdaptiveType() const { return state.params.adaptive->get(); }
//    inline const bool getAdaptiveInversional() const noexcept { return state.params.tAdaptiveInversional->get(); }
//    inline const int getAdaptiveClusterThresh() const noexcept { return state.params.tAdaptiveClusterThresh->get(); }
//    inline const int getAdaptiveHistory() const noexcept { return state.params.tAdaptiveHistory->get(); }
//    inline const int getAdaptiveAnchorFundamental() const noexcept { return state.params.tAdaptiveAnchorFundamental->get(); }
//    inline const TuningSystem getAdaptiveIntervalScale() const noexcept { return state.params.tAdaptiveIntervalScale->get(); }
//    inline const TuningSystem getAdaptiveAnchorScale() const noexcept { return state.params.tAdaptiveAnchorScale->get(); }
//    float intervalToRatio(float interval) const;

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

//    /*
//     * Adaptive vars
//     */
//    int adaptiveFundamentalNote = 60; //moves with adaptive tuning
//    float adaptiveFundamentalFreq = mtof(adaptiveFundamentalNote);
//    int adaptiveHistoryCounter = 0;
//    long clusterTime;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TuningProcessor)
};

#endif //BITKLAVIER2_TUNINGPROCESSOR_H
