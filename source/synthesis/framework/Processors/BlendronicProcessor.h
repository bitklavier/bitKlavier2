/*
==============================================================================

  Blendronic.h
  Created: 11 Jun 2019 2:00:53pm
  Author:  Theodore R Trevisan

  The original algorithm for Blendrónic was developed by Dan for the Feedback
  movement from "neither Anvil nor Pulley," and was subsequently used in
  Clapping Machine Music Variations, Olagón, and others. A paper describing
  the original algorithm was presented at the International Computer Music
  Conference in 2010 (http://www.manyarrowsmusic.com/papers/cmmv.pdf).

  "Clapping Machine Music Variations: A Composition for Acoustic/Laptop Ensemble"
  Dan Trueman
  Proceedings for the International Computer Music Conference
  SUNY Stony Brook, 2010

  The basic idea is that the length of a delay line changes periodically, as
  set by a sequence of beat lengths; the changes can happen instantaneously,
  or can take place over a period of time, a "smoothing" time that creates
  a variety of artifacts, tied to the beat pattern. The smoothing parameters
  themselves can be sequenced in a pattern, as can a feedback coefficient,
  which determines how much of the out of the delay line is fed back into it.

==============================================================================
*/

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
#include "BlendronicDelay.h"

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

    /**
     * todo: Tempo params
     * placeholders here for now
     */
    double _tempo = 120.;
    double _subdivisions = 1.;
    double _periodMultiplier = 1.;

    /**
     * todo: General params
     * placeholders here for now
     */
    double _generalSettingsPeriodMultiplier = 1.;

    /*
     BlendronicProcessor has the primary function -- tick() -- that handles the delay line.
     The actual delay class is BlendronicDelay, a delay line with linear interpolation and feedback.

     It connects Keymap, and Tempo preparations together as needed, and gets the Blendrónic
     params it needs to behave as expected.
    */

    std::unique_ptr<BlendronicDelay> delay;
    void updateDelayParameters();
//    void tick(float* outputs);
    void tick(float* inL, float* inR);

    juce::Array<juce::Array<float>> velocities;
    juce::Array<juce::Array<float>> invertVelocities;
    juce::Array<juce::uint64> holdTimers;
    juce::Array<int> keysDepressed;   //current keys that are depressed

    bool inSyncCluster, inClearCluster, inOpenCluster, inCloseCluster;
    bool nextSyncOffIsFirst, nextClearOffIsFirst, nextOpenOffIsFirst, nextCloseOffIsFirst;

    juce::uint64 thresholdSamples;
    juce::uint64 syncThresholdTimer;
    juce::uint64 clearThresholdTimer;
    juce::uint64 openThresholdTimer;
    juce::uint64 closeThresholdTimer;

    float pulseLength;      // Length in seconds of a pulse (1.0 length beat)
    float numSamplesBeat;   // Length in samples of the current step in the beat pattern
    float numSamplesDelay;  // Length in samples of the current step in the delay pattern

    juce::uint64 sampleTimer; // Sample count for timing param sequence steps

    // Index of sequenced param patterns
    int beatIndex, delayIndex, smoothIndex, feedbackIndex;

    // Values of previous step values for smoothing. Saved separately from param arrays to account for changes to the sequences
    float prevBeat, prevDelay, prevPulseLength;

    // Flag to clear the delay line on the next beat
    bool clearDelayOnNextBeat;

    // For access in BlendronicDisplay
    juce::Array<juce::uint64> beatPositionsInBuffer; // Record of the sample position of beat changes in the delay buffer (used in display)
    int numBeatPositions; // Number of beat positions in the buffer and to be displayed
    int beatPositionsIndex; // Index of beat sample positions for adding/removing positions
    float pulseOffset; // Sample offset of the pulse grid from grid aligned with buffer start (used in display)
    bool resetPhase;

    /**
     * todo: for display
     */
//    OwnedArray<BlendronicDisplay::ChannelInfo> audio;
//    std::unique_ptr<BlendronicDisplay::ChannelInfo> smoothing;

    chowdsp::Gain<float> gain;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BlendronicProcessor)
};


#endif //BITKLAVIER2_BLENDRONICPROCESSOR_H
