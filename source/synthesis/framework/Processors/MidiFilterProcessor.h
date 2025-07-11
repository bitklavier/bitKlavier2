//
// Created by Dan Trueman on 7/10/25.
//

#ifndef BITKLAVIER0_MIDIFILTERPROCESSOR_H
#define BITKLAVIER0_MIDIFILTERPROCESSOR_H

#include <juce_audio_processors/juce_audio_processors.h>
#include <Identifiers.h>
#include "synth_base.h"
#include "PreparationStateImpl.h"
#include "PluginBase.h"

struct MidiFilterParams : chowdsp::ParamHolder
{
    // Adds the appropriate parameters to the Tuning Processor
    MidiFilterParams() : chowdsp::ParamHolder ("midifilter")
    {
        add (mftoggle);
    }

    chowdsp::BoolParameter::Ptr mftoggle {
        juce::ParameterID { "mftoggle", 100},
        "some toggle",
        false
    };

    /*
     * serializers are used for more complex params
     */
    /* Custom serializer */
    template <typename Serializer>
    static typename Serializer::SerializedType serialize (const MidiFilterParams& paramHolder);

    /* Custom deserializer */
    template <typename Serializer>
    static void deserialize (typename Serializer::DeserializedType deserial, MidiFilterParams& paramHolder);
};

struct MidiFilterNonParameterState : chowdsp::NonParamState
{
    MidiFilterNonParameterState()
    {
    }
};

class MidiFilterProcessor : public bitklavier::PluginBase<bitklavier::PreparationStateImpl<MidiFilterParams,MidiFilterNonParameterState>>
//class MidiFilterProcessor : public juce::AudioProcessor
{
public:
    MidiFilterProcessor (const juce::ValueTree& v, SynthBase& parent);

    static std::unique_ptr<juce::AudioProcessor> create (SynthBase& parent, const juce::ValueTree& v);

    void prepareToPlay (double sampleRate, int samplesPerBlock) override {};
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    void processAudioBlock (juce::AudioBuffer<float>& buffer) override  {};

    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return true; }
    bool hasEditor() const override { return false; }

    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    juce::AudioProcessor::BusesProperties midiFilterBusLayout() { return BusesProperties(); }

    const juce::String getName() const override { return "midifilter"; }
    double getTailLengthSeconds() const override {}
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 1; }
    const juce::String getProgramName(int index) override { return ""; }

    void setCurrentProgram(int index) override {}
    void changeProgramName(int index, const juce::String &newName) override {}
    void getStateInformation(juce::MemoryBlock &destData) override {}
    void setStateInformation(const void *data, int sizeInBytes) override {}

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiFilterProcessor)
};

#endif //BITKLAVIER0_MIDIFILTERPROCESSOR_H
