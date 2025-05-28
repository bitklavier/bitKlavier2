//
// Created by Joshua Warner on 6/27/24.
//

#include "TuningProcessor.h"
#include "synth_base.h"
TuningProcessor::TuningProcessor(SynthBase* parent,const juce::ValueTree& v) : PluginBase(parent,v,nullptr,  tuningBusLayout())
{
    parent->getStateBank().addParam(std::make_pair<std::string,bitklavier::ParameterChangeBuffer*>(v.getProperty(IDs::uuid).toString().toStdString() + "_" + "absoluteTuning", &(state.params.keyboardState.stateChanges)));
    parent->getStateBank().addParam(std::make_pair<std::string,bitklavier::ParameterChangeBuffer*>(v.getProperty(IDs::uuid).toString().toStdString() + "_" + "circularTuning", &(state.params.keyboardState.stateChanges)));
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

