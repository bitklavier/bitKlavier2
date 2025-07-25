//
// Created by Davis Polito on 6/19/24.
//

#include "KeymapProcessor.h"
#include "array_to_string.h"
#include "common.h"
#include "synth_base.h"

KeymapProcessor::KeymapProcessor (const juce::ValueTree& v, SynthBase& parent) : PluginBase (
                                                                                     parent,
                                                                                     v,
                                                                                     nullptr,
                                                                                     keymapBusLayout()),
                                                                                 _midi (std::make_unique<MidiManager> (&keyboard_state, parent.manager, v))
{
}

static juce::String getMidiMessageDescription (const juce::MidiMessage& m)
{
    if (m.isNoteOn())
        return "Note on " + juce::MidiMessage::getMidiNoteName (m.getNoteNumber(), true, true, 3);
    if (m.isNoteOff())
        return "Note off " + juce::MidiMessage::getMidiNoteName (m.getNoteNumber(), true, true, 3);
    if (m.isProgramChange())
        return "Program change " + juce::String (m.getProgramChangeNumber());
    if (m.isPitchWheel())
        return "Pitch wheel " + juce::String (m.getPitchWheelValue());
    if (m.isAftertouch())
        return "After touch " + juce::MidiMessage::getMidiNoteName (m.getNoteNumber(), true, true, 3) + ": " + juce::String (m.getAfterTouchValue());
    if (m.isChannelPressure())
        return "Channel pressure " + juce::String (m.getChannelPressureValue());
    if (m.isAllNotesOff())
        return "All notes off";
    if (m.isAllSoundOff())
        return "All sound off";
    if (m.isMetaEvent())
        return "Meta event";

    if (m.isController())
    {
        juce::String name (juce::MidiMessage::getControllerName (m.getControllerNumber()));

        if (name.isEmpty())
            name = "[" + juce::String (m.getControllerNumber()) + "]";

        return "Controller " + name + ": " + juce::String (m.getControllerValue());
    }

    return juce::String::toHexString (m.getRawData(), m.getRawDataSize());
}
static juce::String printMidi (juce::MidiMessage& message, const juce::String& source)
{
    auto time = message.getTimeStamp(); //- startTime;

    auto hours = ((int) (time / 3600.0)) % 24;
    auto minutes = ((int) (time / 60.0)) % 60;
    auto seconds = ((int) time) % 60;
    auto millis = ((int) (time * 1000.0)) % 1000;

    auto timecode = juce::String::formatted ("%02d:%02d:%02d.%03d",
        hours,
        minutes,
        seconds,
        millis);

    auto description = getMidiMessageDescription (message);

    return juce::String (timecode + "  -  " + description + " (" + source + ")"); // [7]
}

void KeymapProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    //    const auto spec = juce::dsp::ProcessSpec { sampleRate, (uint32_t) samplesPerBlock, (uint32_t) getMainBusNumInputChannels() };
    _midi->midi_collector_.reset (sampleRate);
}

void KeymapProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    //DBG (v.getParent().getParent().getProperty (IDs::name).toString() + " kmap");

    midiMessages.clear();
    int num_samples = buffer.getNumSamples();

    juce::MidiBuffer midi_messages;
    _midi->removeNextBlockOfMessages (midi_messages, num_samples);
    _midi->replaceKeyboardMessages (midi_messages, num_samples);
    for (auto message : midi_messages)
    {
        if (state.params.keyboard_state.keyStates.test (message.getMessage().getNoteNumber()))
            midiMessages.addEvent (message.getMessage(), message.samplePosition);
    }

    //    // print them out for now
    //    for (auto mi : midiMessages)
    //    {
    //        auto message = mi.getMessage();
    //
    //        mi.samplePosition;
    //        mi.data;
    //        DBG (printMidi (message, "kmap"));
    //    }

    // DBG("keymap");
}
void KeymapProcessor::processBlockBypassed (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    processBlock (buffer, midiMessages);
}
void KeymapProcessor::allNotesOff()
{
    _midi->allNotesOff();
}

template <typename Serializer>
typename Serializer::SerializedType KeymapParams::serialize (const KeymapParams& paramHolder)
{
    auto ser = chowdsp::ParamHolder::serialize<Serializer> (paramHolder);
    Serializer::template addChildElement<128> (ser, "keyOn", paramHolder.keyboard_state.keyStates, getOnKeyString);
    return ser;
}

template <typename Serializer>
void KeymapParams::deserialize (typename Serializer::DeserializedType deserial, KeymapParams& paramHolder)
{
    chowdsp::ParamHolder::deserialize<Serializer> (deserial, paramHolder);
    auto mystr = deserial->getStringAttribute ("keyOn");
    //also used in bkkeymapkeyboardcomponent TODO - make a function
    std::bitset<128> bits;
    std::istringstream iss (mystr.toStdString());
    int key;

    while (iss >> key)
    {
        if (key >= 0 && key < 128)
        {
            bits.set (key);
        }
    }
    paramHolder.keyboard_state.keyStates = bits;
}
