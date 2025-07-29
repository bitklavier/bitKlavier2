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

void LFOModulatorProcessor::getNextAudioBlock(juce::AudioBuffer<float>& buffer,juce::MidiBuffer& midiMessages) {
    setFrequency(_state.params.freq->getCurrentValue());
    // DBG(buffer.getNumSamples());
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        float lfoValue = getNextSample(); // Ranges -depth to +depth

        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            float* channelData = buffer.getWritePointer(channel);
            channelData[sample] = (1.0f + 0.5f*lfoValue); // Example: modulate gain
        }
    }



}