// Copyright (C) 2022-2025 Dan Trueman
// SPDX-License-Identifier: GPL-3.0-or-later

//
// VSTModulationBridge.cpp
//

#include "VSTModulationBridge.h"
#include "../../../common/Identifiers.h"
#include "../../synth_base.h"

VSTModulationBridge::VSTModulationBridge (juce::AudioPluginInstance* plugin,
                                           juce::ValueTree bridgeState,
                                           SynthBase& parent)
    // Bus layout: one "Modulation" input bus with kMaxVSTModParams * 2 discrete channels.
    //   channels [0 .. N-1]   = state (ramp) modulation offsets
    //   channels [N .. 2N-1]  = continuous (LFO) modulation offsets
    : juce::AudioProcessor (BusesProperties()
          .withInput ("Modulation",
                      juce::AudioChannelSet::discreteChannels (kMaxVSTModParams * 2),
                      true)),
      plugin_ (plugin),
      state_ (std::move (bridgeState)),
      parent_ (parent)
{
    for (auto& v : baseValues_)
        v.store (0.0f, std::memory_order_relaxed);
    for (auto& v : modulatedValues_)
        v.store (0.0f, std::memory_order_relaxed);
    offsetBankIndices_.fill (-1);

    // Collect automatable parameter indices from the plugin.
    auto& params = plugin_->getParameters();
    for (int i = 0; i < params.size() && (int) automatableParamIndices_.size() < kMaxVSTModParams; ++i)
        if (params[i]->isAutomatable())
            automatableParamIndices_.push_back (i);
}

void VSTModulationBridge::setupModulatableParams()
{
    // Called on the message thread.
    auto modParams = state_.getOrCreateChildWithName (IDs::MODULATABLE_PARAMS, nullptr);
    modParams.removeAllChildren (nullptr);
    DBG ("[VSTBridge] setupModulatableParams: uuid=" + state_.getProperty (IDs::uuid).toString()
         + "  numAutomatable=" + juce::String ((int) automatableParamIndices_.size()));

    auto& params = plugin_->getParameters();
    for (int slot = 0; slot < (int) automatableParamIndices_.size(); ++slot)
    {
        const int vstParamIdx = automatableParamIndices_[slot];

        // Capture the current normalized value as the base (pre-modulation) value.
        const float initialBase = params[vstParamIdx]->getValue();
        baseValues_[slot].store (initialBase, std::memory_order_relaxed);
        modulatedValues_[slot].store (initialBase, std::memory_order_relaxed);

        // MODULATABLE_PARAM entry: parameter ID is the slot index as a string.
        // Using the slot index (never contains underscores) keeps connectModulation's
        // "<uuid>_<param>" parsing unambiguous regardless of the VST's param name.
        // sliderval is initialized to the base value so that currentDestinationSliderVal
        // in ModulationConnection is correct for Reset operations.
        auto entry = juce::ValueTree (IDs::MODULATABLE_PARAM);
        entry.setProperty (IDs::parameter, juce::String (slot), nullptr);
        entry.setProperty (IDs::start,     0.0f, nullptr);
        entry.setProperty (IDs::end,       1.0f, nullptr);
        entry.setProperty (IDs::skew,      1.0f, nullptr);
        entry.setProperty (IDs::sliderval, initialBase, nullptr);
        modParams.appendChild (entry, nullptr);

        // Register "<bridgeUuid>_<slot>" in the ParamOffsetBank so that
        // connectModulation can find the carry/reset index for this destination.
        // Initialize to the base value so that updateScalingAudioThread sees the
        // correct currentTotalParamUnits on the first trigger.
        offsetBankIndices_[slot] = parent_.getParamOffsetBank().addParam (
            state_, juce::String (slot), initialBase);
    }
}

void VSTModulationBridge::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    if (automatableParamIndices_.empty())
        return;

    // Find the modulation input bus.
    int modBusIdx = -1;
    for (int i = 0; i < getBusCount (true); ++i)
    {
        if (getBus (true, i)->getName() == "Modulation")
        {
            modBusIdx = i;
            break;
        }
    }
    if (modBusIdx == -1)
        return;

    const auto& modBus = getBusBuffer (buffer, true, modBusIdx);
    const int numChannels = modBus.getNumChannels();

    auto& vstParams = plugin_->getParameters();

    for (int slot = 0; slot < kMaxVSTModParams && slot < (int) automatableParamIndices_.size(); ++slot)
    {
        // Use the first sample of each channel as the block-level modulation value.
        // The bus is laid out as: channels [0..kMaxVSTModParams-1]  = state offsets,
        //                          channels [kMaxVSTModParams..2N-1] = continuous offsets.
        const float stateOff = (slot < numChannels)
                               ? modBus.getReadPointer (slot)[0] : 0.0f;
        const float contOff  = (slot + kMaxVSTModParams < numChannels)
                               ? modBus.getReadPointer (slot + kMaxVSTModParams)[0] : 0.0f;
        const float totalOff = stateOff + contOff;

        const bool isActive = std::abs (totalOff) > 1e-6f;
        // Log first non-zero modulation signal for each slot (one-shot per slot)
        if (isActive && ! wasModulating_[slot])
            DBG ("[VSTBridge] slot=" + juce::String (slot)
                 + "  stateOff=" + juce::String (stateOff)
                 + "  contOff=" + juce::String (contOff));
        const float base = baseValues_[slot].load (std::memory_order_relaxed);

        // Only call setValue when there is a non-zero modulation offset, or in the
        // one block immediately after modulation returns to zero (so the parameter
        // snaps cleanly back to its base value rather than being left at ~base).
        // When totalOff == 0 and was already 0 last block, don't touch the parameter
        // — this avoids forcing parameters to their captured base values when idle,
        // which would override any changes made via the native VST UI.
        const float target = juce::jlimit (0.0f, 1.0f, base + totalOff);
        if (isActive || wasModulating_[slot])
        {
            const int vstParamIdx = automatableParamIndices_[slot];
            if (vstParamIdx < vstParams.size())
                vstParams[vstParamIdx]->setValue (target);
        }
        // Always keep modulatedValues_ current so VSTParametersView's polling
        // timer can display the live value without relying on parameterValueChanged.
        modulatedValues_[slot].store (target, std::memory_order_relaxed);

        // Always update the ParamOffsetBank with the current modulated value.
        // ModulationConnection::updateScalingAudioThread reads this as
        // currentTotalParamUnits on retrigger. Without this, it always sees 0
        // (the initial value), causing the anchor computation to be wrong and
        // scalingValue_ to accumulate on each retrigger.
        if (offsetBankIndices_[slot] >= 0)
            parent_.getParamOffsetBank().setOffset (offsetBankIndices_[slot], base + totalOff);

        wasModulating_[slot] = isActive;
    }
}
