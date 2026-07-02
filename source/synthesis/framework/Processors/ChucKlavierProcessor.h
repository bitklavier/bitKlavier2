// Copyright (C) 2022-2025 Dan Trueman
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "IMuteSolable.h"
#include "PluginBase.h"
#include "Identifiers.h"
#include <PreparationStateImpl.h>
#include <chowdsp_plugin_base/chowdsp_plugin_base.h>
#include <chowdsp_plugin_utils/chowdsp_plugin_utils.h>
#include <chowdsp_plugin_state/chowdsp_plugin_state.h>
#include <chowdsp_serialization/chowdsp_serialization.h>
#include "utils.h"

struct ChucKlavierParams : chowdsp::ParamHolder
{
    float rangeStart = -80.0f;
    float rangeEnd   =   6.0f;
    float skewFactor =   2.0f;

    ChucKlavierParams (const juce::ValueTree& /*v*/) : chowdsp::ParamHolder ("chucklavier")
    {
        add (outputGain, inputGain, externalGain, outputSend);

        doForAllParameters ([this] (auto& param, size_t) {
            if (auto* sliderParam = dynamic_cast<chowdsp::FloatParameter*> (&param))
                if (sliderParam->supportsMonophonicModulation())
                    modulatableParams.push_back (sliderParam);
        });
    }

    chowdsp::GainDBParameter::Ptr inputGain {
        juce::ParameterID { "ChucKInputGain", 100 },
        "Internal Gain",
        juce::NormalisableRange { rangeStart, rangeEnd, 0.0f, skewFactor, false },
        0.0f,
        true
    };

    chowdsp::GainDBParameter::Ptr externalGain {
        juce::ParameterID { "ChucKExternalGain", 100 },
        "External Gain",
        juce::NormalisableRange { rangeStart, rangeEnd, 0.0f, skewFactor, false },
        rangeStart,
        true
    };

    chowdsp::GainDBParameter::Ptr outputSend {
        juce::ParameterID { "ChucKSend", 100 },
        "Send",
        juce::NormalisableRange { rangeStart, rangeEnd, 0.0f, skewFactor, false },
        0.0f,
        true
    };

    chowdsp::GainDBParameter::Ptr outputGain {
        juce::ParameterID { "ChucKOutputGain", 100 },
        "Output Gain",
        juce::NormalisableRange { rangeStart, rangeEnd, 0.0f, skewFactor, false },
        0.0f,
        true
    };

    std::tuple<std::atomic<float>, std::atomic<float>> outputLevels;
    std::tuple<std::atomic<float>, std::atomic<float>> sendLevels;
    std::tuple<std::atomic<float>, std::atomic<float>> inputLevels;
    std::tuple<std::atomic<float>, std::atomic<float>> externalLevels;

    std::atomic<bool> muted_    { false };
    std::atomic<bool> userMuted_{ false };
    std::atomic<bool> soloed_   { false };
    std::atomic<bool> soloMuted_{ false };
};

struct ChucKlavierNonParameterState : chowdsp::NonParamState
{
    ChucKlavierNonParameterState() = default;
};

class ChucKlavierProcessor
    : public bitklavier::PluginBase<bitklavier::PreparationStateImpl<ChucKlavierParams, ChucKlavierNonParameterState>>,
      public bitklavier::ExternalAudioInputReceiver,
      public IMuteSolable
{
public:
    ChucKlavierProcessor (SynthBase& parent, const juce::ValueTree& v, juce::UndoManager*);

    std::atomic<bool>& getMuted()     override { return state.params.muted_; }
    std::atomic<bool>& getUserMuted() override { return state.params.userMuted_; }
    std::atomic<bool>& getSoloed()    override { return state.params.soloed_; }
    std::atomic<bool>& getSoloMuted() override { return state.params.soloMuted_; }

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processAudioBlock (juce::AudioBuffer<float>& buffer) override {}
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    void processBlockBypassed (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override {}

    void setExternalInputBuffer (const juce::AudioBuffer<float>* buf) override { externalInputBuffer = buf; }

    bool hasEditor() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }

    juce::AudioProcessor::BusesProperties chucKlavierBusLayout()
    {
        return BusesProperties()
            .withOutput ("Output",     juce::AudioChannelSet::stereo(), true)
            .withInput  ("Input",      juce::AudioChannelSet::stereo(), true)
            .withInput  ("Send Pad",   juce::AudioChannelSet::stereo(), true)
            .withInput  ("Modulation", juce::AudioChannelSet::discreteChannels (4 * 2), true)
            .withOutput ("Modulation", juce::AudioChannelSet::mono(), false)
            .withOutput ("Send",       juce::AudioChannelSet::stereo(), true);
    }

    bool isBusesLayoutSupported (const juce::AudioProcessor::BusesLayout&) const override { return true; }

private:
    const juce::AudioBuffer<float>* externalInputBuffer = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChucKlavierProcessor)
};
