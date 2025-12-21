//
// Created by Davis Polito on 6/19/24.
//

#ifndef BITKLAVIER2_KEYMAPPROCESSOR_H
#define BITKLAVIER2_KEYMAPPROCESSOR_H

#include "json.hpp"
#include "midi_manager.h"
#include "PreparationStateImpl.h"
#include "PluginBase.h"
#include "utils.h"
#include "VelocityMinMaxParams.h"

//struct KeymapKeyboardState
//{
//    KeymapKeyboardState() {
//        keyStates.set();
//    }
//
//    std::bitset<128> keyStates;
//};

struct KeymapParams : chowdsp::ParamHolder
{
    KeymapParams(const juce::ValueTree &v) : chowdsp::ParamHolder("keymap")
    {
        add(velocityCurve_asymWarp,
            velocityCurve_symWarp,
            velocityCurve_scale,
            velocityCurve_offset,
            velocityCurve_invert,
            velocityMinMax);

        // params that are audio-rate modulatable are added to vector of all continuously modulatable params
        doForAllParameters ([this] (auto& param, size_t) {
            if (auto* sliderParam = dynamic_cast<chowdsp::ChoiceParameter*> (&param))
                if (sliderParam->supportsMonophonicModulation())
                    modulatableParams.push_back ( sliderParam);

            if (auto* sliderParam = dynamic_cast<chowdsp::BoolParameter*> (&param))
                if (sliderParam->supportsMonophonicModulation())
                    modulatableParams.push_back ( sliderParam);

            if (auto* sliderParam = dynamic_cast<chowdsp::FloatParameter*> (&param))
                if (sliderParam->supportsMonophonicModulation())
                    modulatableParams.push_back ( sliderParam);
        });
    }

    chowdsp::FloatParameter::Ptr velocityCurve_asymWarp {
        juce::ParameterID { "velocityCurve_asymWarp", 100 },
        "CENTER WARP",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 10.0f, 1.0f),
        1.0f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal,
        true
    };

    chowdsp::FloatParameter::Ptr velocityCurve_symWarp {
        juce::ParameterID { "velocityCurve_symWarp", 100 },
        "SIDES WARP",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 5.0f, 1.0f),
        1.0f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal,
        true
    };

    chowdsp::FloatParameter::Ptr velocityCurve_scale {
        juce::ParameterID { "velocityCurve_scale", 100 },
        "SCALE",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 10.0f, 1.0f),
        1.0f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal,
        true
    };

    chowdsp::FloatParameter::Ptr velocityCurve_offset {
        juce::ParameterID { "velocityCurve_offset", 100 },
        "OFFSET",
        chowdsp::ParamUtils::createNormalisableRange (-1.0f, 1.0f, 0.0f),
        0.0f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal,
        true
    };

    chowdsp::BoolParameter::Ptr velocityCurve_invert {
        juce::ParameterID { "velocityCurve_invert", 100 },
        "velocityCurve_invert",
        false
    };

    KeymapKeyboardState keyboard_state;
    VelocityMinMaxParams velocityMinMax;

    std::atomic<float> invelocity;
    std::atomic<float> warpedvelocity;

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
    }
};

class KeymapProcessor : public bitklavier::PluginBase<bitklavier::PreparationStateImpl<KeymapParams, KeymapNonParameterState>>
{
public:
    KeymapProcessor( SynthBase& parent,const juce::ValueTree& v);
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processAudioBlock (juce::AudioBuffer<float>& buffer) override  {};
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    void processBlockBypassed (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    bool hasEditor() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    juce::AudioProcessor::BusesProperties  keymapBusLayout() { return BusesProperties();}
    bool setMidiDevice(juce::AudioDeviceManager& deviceManager, juce::String identifier) { deviceManager.addMidiInputDeviceCallback(identifier, static_cast<juce::MidiInputCallback*>(_midi.get()));}
    void allNotesOff();
    std::unique_ptr<MidiManager> _midi;

    float applyVelocityCurve(float velocity);
    bool checkVelocityRange(float velocity);

private:
    juce::MidiKeyboardState keyboard_state;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KeymapProcessor)
};


#endif //BITKLAVIER2_KEYMAPPROCESSOR_H
