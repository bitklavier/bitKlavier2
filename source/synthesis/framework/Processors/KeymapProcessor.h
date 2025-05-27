//
// Created by Davis Polito on 6/19/24.
//

#ifndef BITKLAVIER2_KEYMAPPROCESSOR_H
#define BITKLAVIER2_KEYMAPPROCESSOR_H

#include <chowdsp_plugin_base/chowdsp_plugin_base.h>
#include <chowdsp_plugin_utils/chowdsp_plugin_utils.h>
#include <chowdsp_sources/chowdsp_sources.h>
#include <chowdsp_plugin_state/chowdsp_plugin_state.h>
#include <juce_audio_devices/juce_audio_devices.h>

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
//        add (gainParam,
//             sustainParam,
//             attackParam
//        );
    }

//    // juce::Gain param
//    chowdsp::GainDBParameter::Ptr gainParam {
//            juce::ParameterID { "gain", 100 },
//            "Gain",
//            juce::NormalisableRange { -30.0f, 0.0f },
//            -24.0f
//    };
//
//    // Float param
//    chowdsp::FloatParameter::Ptr sustainParam {
//            juce::ParameterID { "sustain", 100 },
//            "Sustain",
//            chowdsp::ParamUtils::createNormalisableRange (20.0f, 20000.0f, 2000.0f),
//            1000.0f,
//            &chowdsp::ParamUtils::floatValToString,
//            &chowdsp::ParamUtils::stringToFloatVal
//    };
//
//    chowdsp::TimeMsParameter::Ptr attackParam {
//            juce::ParameterID { "attack", 100 },
//            "attack",
//            chowdsp::ParamUtils::createNormalisableRange (2.01f, 10.0f, 4.0f),
//            3.5f,
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

class KeymapProcessor : public bitklavier::PluginBase<bitklavier::PreparationStateImpl<KeymapParams,KeymapNonParameterState,chowdsp::XMLSerializer>>
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
    //chowdsp::experimental::Keymapillator<float> oscillator;
    chowdsp::Gain<float> gain;
    juce::MidiKeyboardState keyboard_state;

    // user settings
    bool invertNoteOnNoteOff = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KeymapProcessor)
};


#endif //BITKLAVIER2_KEYMAPPROCESSOR_H
