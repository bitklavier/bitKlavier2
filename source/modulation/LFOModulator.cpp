//
// Created by Davis Polito on 2/7/25.
//

#include "LFOModulator.h"

LFOModulatorProcessor::LFOModulatorProcessor(juce::ValueTree& vt) : ModulatorStateBase<bitklavier::PreparationStateImpl<LFOParams>>(vt)

{
//    vt.setProperty(IDs::uuid, state.params.processor.processorUniqueID, nullptr);
//    name = vt.getProperty(IDs::type).toString() + vt.getProperty(IDs::uuid).toString();
    createUuidProperty(vt);
}

void LFOModulatorProcessor::getNextAudioBlock(juce::AudioBuffer<float>& bufferToFill,juce::MidiBuffer& midiMessages) {
    for (int i = 0; i < bufferToFill.getNumSamples(); i++)
    {
        bufferToFill.setSample(0, i,1.f);
    }

}