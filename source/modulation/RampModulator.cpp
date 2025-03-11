//
// Created by Davis Polito on 2/7/25.
//

#include "RampModulator.h"

RampModulatorProcessor::RampModulatorProcessor(juce::ValueTree& vt) : ModulatorStateBase<bitklavier::PreparationStateImpl<RampParams>>(vt)

{
//    vt.setProperty(IDs::uuid, state.params.processor.processorUniqueID, nullptr);
//    name = vt.getProperty(IDs::type).toString() + vt.getProperty(IDs::uuid).toString();
    createUuidProperty(vt);
}

void RampModulatorProcessor::getNextAudioBlock(juce::AudioBuffer<float>& bufferToFill,juce::MidiBuffer& midiMessages) {
    for (int i = 0; i < bufferToFill.getNumSamples(); i++)
    {
        bufferToFill.setSample(0, i,1.f);
    }

}