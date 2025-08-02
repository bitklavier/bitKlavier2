//
// Created by Dan Trueman on 8/2/25.
//

#include "MidiTargetProcessor.h"

MidiTargetProcessor::MidiTargetProcessor (
    const juce::ValueTree& v, SynthBase& parent) :
                         PluginBase (parent, v, nullptr, midiTargetBusLayout())
{
    state.params.activeTargets.fill(false);

    /*
     * these will be set by user
     */
    state.params.activeTargets[BlendronicTargetBeatSync] = true; // for testing
}

std::unique_ptr<juce::AudioProcessor> MidiTargetProcessor::create (SynthBase& parent, const juce::ValueTree& v)
{
    return std::make_unique<MidiTargetProcessor> (v, parent);
}

void MidiTargetProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{

    juce::MidiBuffer saveMidi (midiMessages);

    //clear the midi buffer, because we're only going to pass the ones we want, by channel
    midiMessages.clear();

    for (auto mi : saveMidi)
    {
        auto message = mi.getMessage();

        for (int i = BlendronicTargetNormal; i < BlendronicTargetNil; ++i)
        {
            // Cast the integer back to the enum type
            BlendronicTargetType currentTarget = static_cast<BlendronicTargetType> (i);
            if (state.params.activeTargets[currentTarget])
            {
                juce::MidiMessage newmsg(message);
                newmsg.setChannel (i);
                midiMessages.addEvent (newmsg, mi.samplePosition);
            }
        }
    }
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