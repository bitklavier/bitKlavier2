//
// Created by Davis Polito on 1/30/25.
//

#include "ResetProcessor.h"
#include "ModulatorBase.h"
#include "ModulationConnection.h"

/**
 * todo: this needs to be implemented
 * @param buffer
 * @param midiMessages
 */
void bitklavier::ResetProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    buffer.clear();

    float outval = 0;
    for (auto msg : midiMessages)
    {
        if (msg.getMessage().isNoteOn())
        {
            //DBG("reset called");
            outval = 1;
        }
    }

    auto reset_out = getChannelIndexInProcessBlockBuffer(false,3,0);
    if (reset_out >= 0 && reset_out < buffer.getNumChannels() && buffer.getNumSamples() > 0)
        buffer.setSample(reset_out,0,outval);
    midiMessages.clear();
}
