//
// Created by Dan Trueman on 7/22/25.
//

#include "PianoSwitchProcessor.h"
#include "ModulatorBase.h"

void bitklavier::PianoSwitchProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    buffer.clear();
    for (auto msg : midiMessages)
    {
        if (msg.getMessage().isNoteOn())
        {
            DBG("PianoSwitchProcessor::processBlock received noteOn " + juce::String(msg.getMessage().getNoteNumber()));
        }
    }
}
