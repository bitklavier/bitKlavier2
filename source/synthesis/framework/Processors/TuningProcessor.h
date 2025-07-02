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

enum AdaptiveSystems {
    None = 1 << 0,
    Adaptive = 1 << 1,
    Adaptive_Anchored = 1 << 2,
    Spring = 1 << 3,
};

/**
 * TuningState is the primary struct that is shared around to get/set tuning information
 */
struct TuningState : bitklavier::StateChangeableParameter
{
    void setKeyOffset (int midiNoteNumber, float val)
    {
        if (midiNoteNumber >= 0 && midiNoteNumber < 128)
            absoluteTuningOffset[midiNoteNumber] = val;
    }

    void setCircularKeyOffset (int midiNoteNumber, float val)
    {
        if (midiNoteNumber >= 0 && midiNoteNumber < 12)
            circularTuningOffset[midiNoteNumber] = val;
    }

    void setKeyOffset (int midiNoteNumber, float val, bool circular)
    {
        if (circular)
            setCircularKeyOffset (midiNoteNumber, val);
        else
            setKeyOffset (midiNoteNumber, val);
    }

    void processStateChanges() override
    {
        for (auto [index, change] : stateChanges.changeState)
        {
            static juce::var nullVar;
            auto val = change.getProperty (IDs::absoluteTuning);
            auto val1 = change.getProperty (IDs::circularTuning);
            if (val != nullVar)
            {
                absoluteTuningOffset = parseIndexValueStringToArrayAbsolute<128> (val.toString().toStdString());
            }
            else if (val1 != nullVar)
            {
                circularTuningOffset = parseFloatStringToArrayCircular<12> (val1.toString().toStdString());
                // absoluteTuningOffset = std::array<float,128>(val1.toString().toStdString());
            }
        }
    }

    static std::array<float, 12> rotateValuesByFundamental (std::array<float, 12> vals, int fundamental)
    {
        int offset;
        if (fundamental <= 0)
            offset = 0;
        else
            offset = fundamental;
        std::array<float, 12> new_vals = { 0.f };
        for (int i = 0; i < 12; i++)
        {
            int index = ((i - offset) + 12) % 12;
            new_vals[i] = vals[index];
        }
        return new_vals;
    }

    void setFundamental (int fund)
    {
        //need to shift keyValues over by difference in fundamental
        int oldFund = fundamental;
        fundamental = fund;
        int offset = fund - oldFund;
        auto vals = circularTuningOffset;
        for (int i = 0; i < 12; i++)
        {
            int index = ((i - offset) + 12) % 12;
            circularTuningOffset[i] = vals[index];
        }
    }

    /**
     * helper function for the semitone width fundamental UI elements
     * @return the fundamental in midinote number value, given the octave and pitchclass name (so C4 will return 60)
     */
    int getSemitoneWidthFundamental()
    {
        auto fund = semitoneWidthParams.reffundamental.get()->getIndex();
        auto oct = semitoneWidthParams.octave->getCurrentValueAsText().juce::String::getIntValue();
        return fund + (oct + 1) * 12;
    }

    double getSemitoneWidth()
    {
        return semitoneWidthParams.semitoneWidthSliderParam->getCurrentValue();
    }

    /**
     *
     * @param midiNoteNumber
     * @return new transposition to new midiNoteNumber based on semitone width setting (fractional midi value)
     * if semitone width is 100, then output = 0
     * otherwise the output will be transposed by the return value
     * for example: if the semitone width = 50, the semitone fundamental = 60, and midiNoteNumber = 61, the output will be -0.5
     */
    double getSemitoneWidthOffsetForMidiNote(double midiNoteNumber)
    {
        double offset;
        if (fabs(getSemitoneWidth() - 100.) < 1.) offset = 0.; // return 0 for cases within a cent of 100
        else offset = .01 * (midiNoteNumber - getSemitoneWidthFundamental()) * (getSemitoneWidth() - 100.);
        return offset;
    }

    /**
     * BKSynth will use this to find the closest sample for a particular note
     *      need something like this to find the best sample for this midiNoteNumber
     *      it may be very far from the original midi key played because of the semitone width variable
     * @param noteNum
     * @param transp
     * @return
     */
    int getClosestKey(int noteNum, float transp, bool tuneTranspositions)
    {
        /*
         * check for when there is no need to adjust for semitone width (which is 99.9% of the time!)
         */
        if (getSemitoneWidthOffsetForMidiNote(noteNum) == 0)
        {
            return (noteNum + transp);
        }

        double workingOffset;
        if (!tuneTranspositions) {
            workingOffset = getSemitoneWidthOffsetForMidiNote (noteNum); // only track semitone width changes for the played note, note the transposition
        }
        else {
            workingOffset = getSemitoneWidthOffsetForMidiNote (noteNum + transp); // track semitone width for transposition as well
        }

        int key = static_cast<int>(std::round(noteNum + workingOffset + transp));

//        DBG("closest key for " + juce::String(noteNum)
//             + " with transp " + juce::String(transp)
//             + " = " + juce::String(key));

        return key;
    }

    /**
     * Get the tuning offset value, from "offset" slider
     * @return offset in fractional Midi note values
     */
    double getOffset()
    {
        return offSet->getCurrentValue() * 0.01;
    }

    /**
     * getTargetFrequency() is the primary function for synthesizers to handle tuning
     *      should include static and dynamic tunings, and account for semitone width changes
     *      is called every block
     *      return fractional MIDI value, NOT cents
     *
     * @param currentlyPlayingNote
     * @param currentTransposition
     * @param tuneTranspositions
     * @return fractional MIDI value (not cents)
     */
    double getTargetFrequency (int currentlyPlayingNote, double currentTransposition, bool tuneTranspositions);

    juce::MidiKeyboardState keyboardState;
    std::array<float, 128> absoluteTuningOffset = { 0.f };
    std::array<float, 12> circularTuningOffset = { 0.f };
    int fundamental = 0;
    float A4frequency = 440.; // set this in gallery preferences

    std::atomic<bool> setFromAudioThread;

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
};

struct TuningParams : chowdsp::ParamHolder
{
    // Adds the appropriate parameters to the Tuning Processor
    TuningParams() : chowdsp::ParamHolder ("tuning")
    {
        add (tuningSystem,
            fundamental,
            adaptive,
            tuningState.semitoneWidthParams,
            tuningState.offSet);
    }

    /*
     * these params are not audio-rate modulatable, so will need to be handled
     * in a processStateChanges() call, called every block
     */
    chowdsp::EnumChoiceParameter<TuningSystem>::Ptr tuningSystem {
        juce::ParameterID { "tuningSystem", 100 },
        "Tuning System",
        TuningSystem::Equal_Temperament,
        std::initializer_list<std::pair<char, char>> { { '_', ' ' }, { '1', '/' }, { '2', '-' }, { '3', '\'' } }
    };

    chowdsp::EnumChoiceParameter<Fundamental>::Ptr fundamental {
        juce::ParameterID { "fundamental", 100 },
        "Fundamental",
        Fundamental::C,
        std::initializer_list<std::pair<char, char>> { { '_', ' ' }, { '1', '/' }, { '2', '-' }, { '3', '\'' }, { '4', '#' }, { '5', 'b' } }
    };

    chowdsp::EnumChoiceParameter<AdaptiveSystems>::Ptr adaptive {
        juce::ParameterID { "adaptiveSystem", 100 },
        "Adaptive System",
        AdaptiveSystems::None,
        std::initializer_list<std::pair<char, char>> { { '_', ' ' }, { '1', '/' }, { '2', '-' }, { '3', '\'' }, { '4', '#' }, { '5', 'b' } }
    };


    /**
     * params to add:
     *
     * individually:
     * - offset (float slider/knob)
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

    static std::unique_ptr<juce::AudioProcessor> create (SynthBase& parent, const juce::ValueTree& v)
    {
        return std::make_unique<TuningProcessor> (parent, v);
    }
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processAudioBlock (juce::AudioBuffer<float>& buffer) override {};
    bool acceptsMidi() const override
    {
        return true;
    }
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    juce::AudioProcessor::BusesProperties tuningBusLayout()
    {
        return BusesProperties()
            .withOutput ("Output1", juce::AudioChannelSet::stereo(), false)
            .withInput ("input", juce::AudioChannelSet::stereo(), false);
    }

    bool hasEditor() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }

private:
    chowdsp::Gain<float> gain;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TuningProcessor)
};

#endif //BITKLAVIER2_TUNINGPROCESSOR_H
