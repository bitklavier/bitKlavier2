//
// Created by Davis Polito on 6/19/24.
//

#ifndef BITKLAVIER2_KEYMAPPROCESSOR_H
#define BITKLAVIER2_KEYMAPPROCESSOR_H

#include "json.hpp"
#include "midi_manager.h"
#include "PreparationStateImpl.h"
#include "PluginBase.h"

struct KeymapKeyboardState {
    KeymapKeyboardState() {
        keyStates.set();
    }
    std::bitset<128> harmonizationState;
    std::bitset<128> keyToHarmonize;
    std::bitset<128> keyStates;
};

struct KeymapParams : chowdsp::ParamHolder
{
    KeymapParams() : chowdsp::ParamHolder("keymap")
    {
    }

    KeymapKeyboardState keyboard_state;
    /** Custom serializer */
    template <typename Serializer>
    static typename Serializer::SerializedType serialize (const KeymapParams& paramHolder);

    /** Custom deserializer */
    template <typename Serializer>
    static void deserialize (typename Serializer::DeserializedType deserial, KeymapParams& paramHolder);
};

struct KeymapNonParameterState : chowdsp::NonParamState
{
    KeymapNonParameterState()
    {
        //addStateValues ({ &prepPoint });
    }

    //chowdsp::StateValue<juce::Point<int>> prepPoint { "prep_point", { 300, 500 } };
};

class KeymapProcessor : public bitklavier::PluginBase<bitklavier::PreparationStateImpl<KeymapParams,KeymapNonParameterState>>
{
public:
    KeymapProcessor(const juce::ValueTree& v, SynthBase& parent);
    static std::unique_ptr<juce::AudioProcessor> create(SynthBase& parent,const juce::ValueTree& v) {
        return std::make_unique<KeymapProcessor>(v,parent);
    }
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processAudioBlock (juce::AudioBuffer<float>& buffer) override  {};
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    bool hasEditor() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    juce::AudioProcessor::BusesProperties  keymapBusLayout() { return BusesProperties();}
    bool setMidiDevice(juce::AudioDeviceManager& deviceManager, juce::String identifier) { deviceManager.addMidiInputDeviceCallback(identifier, static_cast<juce::MidiInputCallback*>(_midi.get()));}

    std::unique_ptr<MidiManager> _midi;

private:
    juce::MidiKeyboardState keyboard_state;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KeymapProcessor)
};


#endif //BITKLAVIER2_KEYMAPPROCESSOR_H
