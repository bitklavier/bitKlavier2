//
// Created by Joshua Warner on 6/27/24.
//

#include "TuningProcessor.h"

TuningProcessor::TuningProcessor(SynthBase* parent,const juce::ValueTree& v) : PluginBase(parent,v,nullptr,  tuningBusLayout())
{

}

void TuningProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const auto spec = juce::dsp::ProcessSpec { sampleRate, (uint32_t) samplesPerBlock, (uint32_t) getMainBusNumInputChannels() };

    gain.prepare (spec);
    gain.setRampDurationSeconds (0.05);
}


void TuningProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{

}

