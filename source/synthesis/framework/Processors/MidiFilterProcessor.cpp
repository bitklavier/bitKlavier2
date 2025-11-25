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
    juce::MidiBuffer saveMidi (midiMessages);
    midiMessages.clear();

    for (auto mi : saveMidi)
    {
        auto message = mi.getMessage();

        if(!*state.params.ignoreSustainPedal &&
            (message.isSustainPedalOn() || message.isSustainPedalOff()) &&
            !*state.params.sostenutoMode)
        {
            midiMessages.addEvent (message, mi.samplePosition);
        }

        if(*state.params.sostenutoMode)
        {
            if (message.isSustainPedalOn())
            {
                DBG("sustain pedal to sostenuto pedalOn");
                // Sostenuto On on MIDI Channel 1
                const int channel = 1;
                const int controllerNumber = 66; // Sostenuto CC
                const int controllerValue = 127; // On

                auto sostenutoOnMsg = juce::MidiMessage::controllerEvent(
                    channel,
                    controllerNumber,
                    controllerValue
                );
                midiMessages.addEvent (sostenutoOnMsg, mi.samplePosition);
            }
            else if (message.isSustainPedalOff())
            {
                DBG("sustain pedal to sostenuto pedalOff");
                // Sostenuto Off on MIDI Channel 1
                const int channel = 1;
                const int controllerNumber = 66; // Sostenuto CC
                const int controllerValue = 0;   // Off

                auto sostenutoOffMsg = juce::MidiMessage::controllerEvent(
                    channel,
                    controllerNumber,
                    controllerValue
                );
                midiMessages.addEvent (sostenutoOffMsg, mi.samplePosition);
            }
        }

        if(*state.params.notesAreSostenutoPedal)
        {
            if (message.isNoteOn())
            {
                DBG("noteOn to sostenuto pedalOn");
                // Sostenuto On on MIDI Channel 1
                const int channel = 1;
                const int controllerNumber = 66; // Sostenuto CC
                const int controllerValue = 127; // On

                auto sostenutoOnMsg = juce::MidiMessage::controllerEvent(
                    channel,
                    controllerNumber,
                    controllerValue
                );
                midiMessages.addEvent (sostenutoOnMsg, mi.samplePosition);
            }
            else if (message.isNoteOff())
            {
                DBG("noteOff to sostenuto pedalOff");
                // Sostenuto Off on MIDI Channel 1
                const int channel = 1;
                const int controllerNumber = 66; // Sostenuto CC
                const int controllerValue = 0;   // Off

                auto sostenutoOffMsg = juce::MidiMessage::controllerEvent(
                    channel,
                    controllerNumber,
                    controllerValue
                );
                midiMessages.addEvent (sostenutoOffMsg, mi.samplePosition);
            }
        }

        else if(*state.params.notesAreSustainPedal)
        {
            if (message.isNoteOn())
            {
                DBG("noteOn to sustain pedalOn");
                // Sustain Pedal On (Channel 1)
                const int channel = 1;
                const int sustainCC = 64;
                const int valueOn = 127;

                auto sustainOnMsg = juce::MidiMessage::controllerEvent(
                    channel,
                    sustainCC,
                    valueOn
                );
                midiMessages.addEvent (sustainOnMsg, mi.samplePosition);
            }
            else if (message.isNoteOff())
            {
                DBG("noteOff to sustain pedalOff");
                // Sustain Pedal Off (Channel 1)
                const int channel = 1;
                const int sustainCC = 64;
                const int valueOff = 0;

                auto sustainOffMsg = juce::MidiMessage::controllerEvent(
                    channel,
                    sustainCC,
                    valueOff
                );
                midiMessages.addEvent (sustainOffMsg, mi.samplePosition);
            }
        }

        else if(*state.params.toggleNoteMessages)
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
                if(*state.params.allNotesOff)
                {
                    for (int i = 1; i < 128; i++)
                    {
                        auto offmsg = juce::MidiMessage::noteOff (1, i, 0.5f);
                        midiMessages.addEvent (offmsg, mi.samplePosition);
                    }
                }
                else
                {
                    if (*state.params.invertNoteOnOff)
                        midiMessages.addEvent (swapNoteOnNoteOff (message), mi.samplePosition);
                    else
                        midiMessages.addEvent (message, mi.samplePosition);
                }
            }

            if (message.isNoteOff() && !*state.params.ignoreNoteOff)
            {
                if(*state.params.allNotesOff)
                {
                    for (int i = 1; i < 128; i++)
                    {
                        auto offmsg = juce::MidiMessage::noteOff (1, i, 0.5f);
                        midiMessages.addEvent (offmsg, mi.samplePosition);
                    }
                }
                else
                {
                    if (*state.params.invertNoteOnOff)
                        midiMessages.addEvent (swapNoteOnNoteOff (message), mi.samplePosition);
                    else
                        midiMessages.addEvent (message, mi.samplePosition);
                }
            }
        }
    }
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