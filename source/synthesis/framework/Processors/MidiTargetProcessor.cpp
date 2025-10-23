//
// Created by Dan Trueman on 8/2/25.
//

#include "MidiTargetProcessor.h"

MidiTargetProcessor::MidiTargetProcessor ( SynthBase& parent,
    const juce::ValueTree& v) : PluginBase (parent, v, nullptr, midiTargetBusLayout())
{

}

void MidiTargetProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    // keep this copy around the iterate through
    juce::MidiBuffer saveMidi (midiMessages);

    // clear the midi buffer, because we're only going to pass the ones we want, by channel
    midiMessages.clear();

    for (auto mi : saveMidi)
    {
        auto message = mi.getMessage();

        /**
         * todo: should be able to set the limits on this loop based on
         *       the connected prep and otherwise leave this.
         */
        int startParam = BlendronicTargetFirst + 1;
        int lastParam = BlendronicTargetNil;

        // for Synchronic...
        startParam = SynchronicTargetFirst + 1;
        lastParam = SynchronicTargetNil;

        // for Resonance...
        startParam = ResonanceTargetFirst + 1;
        lastParam = ResonanceTargetNil;

        for (int i = startParam; i < lastParam; ++i)
        {
            // Cast the integer back to the enum type
            PreparationParameterTargetType currentTarget = static_cast<PreparationParameterTargetType> (i);
            if (state.params.targetMapper[currentTarget]->get())
            {
                int newchan = i - startParam + 1;
                if(state.params.noteModeMapper[currentTarget]->get() == _NoteOn)
                {
                    //only allow noteOffs through
                    if (message.isNoteOn())
                    {
                        juce::MidiMessage newmsg(message);
                        newmsg.setChannel (newchan);
                        midiMessages.addEvent (newmsg, mi.samplePosition);
                    }
                }
                else if(state.params.noteModeMapper[currentTarget]->get() == _NoteOff)
                {
                    //only allow noteOffs through
                    if (message.isNoteOff())
                    {
                        juce::MidiMessage newmsg(message);
                        newmsg.setChannel (newchan);
                        midiMessages.addEvent (newmsg, mi.samplePosition);
                    }
                }
                else
                {
                    //put it through for both noteOns and noteOffs
                    DBG("passing through bo0th ons and offs");
                    juce::MidiMessage newmsg(message);
                    newmsg.setChannel (newchan);
                    midiMessages.addEvent (newmsg, mi.samplePosition);
                }
            }
        }
    }
}


template <typename Serializer>
typename Serializer::SerializedType MidiTargetParams::serialize (const MidiTargetParams& paramHolder)
{
    auto ser = chowdsp::ParamHolder::serialize<Serializer> (paramHolder);
    return ser;
}

template <typename Serializer>
void MidiTargetParams::deserialize (typename Serializer::DeserializedType deserial, MidiTargetParams& paramHolder)
{
    chowdsp::ParamHolder::deserialize<Serializer> (deserial, paramHolder);
}
