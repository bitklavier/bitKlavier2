// Copyright (C) 2022-2025 Dan Trueman
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ChucKlavierProcessor.h"
#include "synth_base.h"

// VERSION is defined as a preprocessor macro by the JUCE build system ("5.1.0").
// chuck.h has a member `static std::string VERSION` which the macro corrupts.
// Undef before including to avoid the collision.
#undef VERSION
#include "chuck.h"
#include "chuck_errmsg.h"

ChucKlavierProcessor::ChucKlavierProcessor (SynthBase& parent, const juce::ValueTree& vt, juce::UndoManager* um)
    : PluginBase (parent, vt, um, chucKlavierBusLayout())
{
}

// Destructor defined here so std::unique_ptr<ChucK> sees the complete ChucK type.
ChucKlavierProcessor::~ChucKlavierProcessor() = default;

void ChucKlavierProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    if (vm_ && vmSampleRate_ == sampleRate)
    {
        // Same sample rate — just recompile any saved script without recreating the VM.
        // This avoids a full teardown when prepareToPlay is called multiple times.
    }
    else
    {
        vm_.reset();
        vm_ = std::make_unique<ChucK>();

        vm_->setParam (CHUCK_PARAM_SAMPLE_RATE,     (t_CKINT) sampleRate);
        vm_->setParam (CHUCK_PARAM_INPUT_CHANNELS,  (t_CKINT) 2);
        vm_->setParam (CHUCK_PARAM_OUTPUT_CHANNELS, (t_CKINT) 2);
        vm_->setParam (CHUCK_PARAM_VM_HALT,         (t_CKINT) 0);

        // Route ChucK's stderr to our captured string rather than the console.
        vm_->setCherrCallback ([] (const char* msg) {
            // Stateless callback — nothing to do here; callers read EM_lasterror() directly.
            (void) msg;
        });

        vm_->init();
        vm_->start();
        vmSampleRate_ = sampleRate;
    }

    chuckInBuf_.resize  ((size_t) samplesPerBlock * 2, 0.0f);
    chuckOutBuf_.resize ((size_t) samplesPerBlock * 2, 0.0f);

    // Compile the saved script, or the default passthrough if none.
    juce::String savedScript = v.getProperty (IDs::chuckScript, "");
    const std::string script = savedScript.isNotEmpty()
                               ? savedScript.toStdString()
                               : std::string (kDefaultScript);
    vm_->compileCode (script, "");
}

void ChucKlavierProcessor::releaseResources()
{
    vm_.reset();
    vmSampleRate_ = 0.0;
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

    // ChucK VM tick — skip when suspended for hot-swap, or if not yet initialised.
    if (vm_ != nullptr && !vmSuspended_.load (std::memory_order_acquire))
    {
        // Interleave JUCE stereo → ChucK interleaved format
        const float* L = buffer.getReadPointer (0);
        const float* R = buffer.getReadPointer (1);
        for (int i = 0; i < numSamples; ++i)
        {
            chuckInBuf_[size_t(i * 2)]     = L[i];
            chuckInBuf_[size_t(i * 2 + 1)] = R[i];
        }

        vm_->run (chuckInBuf_.data(), chuckOutBuf_.data(), (t_CKINT) numSamples);

        // Deinterleave ChucK output → JUCE stereo
        float* outL = buffer.getWritePointer (0);
        float* outR = buffer.getWritePointer (1);
        for (int i = 0; i < numSamples; ++i)
        {
            outL[i] = (float) chuckOutBuf_[size_t(i * 2)];
            outR[i] = (float) chuckOutBuf_[size_t(i * 2 + 1)];
        }
    }
    else if (vmSuspended_.load (std::memory_order_acquire))
    {
        // Silence output while hot-swap is in progress; acknowledge the park.
        buffer.clear (0, 0, numSamples);
        buffer.clear (1, 0, numSamples);
        vmParked_.store (true, std::memory_order_release);
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
