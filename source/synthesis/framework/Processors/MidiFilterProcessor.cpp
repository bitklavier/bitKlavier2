//
// Created by Dan Trueman on 7/10/25.
//

#include "MidiFilterProcessor.h"

MidiFilterProcessor::MidiFilterProcessor (
    const juce::ValueTree& v, SynthBase& parent) :
             PluginBase (parent, v, nullptr, midiFilterBusLayout())
{

}

std::unique_ptr<juce::AudioProcessor> MidiFilterProcessor::create (SynthBase& parent, const juce::ValueTree& v)
{
    return std::make_unique<MidiFilterProcessor> (v, parent);
}

void MidiFilterProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    /**
     *
     * all the preparation parameter targeting and noteOn/Off/Both happens here now
     *
     * other processes that could/should happen here:
     * Invert NoteOn/Off
     * Trigger All Notes Off
     * Ignore Sustain Pedal
     * Use as Sustain Pedal
     * Toggle Keys
     * Sostenuto Mode
     * Ignore NoteOff
     *
     */

    juce::MidiBuffer saveMidi (midiMessages);
    midiMessages.clear();

    // this could surely be optimized, though i'm not sure how important that is ./dlt
    // i'm also not 100% this properly retain the samplePosition, but again, it's not clear how important
    // that is when noteOn and noteOff are inverted... might be important for other MIDI processing, however
    bool invertNoteOnNoteOff = true; // for now
    if (invertNoteOnNoteOff)
    {
        for (auto mi : saveMidi)
        {
            auto message = mi.getMessage();
            if (message.isNoteOn() || message.isNoteOff())
                midiMessages.addEvent (swapNoteOnNoteOff (message), mi.samplePosition);
            else
                midiMessages.addEvent (message, mi.samplePosition);
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

/**
 * note sure we'll need these
 *
 * @tparam Serializer
 * @param paramHolder
 * @return
 */
template <typename Serializer>
typename Serializer::SerializedType MidiFilterParams::serialize (const MidiFilterParams& paramHolder)
{
    /*
     * first, call the default serializer, which gets all the simple params
     */
    auto ser = chowdsp::ParamHolder::serialize<Serializer> (paramHolder);

    /*
     * then serialize the more complex params
     */

    return ser;
}

template <typename Serializer>
void MidiFilterParams::deserialize (typename Serializer::DeserializedType deserial, MidiFilterParams& paramHolder)
{

}