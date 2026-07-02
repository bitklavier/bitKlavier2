// Copyright (C) 2022-2025 Dan Trueman
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ChucKlavierProcessor.h"
#include "synth_base.h"

ChucKlavierProcessor::ChucKlavierProcessor (SynthBase& parent, const juce::ValueTree& vt, juce::UndoManager* um)
    : PluginBase (parent, vt, um, chucKlavierBusLayout())
{
}

void ChucKlavierProcessor::prepareToPlay (double /*sampleRate*/, int /*samplesPerBlock*/)
{
}

void ChucKlavierProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midiMessages*/)
{
    state.getParameterListeners().callAudioThreadBroadcasters();
    processContinuousModulations (buffer);

    const int numSamples = buffer.getNumSamples();
    const bool muted = state.params.muted_.load (std::memory_order_relaxed);

    // Internal input gain
    auto inputGainMult = bitklavier::utils::dbToMagnitude (state.params.inputGain->getCurrentValue());
    buffer.applyGain (0, 0, numSamples, inputGainMult);
    buffer.applyGain (1, 0, numSamples, inputGainMult);

    std::get<0> (state.params.inputLevels) = buffer.getRMSLevel (0, 0, numSamples);
    std::get<1> (state.params.inputLevels) = buffer.getRMSLevel (1, 0, numSamples);

    // Mix in external audio
    if (externalInputBuffer != nullptr)
    {
        const int extSamples = juce::jmin (numSamples, externalInputBuffer->getNumSamples());
        auto extGainMult = bitklavier::utils::dbToMagnitude (state.params.externalGain->getCurrentValue());

        std::get<0> (state.params.externalLevels) = externalInputBuffer->getRMSLevel (0, 0, extSamples) * extGainMult;
        std::get<1> (state.params.externalLevels) = externalInputBuffer->getRMSLevel (1, 0, extSamples) * extGainMult;

        buffer.addFrom (0, 0, *externalInputBuffer, 0, 0, extSamples, extGainMult);
        buffer.addFrom (1, 0, *externalInputBuffer, 1, 0, extSamples, extGainMult);
    }
    else
    {
        std::get<0> (state.params.externalLevels) = 0.0f;
        std::get<1> (state.params.externalLevels) = 0.0f;
    }

    // Send output
    const int sendBufferIndex = getChannelIndexInProcessBlockBuffer (false, 2, 0);
    auto sendGainMult = muted ? 0.0f : bitklavier::utils::dbToMagnitude (state.params.outputSend->getCurrentValue());
    buffer.copyFrom (sendBufferIndex,     0, buffer.getReadPointer (0), numSamples, sendGainMult);
    buffer.copyFrom (sendBufferIndex + 1, 0, buffer.getReadPointer (1), numSamples, sendGainMult);

    std::get<0> (state.params.sendLevels) = buffer.getRMSLevel (sendBufferIndex,     0, numSamples);
    std::get<1> (state.params.sendLevels) = buffer.getRMSLevel (sendBufferIndex + 1, 0, numSamples);

    // Main output gain
    auto outputGainMult = muted ? 0.0f : bitklavier::utils::dbToMagnitude (state.params.outputGain->getCurrentValue());
    buffer.applyGain (0, 0, numSamples, outputGainMult);
    buffer.applyGain (1, 0, numSamples, outputGainMult);

    std::get<0> (state.params.outputLevels) = buffer.getRMSLevel (0, 0, numSamples);
    std::get<1> (state.params.outputLevels) = buffer.getRMSLevel (1, 0, numSamples);
}
