//
// Created by Dan Trueman on 7/22/25.
//

#include "PianoSwitchProcessor.h"

PianoSwitchProcessor::PianoSwitchProcessor (
    const juce::ValueTree& v, SynthBase& parent) :
         PluginBase (parent, v, nullptr, pianoSwitchBusLayout())
{

}

std::unique_ptr<juce::AudioProcessor> PianoSwitchProcessor::create (SynthBase& parent, const juce::ValueTree& v)
{
    return std::make_unique<PianoSwitchProcessor> (v, parent);
}

void PianoSwitchProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
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
