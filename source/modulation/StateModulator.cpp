//
// Created by Davis Polito on 2/17/25.
//

#include "StateModulator.h"

StateModulatorProcessor::StateModulatorProcessor(juce::ValueTree& vt): ModulatorBase(vt)

{

}

void StateModulatorProcessor::getNextAudioBlock(juce::AudioBuffer<float>& bufferToFill,juce::MidiBuffer& midiMessages) {
    for (int i = 0; i < bufferToFill.getNumSamples(); i++)
    {
        bufferToFill.setSample(0, i,1.f);
    }

}