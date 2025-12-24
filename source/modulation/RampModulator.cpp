//
// Created by Davis Polito on 2/7/25.
// --ramp code partially derived from Envelope in STK
//

#include "RampModulator.h"
#include "ParameterView/ParametersView.h"

RampModulatorProcessor::RampModulatorProcessor(juce::ValueTree& vt) : ModulatorStateBase<bitklavier::PreparationStateImpl<RampParams>>(vt)
{
    createUuidProperty(vt);
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
    if ( timeToDest <= 1.0 ) {
        timeToDest = 1.;
    }

    rate_ = fabs(target_ - value_) / ( timeToDest * sampleRate * 0.001 );
    //DBG("rate = " << rate_ << " timeToDest = " << timeToDest << " value_ = " << value_ << " target_ = " << target_ << " sampleRate = " << sampleRate << "");
}

float RampModulatorProcessor::getNextSample()
{
    if ( state_ ) {
        if ( target_ > value_ ) {
            value_ += rate_;
            if ( value_ >= target_ ) {
                value_ = target_;
                state_ = 0;
            }
        }
        else {
            value_ -= rate_;
            if ( value_ <= target_ ) {
                value_ = target_;
                state_ = 0;
            }
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
    value_ = 0;
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
    // Keep continuity: start from the current output level
    value_ = juce::jlimit (0.0f, 1.0f, current);

    // Retrigger means "ramp toward 1 again" (match triggerModulation semantics)
    target_ = 1.0f;

    state_ = 1;

    // Recompute rate based on (target_ - value_) and current time
    setTime (*_state.params.time);

    ModulatorBase::triggerModulation(); // if you use this for notifying listeners etc.
}
