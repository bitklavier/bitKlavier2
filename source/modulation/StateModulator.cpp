//
// Created by Davis Polito on 2/17/25.
//

#include "StateModulator.h"
#include "synth_section.h"
StateModulatorProcessor::StateModulatorProcessor(const juce::ValueTree& vt, juce::UndoManager* um): ModulatorBase(vt,um)
{
    createUuidProperty(state);
}

void StateModulatorProcessor::getNextAudioBlock(juce::AudioBuffer<float>& bufferToFill,juce::MidiBuffer& midiMessages) {
    for (int i = 0; i < bufferToFill.getNumSamples(); i++)
    {
        bufferToFill.setSample(0, i,1.f);
    }
}

SynthSection *StateModulatorProcessor::createEditor()
{
    return new SynthSection("state" ,state.getProperty(IDs::type).toString() + "-" + state.getProperty(IDs::uuid).toString());
}
