// Copyright (C) 2022-2025 Dan Trueman
// SPDX-License-Identifier: GPL-3.0-or-later

/*
 * ReverbProcessor — Hall reverb bus processor for bitKlavier
 * DSP adapted from Dragonfly Hall Reverb by Michael Willis and Rob van den Berg
 * https://github.com/michaelwillis/dragonfly-reverb  (GPL-3.0)
 * Uses freeverb3 by Jezar at Dreampoint / Teru Kamogashira (GPL-2.0+)
 */

#include "ReverbProcessor.h"
#include "synth_base.h"
#include <juce_audio_basics/juce_audio_basics.h>

ReverbProcessor::ReverbProcessor (SynthBase& parent, const juce::ValueTree& vt, juce::UndoManager* um)
    : PluginBase (parent, vt, um, reverbBusLayout())
{
    this->v.getOrCreateChildWithName (IDs::PARAM_DEFAULT, nullptr);
    // Reverb params are serialized as properties directly on v (e.g. reverbDry, reverbDecay).
    // If any such property exists, PluginBase has already restored state and we must not
    // overwrite it with the default preset.
    const bool hasSavedState = this->v.hasProperty ("reverbDry");

    // Initialise freeverb3 objects (mirroring DragonflyReverbDSP constructor)
    early_.loadPresetReflection (FV3_EARLYREF_PRESET_1);
    early_.setMuteOnChange (false);
    early_.setdryr (0);    // mute dry pass-through inside early processor
    early_.setwet (0);     // 0 dB wet
    early_.setwidth (0.8);
    early_.setLRDelay (0.3);
    early_.setLRCrossApFreq (750, 4);
    early_.setDiffusionApFreq (150, 4);
    early_.setSampleRate (sampleRate_);
    early_send_ = 0.20f;

    late_.setMuteOnChange (false);
    late_.setwet (0);
    late_.setdryr (0);
    late_.setwidth (1.0);
    late_.setSampleRate (sampleRate_);

    // Initialise param arrays — force update on first processBlock by setting oldParams to -1
    for (int i = 0; i < kReverbParamCount; ++i)
    {
        newParams_[i] = kReverbPresets[kReverbDefaultPreset - 1].params[i];
        oldParams_[i] = -1.f;
    }

    // Apply default preset values to chowdsp params only when there is no saved state;
    // if state was already deserialized from the ValueTree by PluginBase, skip this so
    // we don't overwrite the restored parameter values.
    if (!hasSavedState)
        applyPreset (kReverbDefaultPreset);

    // ── Listeners ──────────────────────────────────────────────────────

    // MessageThread: detect user edits (vs. preset application) and mark Custom
    auto setCustomIfUserEdit = [this]()
    {
        if (juce::Time::currentTimeMillis() - presetAppliedAtMs_ > 200)
            setPresetIndex (0);
    };

    for (auto* param : {
        static_cast<juce::RangedAudioParameter*> (state.params.dryLevel.get()),
        static_cast<juce::RangedAudioParameter*> (state.params.earlyLevel.get()),
        static_cast<juce::RangedAudioParameter*> (state.params.lateLevel.get()),
        static_cast<juce::RangedAudioParameter*> (state.params.size.get()),
        static_cast<juce::RangedAudioParameter*> (state.params.width.get()),
        static_cast<juce::RangedAudioParameter*> (state.params.predelay.get()),
        static_cast<juce::RangedAudioParameter*> (state.params.diffuse.get()),
        static_cast<juce::RangedAudioParameter*> (state.params.lowCut.get()),
        static_cast<juce::RangedAudioParameter*> (state.params.lowXover.get()),
        static_cast<juce::RangedAudioParameter*> (state.params.lowMult.get()),
        static_cast<juce::RangedAudioParameter*> (state.params.highCut.get()),
        static_cast<juce::RangedAudioParameter*> (state.params.highXover.get()),
        static_cast<juce::RangedAudioParameter*> (state.params.highMult.get()),
        static_cast<juce::RangedAudioParameter*> (state.params.spin.get()),
        static_cast<juce::RangedAudioParameter*> (state.params.wander.get()),
        static_cast<juce::RangedAudioParameter*> (state.params.decay.get()),
        static_cast<juce::RangedAudioParameter*> (state.params.earlySend.get()),
        static_cast<juce::RangedAudioParameter*> (state.params.modulation.get()) })
    {
        reverbCallbacks_ += { state.getParameterListeners().addParameterListener (
            *param,
            chowdsp::ParameterListenerThread::MessageThread,
            setCustomIfUserEdit) };
    }
}

void ReverbProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    sampleRate_ = sampleRate;
    early_.setSampleRate (sampleRate);
    late_.setSampleRate (sampleRate);
    // Force all params to re-apply on next processBlock
    for (int i = 0; i < kReverbParamCount; ++i)
        oldParams_[i] = -1.f;
}

bool ReverbProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void ReverbProcessor::applyPreset (int idx)
{
    if (idx < 1 || idx > 25) return;
    const float* p = kReverbPresets[idx - 1].params;

    // Timestamp BEFORE setParameterValue so the MT "setCustomIfUserEdit" guard triggers correctly
    presetAppliedAtMs_ = juce::Time::currentTimeMillis();

    state.params.dryLevel->setParameterValue    (p[0]);
    state.params.earlyLevel->setParameterValue  (p[1]);
    state.params.lateLevel->setParameterValue   (p[2]);
    state.params.size->setParameterValue        (p[3]);
    state.params.width->setParameterValue       (p[4]);
    state.params.predelay->setParameterValue    (p[5]);
    state.params.diffuse->setParameterValue     (p[6]);
    state.params.lowCut->setParameterValue      (p[7]);
    state.params.lowXover->setParameterValue    (p[8]);
    state.params.lowMult->setParameterValue     (p[9]);
    state.params.highCut->setParameterValue     (p[10]);
    state.params.highXover->setParameterValue   (p[11]);
    state.params.highMult->setParameterValue    (p[12]);
    state.params.spin->setParameterValue        (p[13]);
    state.params.wander->setParameterValue      (p[14]);
    state.params.decay->setParameterValue       (p[15]);
    state.params.earlySend->setParameterValue   (p[16]);
    state.params.modulation->setParameterValue  (p[17]);

    setPresetIndex (idx);
}

void ReverbProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midi*/)
{
    state.getParameterListeners().callAudioThreadBroadcasters();
    // Bus processors are called directly from processAudioAndMidi with a plain stereo buffer,
    // not through the AudioProcessorGraph, so the modulation bus channels are not present in the
    // buffer. processContinuousModulations would read out-of-bounds. Modulation of bus processor
    // params requires routing them through the graph first.
    if (v.getType() != IDs::BUSREVERB)
        processContinuousModulations (buffer);
    state.params.processStateChanges();


    // mix in external audio (mic/line in standalone, DAW sidechain in plugin)
    if (externalInputBuffer != nullptr)
    {
        const int extSamples = juce::jmin (buffer.getNumSamples(), externalInputBuffer->getNumSamples());
        const auto extgainmult = bitklavier::utils::dbToMagnitude (state.params.externalGain->getCurrentValue());
        buffer.addFrom (0, 0, *externalInputBuffer, 0, 0, extSamples, extgainmult);
        buffer.addFrom (1, 0, *externalInputBuffer, 1, 0, extSamples, extgainmult);
        std::get<0> (state.params.externalLevels) = externalInputBuffer->getRMSLevel (0, 0, extSamples) * extgainmult;
        std::get<1> (state.params.externalLevels) = externalInputBuffer->getRMSLevel (1, 0, extSamples) * extgainmult;
    }
    else
    {
        std::get<0> (state.params.externalLevels) = 0.0f;
        std::get<1> (state.params.externalLevels) = 0.0f;
    }

    if (! state.params.activeReverb->get())
        return;

    const int numSamples = buffer.getNumSamples();

    // Pull current param values from chowdsp params into newParams_[]
    newParams_[0]  = *state.params.dryLevel;
    newParams_[1]  = *state.params.earlyLevel;
    newParams_[2]  = *state.params.lateLevel;
    newParams_[3]  = *state.params.size;
    newParams_[4]  = *state.params.width;
    newParams_[5]  = *state.params.predelay;
    newParams_[6]  = *state.params.diffuse;
    newParams_[7]  = *state.params.lowCut;
    newParams_[8]  = *state.params.lowXover;
    newParams_[9]  = *state.params.lowMult;
    newParams_[10] = *state.params.highCut;
    newParams_[11] = *state.params.highXover;
    newParams_[12] = *state.params.highMult;
    newParams_[13] = *state.params.spin;
    newParams_[14] = *state.params.wander;
    newParams_[15] = *state.params.decay;
    newParams_[16] = *state.params.earlySend;
    newParams_[17] = *state.params.modulation;

    // Input gain
    const float inGain = bitklavier::utils::dbToMagnitude (*state.params.inputGain);
    buffer.applyGain (0, 0, numSamples, inGain);
    buffer.applyGain (1, 0, numSamples, inGain);

    std::get<0> (state.params.inputLevels) = buffer.getRMSLevel (0, 0, numSamples);
    std::get<1> (state.params.inputLevels) = buffer.getRMSLevel (1, 0, numSamples);

    // Apply any changed params to the freeverb3 objects (adapted from DSP.cpp)
    for (int index = 0; index < kReverbParamCount; ++index)
    {
        if (oldParams_[index] == newParams_[index]) continue;
        oldParams_[index] = newParams_[index];
        float value = newParams_[index];

        switch (index)
        {
            case 0:  dryLevel_   = value / 100.f; break;
            case 1:  earlyLevel_ = value / 100.f; break;
            case 2:  lateLevel_  = value / 100.f; break;
            case 3:  early_.setRSFactor (value / 10.f);
                     late_.setRSFactor  (value / 80.f); break;
            case 4:  early_.setwidth (value / 100.f);
                     late_.setwidth  (value / 100.f); break;
            case 5:  late_.setPreDelay (value < 0.1f ? 0.1f : value); break;
            case 6:  late_.setidiffusion1 (value / 140.f);
                     late_.setapfeedback  (value / 140.f); break;
            case 7:  early_.setoutputhpf (value);
                     late_.setoutputhpf  (value); break;
            case 8:  late_.setxover_low  (value); break;
            case 9:  late_.setrt60_factor_low (value); break;
            case 10: early_.setoutputlpf (value);
                     late_.setoutputlpf  (value); break;
            case 11: late_.setxover_high (value); break;
            case 12: late_.setrt60_factor_high (value); break;
            case 13: late_.setspin   (value); break;
            case 14: late_.setwander (value); break;
            case 15: late_.setrt60   (value); break;
            case 16: early_send_ = value / 100.f; break;
            case 17: {
                float mod = value == 0.f ? 0.001f : value / 100.f;
                late_.setspinfactor (mod);
                late_.setlfofactor  (mod);
                break;
            }
            default: break;
        }
    }

    // Process in 256-sample chunks (freeverb3 internal requirement)
    const float* inputL  = buffer.getReadPointer  (0);
    const float* inputR  = buffer.getReadPointer  (1);
    float*       outputL = buffer.getWritePointer (0);
    float*       outputR = buffer.getWritePointer (1);

    for (uint32_t offset = 0; offset < (uint32_t) numSamples; offset += kBufSize)
    {
        const uint32_t frames = juce::jmin ((uint32_t) numSamples - offset, kBufSize);

        early_.processreplace (
            const_cast<float*> (inputL + offset),
            const_cast<float*> (inputR + offset),
            earlyOutBuf_[0], earlyOutBuf_[1], (long) frames);

        for (uint32_t i = 0; i < frames; ++i)
        {
            lateInBuf_[0][i] = early_send_ * earlyOutBuf_[0][i] + inputL[offset + i];
            lateInBuf_[1][i] = early_send_ * earlyOutBuf_[1][i] + inputR[offset + i];
        }

        late_.processreplace (
            lateInBuf_[0], lateInBuf_[1],
            lateOutBuf_[0], lateOutBuf_[1], (long) frames);

        for (uint32_t i = 0; i < frames; ++i)
        {
            outputL[offset + i] = dryLevel_   * inputL[offset + i]
                                + earlyLevel_ * earlyOutBuf_[0][i]
                                + lateLevel_  * lateOutBuf_[0][i];
            outputR[offset + i] = dryLevel_   * inputR[offset + i]
                                + earlyLevel_ * earlyOutBuf_[1][i]
                                + lateLevel_  * lateOutBuf_[1][i];
        }
    }

    // handle the send
    {
        int sendBufferIndex = getChannelIndexInProcessBlockBuffer (false, 2, 0);
        if (sendBufferIndex >= 0 && sendBufferIndex + 1 < buffer.getNumChannels())
        {
            auto sendgainmult = bitklavier::utils::dbToMagnitude (state.params.outputSend->getCurrentValue());
            buffer.copyFrom (sendBufferIndex, 0, buffer.getReadPointer(0), numSamples, sendgainmult);
            buffer.copyFrom (sendBufferIndex+1, 0, buffer.getReadPointer(1), numSamples, sendgainmult);
            std::get<0> (state.params.sendLevels) = buffer.getRMSLevel (sendBufferIndex, 0, numSamples);
            std::get<1> (state.params.sendLevels) = buffer.getRMSLevel (sendBufferIndex+1, 0, numSamples);
        }
    }

    // Output gain
    const float outGain = bitklavier::utils::dbToMagnitude (*state.params.outputGain);
    buffer.applyGain (0, 0, numSamples, outGain);
    buffer.applyGain (1, 0, numSamples, outGain);

    std::get<0> (state.params.outputLevels) = buffer.getRMSLevel (0, 0, numSamples);
    std::get<1> (state.params.outputLevels) = buffer.getRMSLevel (1, 0, numSamples);
}
