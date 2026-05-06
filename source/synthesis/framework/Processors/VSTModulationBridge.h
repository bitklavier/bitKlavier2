// Copyright (C) 2022-2025 Dan Trueman
// SPDX-License-Identifier: GPL-3.0-or-later

//
// VSTModulationBridge.h
//
// A thin AudioProcessor node inserted between a ModulationProcessor and a VST
// plugin in the AudioProcessorGraph. It receives modulation bus data and applies
// it to the VST's parameters via setValue() once per block (block-accurate).
//
// Graph topology:
//   ModulationProcessor --[mod bus]--> VSTModulationBridge --> (MIDI ordering) --> VST Plugin
//
// The bridge does NOT route audio; it only applies modulation to VST parameters.
// Graph ordering is enforced by the modulation bus connection (ModProc → Bridge)
// and a MIDI ordering edge (Bridge → VST).

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

class SynthBase;

class VSTModulationBridge final : public juce::AudioProcessor
{
public:
    // Maximum number of simultaneously modulatable VST parameters.
    static constexpr int kMaxVSTModParams = 32;

    VSTModulationBridge (juce::AudioPluginInstance* plugin,
                         juce::ValueTree bridgeState,
                         SynthBase& parent);

    // Must be called on the message thread after construction.
    // Populates the MODULATABLE_PARAMS sub-tree on the bridge ValueTree and
    // registers each parameter slot in the ParamOffsetBank.
    void setupModulatableParams();

    const juce::ValueTree& getBridgeState() const { return state_; }
    int getNumAutomatableParams() const { return (int) automatableParamIndices_.size(); }

    // Called from the message thread (VSTParamAttachment) when the user moves a slider
    // so that the base value is updated before the next processBlock reads it.
    void setBaseValue (int slot, float normalizedValue)
    {
        if (slot >= 0 && slot < kMaxVSTModParams)
            baseValues_[slot].store (normalizedValue, std::memory_order_relaxed);
    }

    float getBaseValue (int slot) const
    {
        if (slot >= 0 && slot < kMaxVSTModParams)
            return baseValues_[slot].load (std::memory_order_relaxed);
        return 0.0f;
    }

    // Returns the current modulated value for a slot (base + total modulation offset).
    // Written on the audio thread in processBlock; read on the message thread for UI
    // updates.  Initialized to the base value in setupModulatableParams().
    float getModulatedValue (int slot) const
    {
        if (slot >= 0 && slot < kMaxVSTModParams)
            return modulatedValues_[slot].load (std::memory_order_relaxed);
        return 0.0f;
    }

    // Indices into plugin->getParameters() for each automatable param (slot 0..N-1).
    // Slot index == modulation bus channel index used by connectModulation.
    std::vector<int> automatableParamIndices_;

    // -------------------------------------------------------------------------
    // AudioProcessor interface
    const juce::String getName() const override { return "VSTModulationBridge"; }
    void prepareToPlay (double, int) override {}
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlock (juce::AudioBuffer<double>&, juce::MidiBuffer&) override {}
    double getTailLengthSeconds() const override { return 0.0; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool hasEditor() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override {}
    void setStateInformation (const void*, int) override {}

private:
    juce::AudioPluginInstance* plugin_; // non-owning
    juce::ValueTree state_;
    SynthBase& parent_;

    // Per-slot base values (normalized 0..1).
    // Written on the message thread when a modulation connection is established,
    // read on the audio thread in processBlock.
    std::array<std::atomic<float>, kMaxVSTModParams> baseValues_;

    // Per-slot current modulated values (base + total offset, clamped 0..1).
    // Written on the audio thread in processBlock; read on the message thread
    // by VSTParametersView's polling timer to update the live-value display.
    std::array<std::atomic<float>, kMaxVSTModParams> modulatedValues_;

    // True for each slot that had a non-zero totalOff last block, so we can
    // issue one final setValue(base) call when modulation returns to zero.
    std::array<bool, kMaxVSTModParams> wasModulating_ {};

    // Index of each slot's entry in the SynthBase ParamOffsetBank.
    // Written once on the message thread (setupModulatableParams), read on the audio
    // thread (processBlock). Safe because setup completes before audio starts.
    // -1 means not registered (slot unused).
    std::array<int, kMaxVSTModParams> offsetBankIndices_;

};
