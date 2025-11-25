//
// Created by Dan Trueman on 7/10/25.
//

#include "MidiFilterProcessor.h"

MidiFilterProcessor::MidiFilterProcessor (
     SynthBase& parent,const juce::ValueTree& v) :
             PluginBase (parent, v, nullptr, midiFilterBusLayout())
{
    noteOnState.reset();
}

void MidiFilterProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    /**
     *
     * all the preparation parameter targeting and noteOn/Off/Both happens here now
     *
     * other processes that could/should happen here:
     * Trigger All Notes Off
     * Ignore Sustain Pedal
     * Use as Sustain Pedal
     * Sostenuto Mode
     *
     */

    juce::MidiBuffer saveMidi (midiMessages);
    midiMessages.clear();

    for (auto mi : saveMidi)
    {
        auto message = mi.getMessage();

        if(*state.params.toggleNoteMessages)
        {
            if (message.isNoteOn())
            {
                if (!noteOnState.test(message.getNoteNumber()))
                {
                    midiMessages.addEvent (message, mi.samplePosition);
                    noteOnState.set(message.getNoteNumber(), true);
                }
                else
                {
                    midiMessages.addEvent (swapNoteOnNoteOff (message), mi.samplePosition);
                    noteOnState.set(message.getNoteNumber(), false);
                }
            }
        }

        else
        {
            if (message.isNoteOn() && !*state.params.ignoreNoteOn)
            {
                if (*state.params.invertNoteOnOff)
                    midiMessages.addEvent (swapNoteOnNoteOff (message), mi.samplePosition);
                else
                    midiMessages.addEvent (message, mi.samplePosition);
            }

            if (message.isNoteOff() && !*state.params.ignoreNoteOff)
            {
                if (*state.params.invertNoteOnOff)
                    midiMessages.addEvent (swapNoteOnNoteOff (message), mi.samplePosition);
                else
                    midiMessages.addEvent (message, mi.samplePosition);
            }
        }
    }

//    if (*state.params.invertNoteOnOff)
//    {
//        for (auto mi : saveMidi)
//        {
//            auto message = mi.getMessage();
//            if (message.isNoteOn() || message.isNoteOff())
//                midiMessages.addEvent (swapNoteOnNoteOff (message), mi.samplePosition);
//            else
//                midiMessages.addEvent (message, mi.samplePosition);
//        }
//    }
}

// turns noteOn messages into noteOff messages, and vice versa
// there is surely a more efficient way to do this...
juce::MidiMessage MidiFilterProcessor::swapNoteOnNoteOff (juce::MidiMessage inmsg)
{
    if (inmsg.isNoteOff())
    {
        auto newmsg = juce::MidiMessage::noteOn (inmsg.getChannel(), inmsg.getNoteNumber(), inmsg.getVelocity());
        newmsg.addToTimeStamp (inmsg.getTimeStamp());
        return newmsg;
    }
    else if (inmsg.isNoteOn())
    {
        auto newmsg = juce::MidiMessage::noteOff (inmsg.getChannel(), inmsg.getNoteNumber(), inmsg.getVelocity());
        newmsg.addToTimeStamp (inmsg.getTimeStamp());
        return newmsg;
    }

    return inmsg;
}

/*
 * for saving/loading
 */
template <typename Serializer>
typename Serializer::SerializedType MidiFilterParams::serialize (const MidiFilterParams& paramHolder)
{
    auto ser = chowdsp::ParamHolder::serialize<Serializer> (paramHolder);
    return ser;
}

template <typename Serializer>
void MidiFilterParams::deserialize (typename Serializer::DeserializedType deserial, MidiFilterParams& paramHolder)
{
    chowdsp::ParamHolder::deserialize<Serializer> (deserial, paramHolder);
}