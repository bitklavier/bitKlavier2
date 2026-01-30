//
// Created by Dan Trueman on 7/22/25.
//

#include "PianoSwitchProcessor.h"

PianoSwitchProcessor::PianoSwitchProcessor (SynthBase& parent,
    const juce::ValueTree& v, juce::UndoManager* um
    ) : PluginBase (parent, v, um, pianoSwitchBusLayout()),
                         synth_base_ (parent)
{
}

void PianoSwitchProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    buffer.clear();
    //DBG (v.getParent().getParent().getProperty (IDs::name).toString() + "switch");

    for (auto msg : midiMessages)
    {
        if (msg.getMessage().isNoteOn()  && std::abs(synth_base_.sample_index_of_switch - msg.samplePosition) >= 10)
        {
            //DBG ("PianoSwitchProcessor::processBlock received noteOn " + juce::String (msg.getMessage().getNoteNumber()));
            if (roundToInt (v.getProperty (IDs::selectedPianoIndex)) != -1)
            {
                const int selectedIndex = (int) v.getProperty (IDs::selectedPianoIndex);

                // Map selectedIndex to the Nth child with type IDs::PIANO
                juce::ValueTree selectedPianoVT;
                int pianoCount = 0;
                for (auto child : synth_base_.getValueTree())
                {
                    if (child.hasType (IDs::PIANO))
                    {
                        if (pianoCount == selectedIndex)
                        {
                            selectedPianoVT = child;
                            break;
                        }
                        ++pianoCount;
                    }
                }

                if (! selectedPianoVT.isValid() || ! selectedPianoVT.hasType (IDs::PIANO))
                {
                    // Invalid selection, don't attempt to switch
                    break;
                }
                synth_base_.sample_index_of_switch = msg.samplePosition;
                synth_base_.setActivePiano (selectedPianoVT, SwitchTriggerThread::AudioThread);
                synth_base_.callOnMainThread ([=]() {
                    for (auto vt : synth_base_.getValueTree())
                    {
                        if (vt.hasType (IDs::PIANO))
                        {
                            vt.setProperty (IDs::isActive, 0, nullptr);
                        }
                    }
                    // Mark only the selected piano as active
                    juce::ValueTree pianoVT = selectedPianoVT;
                    if (pianoVT.isValid() && pianoVT.hasType (IDs::PIANO))
                        pianoVT.setProperty (IDs::isActive, 1, nullptr);
                });
            }
            break;
        }
    }

}
