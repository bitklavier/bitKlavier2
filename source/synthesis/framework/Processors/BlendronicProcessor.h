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
#include "utils.h"
#include "buffer_debugger.h"

/**
 * @brief Serializes a MultiSlider parameter set into a Serializer object.
 *
 * This function takes a struct containing multi-slider state, converts its
 * atomic arrays into string representations, and adds them to a Serializer
 * object with a given base ID.
 *
 * @tparam Serializer A template parameter for the Serializer type.
 * @param ser A pointer to the Serializer object.
 * @param msliderParam A const reference to the struct containing the multi-slider data.
 * @param thisSliderID A const reference to the base ID string for this slider set.
 */
template <typename Serializer>
void serializeMultiSliderParam(
    typename Serializer::SerializedType& ser,
    const MultiSliderState& msliderParam,
    const juce::String& thisSliderID)
{
    // Define the specific string IDs for serialization
    juce::String thisSlider_sizeID = thisSliderID + "_size";
    juce::String activeSlidersID = thisSliderID + "_activeSliders";
    juce::String activeSliders_sizeID = activeSlidersID + "_size";

    // Serialize the float slider values
    juce::String beatLengths_str = atomicArrayToStringLimited(msliderParam.sliderVals, msliderParam.sliderVals_size);
    Serializer::addChildElement(ser, thisSlider_sizeID, juce::String(msliderParam.sliderVals_size));
    Serializer::addChildElement(ser, thisSliderID, beatLengths_str);

    // Serialize the boolean active sliders
    juce::String activeSliders_str = atomicArrayToStringLimited(msliderParam.activeSliders, msliderParam.activeVals_size);
    Serializer::addChildElement(ser, activeSliders_sizeID, juce::String(msliderParam.activeVals_size));
    Serializer::addChildElement(ser, activeSlidersID, activeSliders_str);
}

/**
 * @brief Deserializes and populates a MultiSliderState struct from a deserialized object.
 *
 * This function reads specific string attributes from a deserialized object,
 * parses them, and populates the members of a MultiSliderState struct, including
 * atomic arrays for slider values and active states.
 *
 * @tparam Serializer A template parameter for the Serializer type.
 * @param deserial The deserialized object to read attributes from.
 * @param msliderParam A reference to the MultiSliderState struct to be populated.
 * @param thisSliderID The base ID string for this slider set.
 */
template <typename Serializer>
void deserializeMultiSliderParam(
    typename Serializer::DeserializedType deserial,
    MultiSliderState& msliderParam,
    const juce::String& thisSliderID)
{
    // Reconstruct the attribute names using the base ID
    juce::String beatLengths_sizeID = thisSliderID + "_size";
    juce::String activeSlidersID = thisSliderID + "_activeSliders";
    juce::String activeSliders_sizeID = activeSlidersID + "_size";

    // Deserialize the slider values
    auto myStr = deserial->getStringAttribute(beatLengths_sizeID);
    msliderParam.sliderVals_size = myStr.getIntValue();
    myStr = deserial->getStringAttribute(thisSliderID);
    std::vector<float> sliderVals_vec = parseStringToVector<float>(myStr);
    populateAtomicArrayFromVector(msliderParam.sliderVals, 1.0f, sliderVals_vec);

    // Deserialize the active sliders
    myStr = deserial->getStringAttribute(activeSliders_sizeID);
    msliderParam.activeVals_size = myStr.getIntValue();
    myStr = deserial->getStringAttribute(activeSlidersID);
    std::vector<bool> activeSliders_vec = parseStringToBoolVector(myStr);
    populateAtomicArrayFromVector(msliderParam.activeSliders, false, activeSliders_vec);

    DBG("deserializeMultiSliderParam activeVals_size = " + juce::String(msliderParam.activeVals_size));
}

/**
 * holds all the parameters for Blendronic
 */
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
            outputGain,
            inputGain,
            outputSend);
    }

    // primary multislider params
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
     * serializers are used for more complex params, called on save and load
     *  - here we need these for all the multisliders
     */
    /* Custom serializer */
    template <typename Serializer>
    static typename Serializer::SerializedType serialize (const BlendronicParams& paramHolder);

    /* Custom deserializer */
    template <typename Serializer>
    static void deserialize (typename Serializer::DeserializedType deserial, BlendronicParams& paramHolder);

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
        for(auto [index, change] : stateChanges.changeState)
        {
            DBG("blendronic state change property: " + change.getPropertyName(0));
        }

        // must clear at the end, otherwise they'll get reapplied again and again
        stateChanges.changeState.clear();

//        for(auto [index, change] : stateChanges.changeState)
//        {
//            auto vminval = change.getProperty("velocitymin");
//            velocityMinParam->setParameterValue(vminval);
//
//            auto vmaxval = change.getProperty("velocitymax");
//            velocityMaxParam->setParameterValue(vmaxval);
//        }

//        auto float_params = getFloatParams();
//
//        int i = numActiveSliders->getCurrentValue();
//        for(auto [index, change] : stateChanges.changeState)
//        {
//            static juce::var nullVar;
//
//            for (i = 0; i < 12; i++)
//            {
//                auto str = "t" + juce::String(i);
//                auto val = change.getProperty(str);
//
//                // need to make sure we don't ignore values of 0.!
//                if (val == nullVar && !val.equalsWithSameType(0.)) break;
//
//                auto& float_param = float_params->at(i);
//                float_param.get()->setParameterValue(val);
//            }
//            numActiveSliders->setParameterValue(i);
//        }
//        stateChanges.changeState.clear();
    }

    /*
     * for storing outputLevels of this preparation for display
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

class BlendronicProcessor : public bitklavier::PluginBase<bitklavier::PreparationStateImpl<BlendronicParams, BlendronicNonParameterState>>,
                            public juce::ValueTree::Listener
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
            .withOutput("Output", juce::AudioChannelSet::stereo(), true)
            .withInput ("Input", juce::AudioChannelSet::stereo(), true)
            .withInput ("Modulation", juce::AudioChannelSet::discreteChannels (9), true)
            .withOutput("Modulation", juce::AudioChannelSet::mono(),false)
            .withOutput("Send",juce::AudioChannelSet::stereo(),true);
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
    double _subdivisions = 4.;
    double _periodMultiplier = 1.;

    /**
     * todo: General params
     * placeholders here for now
     */
    double _generalSettingsPeriodMultiplier = 1.;

    /*
     BlendronicProcessor has the primary function -- tick() -- that handles the delay line.
     The actual delay class is BlendronicDelay, a delay line with linear interpolation and feedback.
    */

    std::unique_ptr<BlendronicDelay> delay;

    void updateDelayParameters();
    void clearNextDelayBlock(int numSamples);
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

    /**
     * todo: for display
     */
    // For access in BlendronicDisplay
//    juce::Array<juce::uint64> beatPositionsInBuffer; // Record of the sample position of beat changes in the delay buffer (used in display)
//    int numBeatPositions; // Number of beat positions in the buffer and to be displayed
//    int beatPositionsIndex; // Index of beat sample positions for adding/removing positions
//    float pulseOffset; // Sample offset of the pulse grid from grid aligned with buffer start (used in display)
//    bool resetPhase;
//    OwnedArray<BlendronicDisplay::ChannelInfo> audio;
//    std::unique_ptr<BlendronicDisplay::ChannelInfo> smoothing;

    chowdsp::Gain<float> gain;

    juce::ScopedPointer<BufferDebugger> bufferDebugger;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BlendronicProcessor)
};


#endif //BITKLAVIER2_BLENDRONICPROCESSOR_H
