//
// Created by Dan Trueman on 8/2/25.
//

/**
 * MidiTarget takes a MidiMessage and sends it out on different channels
 * depending on which preparation targets are activated by the user.
 *
 * For instance, if MidiTarget is between Keymap and Blendronic, and
 * the user has toggled on 'BlendronicTargetPatternSync' and 'BlendronicTargetBeatSync'
 * but left the remaining off, then MidiTarget will take each incoming MIDI message
 * and send out two copies, one on channel 2 (BlendronicTargetPatternSync) and one on
 * channel 3 (BlendronicTargetBeatSync), as determined by their positions in
 * the enum 'BlendronicTargetType.' Note that there will NOT be a message on channel 1
 * in this case, since 'BlendronicTargetNormal' was not toggled on by the user.
 */

#ifndef BITKLAVIER0_MIDITARGETPROCESSOR_H
#define BITKLAVIER0_MIDITARGETPROCESSOR_H

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <Identifiers.h>
#include "synth_base.h"
#include "PreparationStateImpl.h"
#include "PluginBase.h"
#include "target_types.h"

/**
 * todo: MidiTargetProcessor needs to know what kind of prep it first connects to
 *      and then can only connect to that kind of prep again.
 *      the processBlock then will choose the particular target channel vals to iterate
 *      through based on the connected prep type
 */

struct MidiTargetParams : chowdsp::ParamHolder
{
    // Adds the appropriate parameters to the Tuning Processor
    MidiTargetParams() : chowdsp::ParamHolder ("miditarget")
    {
        add (mftoggle);
    }

    chowdsp::BoolParameter::Ptr mftoggle {
        juce::ParameterID { "mftoggle", 100},
        "some toggle",
        false
    };

     // max size based on max channels for midimsg
    std::array<bool, 16> activeTargets;

    /*
     * serializers are used for more complex params
     */
    /* Custom serializer */
    template <typename Serializer>
    static typename Serializer::SerializedType serialize (const MidiTargetParams& paramHolder);

    /* Custom deserializer */
    template <typename Serializer>
    static void deserialize (typename Serializer::DeserializedType deserial, MidiTargetParams& paramHolder);
};

struct MidiTargetNonParameterState : chowdsp::NonParamState
{
    MidiTargetNonParameterState()
    {
    }
};

class MidiTargetProcessor : public bitklavier::PluginBase<bitklavier::PreparationStateImpl<MidiTargetParams,MidiTargetNonParameterState>>
{
public:
    MidiTargetProcessor (const juce::ValueTree& v, SynthBase& parent);
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
    juce::AudioProcessor::BusesProperties midiTargetBusLayout() { return BusesProperties().withInput("disabled",juce::AudioChannelSet::mono(),false)
            .withOutput("Disabled",juce::AudioChannelSet::mono(),false)
            .withOutput("Modulation",juce::AudioChannelSet::discreteChannels(1),false)
            .withInput( "Modulation",juce::AudioChannelSet::discreteChannels(1),true); }

    const juce::String getName() const override { return "miditarget"; }
    double getTailLengthSeconds() const override {}
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 1; }
    const juce::String getProgramName(int index) override { return ""; }

    void setCurrentProgram(int index) override {}
    void changeProgramName(int index, const juce::String &newName) override {}
    void getStateInformation(juce::MemoryBlock &destData) override {}
    void setStateInformation(const void *data, int sizeInBytes) override {}

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiTargetProcessor)
};
#endif //BITKLAVIER0_MIDITARGETPROCESSOR_H
