//
// Created by Dan Trueman on 8/2/25.
//

#include "MidiTargetProcessor.h"

MidiTargetProcessor::MidiTargetProcessor (
    const juce::ValueTree& v, SynthBase& parent) :
                         PluginBase (parent, v, nullptr, midiTargetBusLayout())
{

}

std::unique_ptr<juce::AudioProcessor> MidiTargetProcessor::create (SynthBase& parent, const juce::ValueTree& v)
{
    return std::make_unique<MidiTargetProcessor> (v, parent);
}

void MidiTargetProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{

    juce::MidiBuffer saveMidi (midiMessages);
    midiMessages.clear();

//    bool invertNoteOnNoteOff = true; // for now
//    if (invertNoteOnNoteOff)
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
juce::MidiMessage MidiTargetProcessor::swapNoteOnNoteOff (juce::MidiMessage inmsg)
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
 * not sure we'll need these
 *
 * @tparam Serializer
 * @param paramHolder
 * @return
 */
template <typename Serializer>
typename Serializer::SerializedType MidiTargetParams::serialize (const MidiTargetParams& paramHolder)
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
void MidiTargetParams::deserialize (typename Serializer::DeserializedType deserial, MidiTargetParams& paramHolder)
{

}