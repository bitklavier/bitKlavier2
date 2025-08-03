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
 * the enum 'ParameterTargetType.' Note that there will NOT be a message on channel 1
 * in this case, since 'BlendronicTargetNormal' was not toggled on by the user.
 *
 * In addition, there is the noteOn/noteOff/both mode, where these messages will be
 * sent only in the selected mode: in noteOff mode, for instance, noteOn messages are ignored
 *
 * NOTE: we can have no more than 16 target params for an individual preparation type!
 *          because we are using the Midichannel byte... perhaps MIDI 2.0 will offer other options....
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
    // Adds the appropriate parameters to the MidiTarget Processor
    MidiTargetParams() : chowdsp::ParamHolder ("miditarget")
    {
        add (blendronicTargetNormal,
            blendronicTargetPatternSync,
            blendronicTargetBeatSync,
            blendronicTargetClear,
            blendronicTargetPausePlay,
            blendronicTargetInput,
            blendronicTargetOutput,
            blendronicTargetNormal_noteMode,
            blendronicTargetPatternSync_noteMode,
            blendronicTargetBeatSync_noteMode,
            blendronicTargetClear_noteMode,
            blendronicTargetPausePlay_noteMode,
            blendronicTargetInput_noteMode,
            blendronicTargetOutput_noteMode);

        targetMapper[BlendronicTargetNormal]            = blendronicTargetNormal.get();
        noteModeMapper[BlendronicTargetNormal]          = blendronicTargetNormal_noteMode.get();

        targetMapper[BlendronicTargetPatternSync]       = blendronicTargetPatternSync.get();
        noteModeMapper[BlendronicTargetPatternSync]     = blendronicTargetPatternSync_noteMode.get();

        targetMapper[BlendronicTargetBeatSync]          = blendronicTargetBeatSync.get();
        noteModeMapper[BlendronicTargetBeatSync]        = blendronicTargetBeatSync_noteMode.get();

        targetMapper[BlendronicTargetClear]             = blendronicTargetClear.get();
        noteModeMapper[BlendronicTargetClear]           = blendronicTargetClear_noteMode.get();

        targetMapper[BlendronicTargetPausePlay]         = blendronicTargetPausePlay.get();
        noteModeMapper[BlendronicTargetPausePlay]       = blendronicTargetPausePlay_noteMode.get();

        targetMapper[BlendronicTargetInput]             = blendronicTargetInput.get();
        noteModeMapper[BlendronicTargetInput]           = blendronicTargetInput_noteMode.get();

        targetMapper[BlendronicTargetOutput]            = blendronicTargetOutput.get();
        noteModeMapper[BlendronicTargetOutput]          = blendronicTargetOutput_noteMode.get();

        /**
         * add additional params for other preps/targets here and below
         */
    }

    chowdsp::BoolParameter::Ptr blendronicTargetNormal {
        juce::ParameterID { "bTargetNormal", 100},
        "Normal",
        false
    };

    chowdsp::BoolParameter::Ptr blendronicTargetPatternSync {
        juce::ParameterID { "bTargetPatternSync", 100},
        "Pattern Sync",
        false
    };

    chowdsp::BoolParameter::Ptr blendronicTargetBeatSync {
        juce::ParameterID { "bTargetBeatSync", 100},
        "Beat Sync",
        false
    };

    chowdsp::BoolParameter::Ptr blendronicTargetClear {
        juce::ParameterID { "bTargetClear", 100},
        "Clear",
        false
    };

    chowdsp::BoolParameter::Ptr blendronicTargetPausePlay {
        juce::ParameterID { "bTargetPausePlay", 100},
        "Pause/Play",
        false
    };

    chowdsp::BoolParameter::Ptr blendronicTargetInput {
        juce::ParameterID { "bTargetInput", 100},
        "Open/Close Input",
        false
    };

    chowdsp::BoolParameter::Ptr blendronicTargetOutput {
        juce::ParameterID { "bTargetOutput", 100},
        "Open/Close Output",
        false
    };

    chowdsp::EnumChoiceParameter<TriggerType>::Ptr blendronicTargetNormal_noteMode {
        juce::ParameterID{"bTargetNormal_noteMode", 100},
        "Note Mode",
        TriggerType::_NoteOn,
        std::initializer_list<std::pair<char, char>> { { '_', ' ' } }
    };

    chowdsp::EnumChoiceParameter<TriggerType>::Ptr blendronicTargetPatternSync_noteMode {
        juce::ParameterID{"bTargetPatternSync_noteMode", 100},
        "Note Mode",
        TriggerType::_NoteOn,
        std::initializer_list<std::pair<char, char>> { { '_', ' ' } }
    };

    chowdsp::EnumChoiceParameter<TriggerType>::Ptr blendronicTargetBeatSync_noteMode {
        juce::ParameterID{"bTargetBeatSync_noteMode", 100},
        "Note Mode",
        TriggerType::_NoteOn,
        std::initializer_list<std::pair<char, char>> { { '_', ' ' } }
    };

    chowdsp::EnumChoiceParameter<TriggerType>::Ptr blendronicTargetClear_noteMode {
        juce::ParameterID{"bTargetClear_noteMode", 100},
        "Note Mode",
        TriggerType::_NoteOn,
        std::initializer_list<std::pair<char, char>> { { '_', ' ' } }
    };

    chowdsp::EnumChoiceParameter<TriggerType>::Ptr blendronicTargetPausePlay_noteMode {
        juce::ParameterID{"bTargetPausePlay_noteMode", 100},
        "Note Mode",
        TriggerType::_NoteOn,
        std::initializer_list<std::pair<char, char>> { { '_', ' ' } }
    };

    chowdsp::EnumChoiceParameter<TriggerType>::Ptr blendronicTargetInput_noteMode {
        juce::ParameterID{"bTargetInput_noteMode", 100},
        "Note Mode",
        TriggerType::_NoteOn,
        std::initializer_list<std::pair<char, char>> { { '_', ' ' } }
    };

    chowdsp::EnumChoiceParameter<TriggerType>::Ptr blendronicTargetOutput_noteMode {
        juce::ParameterID{"bTargetOutput_noteMode", 100},
        "Note Mode",
        TriggerType::_NoteOn,
        std::initializer_list<std::pair<char, char>> { { '_', ' ' } }
    };

    /*
     * we store all the targets and their noteModes here, so we can access them
     * as needed in the processBlock loop, by PreparationParameterTargetType
     */
    std::map<PreparationParameterTargetType, chowdsp::BoolParameter*> targetMapper;
    std::map<PreparationParameterTargetType, chowdsp::EnumChoiceParameter<TriggerType>*> noteModeMapper;

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
//    void getStateInformation(juce::MemoryBlock &destData) override {}
//    void setStateInformation(const void *data, int sizeInBytes) override {}

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiTargetProcessor)
};
#endif //BITKLAVIER0_MIDITARGETPROCESSOR_H
