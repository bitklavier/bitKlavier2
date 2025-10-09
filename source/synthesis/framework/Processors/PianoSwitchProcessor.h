//
// Created by Dan Trueman on 7/22/25.
//

#ifndef BITKLAVIER0_PIANOSWITCHPROCESSOR_H
#define BITKLAVIER0_PIANOSWITCHPROCESSOR_H

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <Identifiers.h>
#include "synth_base.h"
#include "PreparationStateImpl.h"
#include "PluginBase.h"

struct PianoSwitchParams : chowdsp::ParamHolder
{
    PianoSwitchParams(const juce::ValueTree& v) : chowdsp::ParamHolder ("pianoswitch"){}
};

struct PianoSwitchNonParameterState : chowdsp::NonParamState
{
    PianoSwitchNonParameterState(){}
};

class PianoSwitchProcessor : public bitklavier::PluginBase<bitklavier::PreparationStateImpl<PianoSwitchParams, PianoSwitchNonParameterState>>
{
public:
    PianoSwitchProcessor (SynthBase& parent, const juce::ValueTree&);

    void prepareToPlay(double sampleRate, int samplesPerBlock) override {}
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages) override;
    void processAudioBlock (juce::AudioBuffer<float>& buffer) override  {};

    bool acceptsMidi() const override {return true;}
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    bool hasEditor() const override { return false; }

    juce::AudioProcessorEditor * createEditor() override { return nullptr; }
    //uses modulation bus for ordering
    juce::AudioProcessor::BusesProperties pianoSwitchBusLayout() { return BusesProperties().withInput("disabled",juce::AudioChannelSet::mono(),false)
        .withOutput("disabled",juce::AudioChannelSet::mono(),false)
        .withOutput("Modulation",juce::AudioChannelSet::discreteChannels(1),true)
        .withInput( "Modulation",juce::AudioChannelSet::discreteChannels(1),true); }

    const juce::String getName() const override { return "pianoswitch"; }
    double getTailLengthSeconds() const override {}
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 1; }
    const juce::String getProgramName(int index) override { return ""; }

    void setCurrentProgram(int index) override {}
    void changeProgramName(int index, const juce::String &newName) override {}
    void getStateInformation(juce::MemoryBlock &destData) override {}
    void setStateInformation(const void *data, int sizeInBytes) override {}

private :
    SynthBase& synth_base_;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PianoSwitchProcessor)
};

#endif //BITKLAVIER0_PIANOSWITCHPROCESSOR_H
