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
#include "ObjectLists/ConnectionsList.h"
#include "PluginBase.h"
#include "PreparationStateImpl.h"
#include "synth_base.h"
#include "target_types.h"
#include <Identifiers.h>
#include <juce_audio_processors/juce_audio_processors.h>

/**
 * todo: MidiTargetProcessor needs to know what kind of prep it first connects to
 *      and then can only connect to that kind of prep again.
 *      the processBlock then will choose the particular target channel vals to iterate
 *      through based on the connected prep type
 */

struct MidiTargetParams : chowdsp::ParamHolder
{
    // Adds the appropriate parameters to the MidiTarget Processor
    MidiTargetParams(const juce::ValueTree& v) : chowdsp::ParamHolder ("miditarget")
    {
        /*
         * be sure to add these in matched orders (_noteMode same as regular param), otherwise the UI won't setup correctly
         */
        add (
            blendronicTargetPatternSync,
            blendronicTargetBeatSync,
            blendronicTargetClear,
            blendronicTargetPausePlay,
            blendronicTargetInput,
            blendronicTargetOutput,
            synchronicTargetDefault,
            synchronicTargetPatternSync,
            synchronicTargetBeatSync,
            synchronicTargetAddNotes,
            synchronicTargetClear,
            synchronicTargetPausePlay,
//            synchronicTargetDeleteOldest,
//            synchronicTargetDeleteNewest,
//            synchronicTargetRotate,
            blendronicTargetPatternSync_noteMode,
            blendronicTargetBeatSync_noteMode,
            blendronicTargetClear_noteMode,
            blendronicTargetPausePlay_noteMode,
            blendronicTargetInput_noteMode,
            blendronicTargetOutput_noteMode,
            synchronicTargetDefault_noteMode,
            synchronicTargetPatternSync_noteMode,
            synchronicTargetBeatSync_noteMode,
            synchronicTargetAddNotes_noteMode,
            synchronicTargetClear_noteMode,
            synchronicTargetPausePlay_noteMode,
            resonanceTargetDefault,
            resonanceTargetRing,
            resonanceTargetAdd,
            resonanceTargetDefault_noteMode,
            resonanceTargetRing_noteMode,
            resonanceTargetAdd_noteMode,
            nostalgicTargetDefault,
            nostalgicTargetClear,
            nostalgicTargetDefault_noteMode,
            nostalgicTargetClear_noteMode);
//            synchronicTargetDeleteOldest_noteMode,
//            synchronicTargetDeleteNewest_noteMode,
//            synchronicTargetRotate_noteMode);

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

        targetMapper[SynchronicTargetDefault]           = synchronicTargetDefault.get();
        noteModeMapper[SynchronicTargetDefault]         = synchronicTargetDefault_noteMode.get();

        targetMapper[SynchronicTargetPatternSync]       = synchronicTargetPatternSync.get();
        noteModeMapper[SynchronicTargetPatternSync]     = synchronicTargetPatternSync_noteMode.get();

        targetMapper[SynchronicTargetBeatSync]          = synchronicTargetBeatSync.get();
        noteModeMapper[SynchronicTargetBeatSync]        = synchronicTargetBeatSync_noteMode.get();

        targetMapper[SynchronicTargetAddNotes]          = synchronicTargetAddNotes.get();
        noteModeMapper[SynchronicTargetAddNotes]        = synchronicTargetAddNotes_noteMode.get();

        targetMapper[SynchronicTargetClear]             = synchronicTargetClear.get();
        noteModeMapper[SynchronicTargetClear]           = synchronicTargetClear_noteMode.get();

        targetMapper[SynchronicTargetPausePlay]         = synchronicTargetPausePlay.get();
        noteModeMapper[SynchronicTargetPausePlay]       = synchronicTargetPausePlay_noteMode.get();

        targetMapper[ResonanceTargetDefault]            = resonanceTargetDefault.get();
        noteModeMapper[ResonanceTargetDefault]          = resonanceTargetDefault_noteMode.get();

        targetMapper[ResonanceTargetAdd]                = resonanceTargetAdd.get();
        noteModeMapper[ResonanceTargetAdd]              = resonanceTargetAdd_noteMode.get();

        targetMapper[ResonanceTargetRing]                = resonanceTargetRing.get();
        noteModeMapper[ResonanceTargetRing]              = resonanceTargetRing_noteMode.get();

//        targetMapper[SynchronicTargetDeleteOldest]      = synchronicTargetDeleteOldest.get();
//        noteModeMapper[SynchronicTargetDeleteOldest]    = synchronicTargetDeleteOldest_noteMode.get();
//
//        targetMapper[SynchronicTargetDeleteNewest]      = synchronicTargetDeleteNewest.get();
//        noteModeMapper[SynchronicTargetDeleteNewest]    = synchronicTargetDeleteNewest_noteMode.get();
//
//        targetMapper[SynchronicTargetRotate]            = synchronicTargetRotate.get();
//        noteModeMapper[SynchronicTargetRotate]          = synchronicTargetRotate_noteMode.get();

        /**
         * add additional params for other preps/targets here and below
         */
        targetMapper[NostalgicTargetDefault]           = nostalgicTargetDefault.get();
        noteModeMapper[NostalgicTargetDefault]         = nostalgicTargetDefault_noteMode.get();

        targetMapper[NostalgicTargetClear]           = nostalgicTargetClear.get();
        noteModeMapper[NostalgicTargetClear]         = nostalgicTargetClear_noteMode.get();
    }

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
     * Synchronic Targets
     */

    // pass through all messages on channel 1 by default; same as having no MidiTarget involved
    chowdsp::BoolParameter::Ptr synchronicTargetDefault {
        juce::ParameterID { "sTargetDefault", 100},
        "Default Behavior",
        false
    };

    chowdsp::EnumChoiceParameter<TriggerType>::Ptr synchronicTargetDefault_noteMode {
        juce::ParameterID{"sTargetDefault_noteMode", 100},
        "Note Mode",
        TriggerType::_Both,
        std::initializer_list<std::pair<char, char>> { { '_', ' ' } }
    };

    chowdsp::BoolParameter::Ptr synchronicTargetPatternSync {
        juce::ParameterID { "sTargetPatternSync", 100},
        "Pattern Sync",
        false
    };

    chowdsp::EnumChoiceParameter<TriggerType>::Ptr synchronicTargetPatternSync_noteMode {
        juce::ParameterID{"sTargetPatternSync_noteMode", 100},
        "Note Mode",
        TriggerType::_NoteOn,
        std::initializer_list<std::pair<char, char>> { { '_', ' ' } }
    };

    chowdsp::BoolParameter::Ptr synchronicTargetBeatSync {
        juce::ParameterID { "sTargetBeatSync", 100},
        "Beat Sync",
        false
    };

    chowdsp::EnumChoiceParameter<TriggerType>::Ptr synchronicTargetBeatSync_noteMode {
        juce::ParameterID{"sTargetBeatSync_noteMode", 100},
        "Note Mode",
        TriggerType::_NoteOn,
        std::initializer_list<std::pair<char, char>> { { '_', ' ' } }
    };

    chowdsp::BoolParameter::Ptr synchronicTargetAddNotes {
        juce::ParameterID { "sTargetAddNotes", 100},
        "Add Notes",
        false
    };

    chowdsp::EnumChoiceParameter<TriggerType>::Ptr synchronicTargetAddNotes_noteMode {
        juce::ParameterID{"sTargetAddNotes_noteMode", 100},
        "Note Mode",
        TriggerType::_NoteOn,
        std::initializer_list<std::pair<char, char>> { { '_', ' ' } }
    };

    chowdsp::BoolParameter::Ptr synchronicTargetClear {
        juce::ParameterID { "sTargetClear", 100},
        "Clear",
        false
    };

    chowdsp::EnumChoiceParameter<TriggerType>::Ptr synchronicTargetClear_noteMode {
        juce::ParameterID{"sTargetClear_noteMode", 100},
        "Note Mode",
        TriggerType::_NoteOn,
        std::initializer_list<std::pair<char, char>> { { '_', ' ' } }
    };

    chowdsp::BoolParameter::Ptr synchronicTargetPausePlay {
        juce::ParameterID { "sTargetPausePlay", 100},
        "Pause/Play",
        false
    };

    chowdsp::EnumChoiceParameter<TriggerType>::Ptr synchronicTargetPausePlay_noteMode {
        juce::ParameterID{"sTargetPausePlay_noteMode", 100},
        "Note Mode",
        TriggerType::_NoteOn,
        std::initializer_list<std::pair<char, char>> { { '_', ' ' } }
    };

//    chowdsp::BoolParameter::Ptr synchronicTargetDeleteOldest {
//        juce::ParameterID { "sTargetDeleteOldest", 100},
//        "Delete Oldest Layer",
//        false
//    };
//
//    chowdsp::EnumChoiceParameter<TriggerType>::Ptr synchronicTargetDeleteOldest_noteMode {
//        juce::ParameterID{"sTargetDeleteOldest_noteMode", 100},
//        "Note Mode",
//        TriggerType::_NoteOn,
//        std::initializer_list<std::pair<char, char>> { { '_', ' ' } }
//    };
//
//    chowdsp::BoolParameter::Ptr synchronicTargetDeleteNewest {
//        juce::ParameterID { "sTargetDeleteNewest", 100},
//        "Delete Newest Layer",
//        false
//    };
//
//    chowdsp::EnumChoiceParameter<TriggerType>::Ptr synchronicTargetDeleteNewest_noteMode {
//        juce::ParameterID{"sTargetDeleteNewest_noteMode", 100},
//        "Note Mode",
//        TriggerType::_NoteOn,
//        std::initializer_list<std::pair<char, char>> { { '_', ' ' } }
//    };
//
//    chowdsp::BoolParameter::Ptr synchronicTargetRotate {
//        juce::ParameterID { "sTargetRotate", 100},
//        "Rotate Layers",
//        false
//    };
//
//    chowdsp::EnumChoiceParameter<TriggerType>::Ptr synchronicTargetRotate_noteMode {
//        juce::ParameterID{"sTargetRotate_noteMode", 100},
//        "Note Mode",
//        TriggerType::_NoteOn,
//        std::initializer_list<std::pair<char, char>> { { '_', ' ' } }
//    };

    /*
     * Resonance Targets
     */

    // pass through all messages on channel 1 by default; same as having no MidiTarget involved
    chowdsp::BoolParameter::Ptr resonanceTargetDefault {
        juce::ParameterID { "rTargetDefault", 100},
        "Default Behavior",
        false
    };

    chowdsp::EnumChoiceParameter<TriggerType>::Ptr resonanceTargetDefault_noteMode {
        juce::ParameterID{"rTargetDefault_noteMode", 100},
        "Note Mode",
        TriggerType::_Both,
        std::initializer_list<std::pair<char, char>> { { '_', ' ' } }
    };

    chowdsp::BoolParameter::Ptr resonanceTargetRing {
        juce::ParameterID { "rTargetRing", 100},
        "Ring Strings",
        false
    };

    chowdsp::EnumChoiceParameter<TriggerType>::Ptr resonanceTargetRing_noteMode {
        juce::ParameterID{"rTargetRing_noteMode", 100},
        "Note Mode",
        TriggerType::_NoteOn,
        std::initializer_list<std::pair<char, char>> { { '_', ' ' } }
    };

    chowdsp::BoolParameter::Ptr resonanceTargetAdd {
        juce::ParameterID { "rTargetAdd", 100},
        "Add/Remove String",
        false
    };

    chowdsp::EnumChoiceParameter<TriggerType>::Ptr resonanceTargetAdd_noteMode {
        juce::ParameterID{"rTargetAdd_noteMode", 100},
        "Note Mode",
        TriggerType::_NoteOn,
        std::initializer_list<std::pair<char, char>> { { '_', ' ' } }
    };

    chowdsp::EnumChoiceParameter<TriggerType>::Ptr nostalgicTargetClear_noteMode {
        juce::ParameterID{"nTargetClear_noteMode", 100},
        "Note Mode",
        TriggerType::_NoteOn,
        std::initializer_list<std::pair<char, char>> { { '_', ' ' } }
    };

    /*
     * Nostalgic Targets
     */

    // pass through all messages on channel 1 by default; same as having no MidiTarget involved
    chowdsp::BoolParameter::Ptr nostalgicTargetDefault {
        juce::ParameterID { "nTargetDefault", 100},
        "Default Behavior",
        false
    };

    chowdsp::BoolParameter::Ptr nostalgicTargetClear {
        juce::ParameterID { "nTargetClear", 100},
        "Clear",
        false
    };

    chowdsp::EnumChoiceParameter<TriggerType>::Ptr nostalgicTargetDefault_noteMode {
        juce::ParameterID{"nTargetDefault_noteMode", 100},
        "Note Mode",
        TriggerType::_Both,
        std::initializer_list<std::pair<char, char>> { { '_', ' ' } }
    };

    /*
     * we store all the targets and their noteModes here, so we can access them
     * as needed in the processBlock loop, by PreparationParameterTargetType
     */
    std::map<PreparationParameterTargetType, chowdsp::BoolParameter*> targetMapper;
    std::map<PreparationParameterTargetType, chowdsp::EnumChoiceParameter<TriggerType>*> noteModeMapper;

    juce::Identifier connectedPrep = IDs::noConnection;

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

class MidiTargetProcessor : public bitklavier::PluginBase<bitklavier::PreparationStateImpl<MidiTargetParams,MidiTargetNonParameterState>>,
public bitklavier::ConnectionList::Listener
{
public:
    MidiTargetProcessor ( SynthBase& parent,const juce::ValueTree& v);

    void prepareToPlay (double sampleRate, int samplesPerBlock) override {};
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    void processAudioBlock (juce::AudioBuffer<float>& buffer) override  {};

    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return true; }
    bool hasEditor() const override { return false; }

    void connectionAdded(bitklavier::Connection*) override;
    void connectionListChanged() override {};
    void removeConnection(bitklavier::Connection*) override;

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

private:
    juce::Array<juce::var> connectedPrepIds;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiTargetProcessor)
};
#endif //BITKLAVIER0_MIDITARGETPROCESSOR_H
