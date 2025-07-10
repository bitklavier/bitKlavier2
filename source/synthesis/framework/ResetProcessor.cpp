//
// Created by Davis Polito on 1/30/25.
//

#include "ResetProcessor.h"
#include "ModulatorBase.h"
#include "ModulationConnection.h"
void bitklavier::ResetProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    buffer.clear();


    for (auto msg : midiMessages)
    {
        if (msg.getMessage().isNoteOn())
        {


        }

    }
    auto reset_out = getChannelIndexInProcessBlockBuffer(false,2,0);
    buffer.setSample(reset_out,0,1);

}