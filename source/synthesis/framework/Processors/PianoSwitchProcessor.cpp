//
// Created by Dan Trueman on 7/22/25.
//

#include "PianoSwitchProcessor.h"

PianoSwitchProcessor::PianoSwitchProcessor(
    const juce::ValueTree &v, SynthBase &parent) : PluginBase(parent, v, nullptr, pianoSwitchBusLayout()),
                                                   synth_base_(parent) {
}

std::unique_ptr<juce::AudioProcessor> PianoSwitchProcessor::create(SynthBase &parent, const juce::ValueTree &v) {
    return std::make_unique<PianoSwitchProcessor>(v, parent);
}

void PianoSwitchProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages) {
    buffer.clear();
    for (auto msg: midiMessages) {
        if (msg.getMessage().isNoteOn()) {
            DBG("PianoSwitchProcessor::processBlock received noteOn " + juce::String(msg.getMessage().getNoteNumber()));
            if (v.getProperty(IDs::selectedPianoIndex).isInt() &&
                v.getProperty(IDs::selectedPianoIndex).isInt() != -1) {
                int index = v.getProperty(IDs::selectedPianoIndex);
                synth_base_.setActivePiano(synth_base_.getValueTree().getChild(v.getProperty(IDs::selectedPianoIndex)),
                                           SwitchTriggerThread::AudioThread);
                synth_base_.callOnMainThread([=]() {
                    for (auto vt: synth_base_.getValueTree()) {
                        if (vt.hasType(IDs::PIANO)) {
                            vt.setProperty(IDs::isActive, 0, nullptr);
                        }
                    }
                    synth_base_.getValueTree().getChild(v.getProperty(IDs::selectedPianoIndex)).setProperty(
                        IDs::isActive, 1, nullptr);
                });
            }
        }

    }
}
