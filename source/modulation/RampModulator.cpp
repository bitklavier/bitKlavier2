//
// Created by Davis Polito on 2/7/25.
//

#include "RampModulator.h"
#include "ParameterView/ParametersView.h"
RampModulatorProcessor::RampModulatorProcessor(juce::ValueTree& vt) : ModulatorStateBase<bitklavier::PreparationStateImpl<RampParams>>(vt)
{
//    vt.setProperty(IDs::uuid, state.params.processor.processorUniqueID, nullptr);
//    name = vt.getProperty(IDs::type).toString() + vt.getProperty(IDs::uuid).toString();
    createUuidProperty(vt);
    target_ = 0.0;
    value_ = 0.0;
    rate_ = 0.001;
    state_ = 0;
}

void RampModulatorProcessor::prepareToPlay (int samplesPerBlock, double sampleRate )
{
    juce::dsp::ProcessSpec spec { sampleRate, static_cast<juce::uint32>(samplesPerBlock), 1};
    sampleRate = spec.sampleRate;
    setTime(*_state.params.time);
    setTarget(0.f);
}

void RampModulatorProcessor::setTarget( float target )
{
    target_ = target;
    //if ( value_ != target_ ) state_ = 1;
}

void RampModulatorProcessor::setTime( float time )
{
    if ( time <= 0.0 ) {
        time = 0.;
    }

    rate_ = fabs(target_ - value_) / ( time * sampleRate * 0.001 );
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

    return value_;
}

void RampModulatorProcessor::triggerModulation()
{
    DBG("RampModulatorProcessor::triggerModulation()");
    setTime(*_state.params.time);
    trigger = true;
    target_ = 1.;
    state_ = 1;
}

void RampModulatorProcessor::triggerReset()
{
    trigger = false;
    value_ = 0;
    state_ = 0;
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
