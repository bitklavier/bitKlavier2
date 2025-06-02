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


    /****************************************************************************************/

    KeymapParams() : chowdsp::ParamHolder("keymap")
    {
    }

//    };

    KeymapKeyboardState keyboard_state;

    /****************************************************************************************/
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
    KeymapProcessor(const juce::ValueTree& v, juce::AudioDeviceManager* manager);

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processAudioBlock (juce::AudioBuffer<float>& buffer) override  {};

    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    juce::MidiMessage swapNoteOnNoteOff (juce::MidiMessage inmsg);

    bool acceptsMidi() const override
    {
        return false;
    }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    bool hasEditor() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    juce::AudioProcessor::BusesProperties  keymapBusLayout()
    {
        return BusesProperties();
    }
    bool setMidiDevice(juce::AudioDeviceManager& deviceManager, juce::String identifier)
    {
        deviceManager.addMidiInputDeviceCallback(identifier, static_cast<juce::MidiInputCallback*>(_midi.get()));
    }

    // user settings
    void setInvertNoteOnNoteOff(bool invert) { invertNoteOnNoteOff = invert; }

    std::unique_ptr<MidiManager> _midi;
private:
    juce::MidiKeyboardState keyboard_state;

    // user settings
    bool invertNoteOnNoteOff = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KeymapProcessor)
};


#endif //BITKLAVIER2_KEYMAPPROCESSOR_H
