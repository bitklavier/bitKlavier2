// Copyright (C) 2022-2025 Dan Trueman
// SPDX-License-Identifier: GPL-3.0-or-later

//
// Created by Davis Polito on 2/7/25.
// --ramp code partially derived from Envelope in STK
//

#include "RampModulator.h"
#include "ParameterView/ParametersView.h"

RampModulatorProcessor::RampModulatorProcessor(const juce::ValueTree& vt, juce::UndoManager* um) : ModulatorStateBase<bitklavier::PreparationStateImpl<RampParams>>(vt,um)
{
    createUuidProperty(state);
}

void RampModulatorProcessor::prepareToPlay (double sampleRate_, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec { sampleRate_, static_cast<juce::uint32>(samplesPerBlock), 1};
    //sampleRate = spec.sampleRate;
    sampleRate = sampleRate_;
    DBG("RampModulator sample rate = " << sampleRate);
    setTime(*_state.params.time);
    setTarget(0.f);
}

void RampModulatorProcessor::setTarget( float target )
{
    target_ = target;
}

void RampModulatorProcessor::setTime( float timeToDest )
{
    if ( timeToDest <= 0.0f )
    {
        // Instant: jump directly to target on the first sample
        value_ = target_;
        state_ = 0;
        return;
    }

    if ( timeToDest < 1.0f )
        timeToDest = 1.0f; // avoid division by near-zero

    float totalSamples = timeToDest * sampleRate * 0.001f;
    rate_ = fabsf(target_ - value_) / totalSamples; // kept for compatibility
    posRate_ = 1.0f / totalSamples;
    pos_ = 0.0f;
    startValue_ = value_;

    float k = *_state.params.curve;
    if (fabsf(k - 1.0f) > 1e-5f)
    {
        logK_ = logf(k);
        invKMinus1_ = 1.0f / (k - 1.0f);
    }
}

float RampModulatorProcessor::getNextSample()
{
    if ( state_ ) {
        pos_ += posRate_;
        if ( pos_ >= 1.0f ) {
            pos_ = 1.0f;
            value_ = target_;
            state_ = 0;
        } else if ( fabsf(*_state.params.curve - 1.0f) < 1e-5f ) {
            // linear: cheap lerp, no expf
            value_ = startValue_ + (target_ - startValue_) * pos_;
        } else {
            // dt_asymwarp inline: (k^pos - 1) / (k - 1)
            float warped = (expf(pos_ * logK_) - 1.0f) * invKMinus1_;
            value_ = startValue_ + (target_ - startValue_) * warped;
        }
    }

    if (value_ > 1.0f) value_ = 1.0f;
    return value_;
}

void RampModulatorProcessor::triggerModulation()
{
    DBG("RampModulatorProcessor::triggerModulation(), time = " << *_state.params.time);
    // target_ = 1.;
    // state_ = 1;
    // setTime(*_state.params.time);
    // ModulatorBase::triggerModulation();
    // Instead of forcing value_ to 0 implicitly, restart from current value_

    retriggerFrom (value_);
}

void RampModulatorProcessor::triggerReset()
{
    DBG("RampModulatorProcessor::triggerReset()");
    value_ = 0.f;
    state_ = 0;
    ModulatorBase::triggerReset();
}

void RampModulatorProcessor::getNextAudioBlock(juce::AudioBuffer<float>& buffer,juce::MidiBuffer& midiMessages) {
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        float rampVal = getNextSample();

        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            float* channelData = buffer.getWritePointer(channel);
            channelData[sample] = (rampVal);
        }
    }
}

SynthSection *RampModulatorProcessor::createEditor() {
    return new bitklavier::ParametersView(_state, _state.params, state.getProperty(IDs::type).toString() + "-" + state.getProperty(IDs::uuid).toString());
}

void RampModulatorProcessor::retriggerFrom(float current)
{
    DBG("RampModulatorProcessor::retriggerFrom = " << current);
    // Keep continuity: start from the current output level
    value_ = juce::jlimit (0.0f, 1.0f, current);

    // Retrigger means "ramp toward 1 again" (match triggerModulation semantics)
    target_ = 1.0f;
    state_ = 1;

    // setTime resets pos_=0 and startValue_=value_, then precomputes curve constants
    setTime (*_state.params.time);

    // this function doesn't currently do anything
    ModulatorBase::triggerModulation(); // if you use this for notifying listeners etc.
}
