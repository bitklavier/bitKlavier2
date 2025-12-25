//
// Created by Dan Trueman on 7/10/25.
//

#ifndef BITKLAVIER0_MIDIFILTERPROCESSOR_H
#define BITKLAVIER0_MIDIFILTERPROCESSOR_H

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <Identifiers.h>
#include "synth_base.h"
#include "PreparationStateImpl.h"
#include "PluginBase.h"

struct MidiFilterParams : chowdsp::ParamHolder
{
    // Adds the appropriate parameters to the Tuning Processor
    MidiFilterParams(const juce::ValueTree &v) : chowdsp::ParamHolder ("midifilter")
    {
        add (allNotesOff,
            toggleNoteMessages,
            ignoreNoteOn,
            ignoreNoteOff,
            invertNoteOnOff,
            ignoreSustainPedal,
            sostenutoMode,
            notesAreSustainPedal,
            notesAreSostenutoPedal);
    }

    chowdsp::BoolParameter::Ptr ignoreNoteOn {
        juce::ParameterID { "ignoreNoteOn", 100},
        "ignore noteOn",
        false
    };

    chowdsp::BoolParameter::Ptr ignoreNoteOff {
        juce::ParameterID { "ignoreNoteOff", 100},
        "ignore noteOff",
        false
    };

    chowdsp::BoolParameter::Ptr invertNoteOnOff {
        juce::ParameterID { "invertNoteOnOff", 100},
        "invert noteOn/Off",
        false
    };

    chowdsp::BoolParameter::Ptr allNotesOff {
        juce::ParameterID { "allNotesOff", 100},
        "all notes off!",
        false
    };

    /*
     * turn each key into a toggle
     * - if note is not playing, noteOn will play, and noteOff will be ignored
     * - if note is already playing, then noteOn will trigger noteOff to turn it off
     */
    chowdsp::BoolParameter::Ptr toggleNoteMessages {
        juce::ParameterID { "toggleNoteMessages", 100},
        "toggle noteOn/Off",
        false
    };

    chowdsp::BoolParameter::Ptr ignoreSustainPedal {
        juce::ParameterID { "ignoreSustainPedal", 100},
        "ignore sustain pedal",
        false
    };

    chowdsp::BoolParameter::Ptr sostenutoMode {
        juce::ParameterID { "sostenutoMode", 100},
        "treat sustain pedal like sostenuto pedal",
        false
    };

    chowdsp::BoolParameter::Ptr notesAreSustainPedal {
        juce::ParameterID { "notesAreSustainPedal", 100},
        "treat keys like sustain pedal",
        false
    };

    chowdsp::BoolParameter::Ptr notesAreSostenutoPedal {
        juce::ParameterID { "notesAreSostenutoPedal", 100},
        "treat keys like sostenuto pedal",
        false
    };

};

struct MidiFilterNonParameterState : chowdsp::NonParamState
{
    MidiFilterNonParameterState()
    {
    }
};

class MidiFilterProcessor : public bitklavier::PluginBase<bitklavier::PreparationStateImpl<MidiFilterParams,MidiFilterNonParameterState>>
{
public:
    MidiFilterProcessor ( SynthBase& parent,const juce::ValueTree& v);

    void prepareToPlay (double sampleRate, int samplesPerBlock) override {};
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    void processAudioBlock (juce::AudioBuffer<float>& buffer) override  {};
    void processBlockBypassed (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override {};

    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return true; }
    bool hasEditor() const override { return false; }

    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    juce::AudioProcessor::BusesProperties midiFilterBusLayout() { return BusesProperties().withInput("disabled",juce::AudioChannelSet::mono(),false)
            .withOutput("Disabled",juce::AudioChannelSet::mono(),false)
            .withOutput("Modulation",juce::AudioChannelSet::discreteChannels(1),false)
            .withInput( "Modulation",juce::AudioChannelSet::discreteChannels(1),true); }

    const juce::String getName() const override { return "midifilter"; }
    double getTailLengthSeconds() const override {}
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 1; }
    const juce::String getProgramName(int index) override { return ""; }

    void setCurrentProgram(int index) override {}
    void changeProgramName(int index, const juce::String &newName) override {}

    /**
     * Midi Processing Functions
     */
    juce::MidiMessage swapNoteOnNoteOff (juce::MidiMessage inmsg);

private:
    std::bitset<128> noteOnState;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiFilterProcessor)
};

#endif //BITKLAVIER0_MIDIFILTERPROCESSOR_H
