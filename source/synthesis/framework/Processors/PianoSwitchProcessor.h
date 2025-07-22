//
// Created by Dan Trueman on 7/22/25.
//

#ifndef BITKLAVIER0_PIANOSWITCHPROCESSOR_H
#define BITKLAVIER0_PIANOSWITCHPROCESSOR_H

#include <juce_audio_processors/juce_audio_processors.h>
#include <Identifiers.h>
#include "synth_base.h"
#include "PreparationStateImpl.h"
#include "PluginBase.h"

struct PianoSwitchParams : chowdsp::ParamHolder
{
    // Adds the appropriate parameters to the Tuning Processor
    PianoSwitchParams() : chowdsp::ParamHolder ("pianoswitch")
    {
        add (pianoToSwitchTo);
    }

    /*
     * this is the index for the piano to switch to
     * likely a better way to do this, but for now...
     */
    chowdsp::FloatParameter::Ptr pianoToSwitchTo {
        juce::ParameterID { "pianoToSwitchTo", 100 },
        "pianoToSwitchTo",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 128.0f, 64.0f),
        0.0f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };
};

struct PianoSwitchNonParameterState : chowdsp::NonParamState
{
    PianoSwitchNonParameterState()
    {
    }
};

class PianoSwitchProcessor: public bitklavier::PluginBase<bitklavier::PreparationStateImpl<PianoSwitchParams, PianoSwitchNonParameterState>>
{
public:
    PianoSwitchProcessor (const juce::ValueTree& v, SynthBase& parent);

    static std::unique_ptr<juce::AudioProcessor> create (SynthBase& parent, const juce::ValueTree& v);

    void prepareToPlay (double sampleRate, int samplesPerBlock) override {};
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    void processAudioBlock (juce::AudioBuffer<float>& buffer) override  {};

    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    bool hasEditor() const override { return false; }

    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    juce::AudioProcessor::BusesProperties pianoSwitchBusLayout() { return BusesProperties(); }

    const juce::String getName() const override { return "pianoswitch"; }
    double getTailLengthSeconds() const override {}
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 1; }
    const juce::String getProgramName(int index) override { return ""; }

    void setCurrentProgram(int index) override {}
    void changeProgramName(int index, const juce::String &newName) override {}
    void getStateInformation(juce::MemoryBlock &destData) override {}
    void setStateInformation(const void *data, int sizeInBytes) override {}

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PianoSwitchProcessor)
};

#endif //BITKLAVIER0_PIANOSWITCHPROCESSOR_H
