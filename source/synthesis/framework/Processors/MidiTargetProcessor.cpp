//
// Created by Dan Trueman on 8/2/25.
//

#include "MidiTargetProcessor.h"

MidiTargetProcessor::MidiTargetProcessor (
    const juce::ValueTree& v, SynthBase& parent) :
                         PluginBase (parent, v, nullptr, midiTargetBusLayout())
{
    //state.params.activeTargets.fill(false);

    /*
     * these will be set when connected
     */
    state.params.activeTargets[BlendronicTargetNormal] = state.params.blendronicTargetBeatSync.get();
    state.params.activeTargets[BlendronicTargetPatternSync] = state.params.blendronicTargetPatternSync.get();
    state.params.activeTargets[BlendronicTargetBeatSync] = state.params.blendronicTargetBeatSync.get();
    state.params.activeTargets[BlendronicTargetClear] = state.params.blendronicTargetClear.get();
    state.params.activeTargets[BlendronicTargetPausePlay] = state.params.blendronicTargetPausePlay.get();

    state.params.triggerModes[BlendronicTargetNormal] = state.params.blendronicTargetBeatSync_noteMode.get();
    state.params.triggerModes[BlendronicTargetPatternSync] = state.params.blendronicTargetPatternSync_noteMode.get();
    state.params.triggerModes[BlendronicTargetBeatSync] = state.params.blendronicTargetBeatSync_noteMode.get();
    state.params.triggerModes[BlendronicTargetClear] = state.params.blendronicTargetClear_noteMode.get();
    state.params.triggerModes[BlendronicTargetPausePlay] = state.params.blendronicTargetPausePlay_noteMode.get();
}

std::unique_ptr<juce::AudioProcessor> MidiTargetProcessor::create (SynthBase& parent, const juce::ValueTree& v)
{
    return std::make_unique<MidiTargetProcessor> (v, parent);
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

        for (int i = BlendronicTargetNormal; i < BlendronicTargetNil; ++i)
        {
            // Cast the integer back to the enum type
            BlendronicTargetType currentTarget = static_cast<BlendronicTargetType> (i);
            if (state.params.activeTargets[currentTarget]->get())
            {
                if(state.params.triggerModes[currentTarget]->get() == _NoteOn)
                {
                    //only allow noteOffs through
                    if (message.isNoteOn())
                    {
                        juce::MidiMessage newmsg(message);
                        newmsg.setChannel (i);
                        midiMessages.addEvent (newmsg, mi.samplePosition);
                    }
                }
                else if(state.params.triggerModes[currentTarget]->get() == _NoteOff)
                {
                    //only allow noteOffs through
                    if (message.isNoteOff())
                    {
                        juce::MidiMessage newmsg(message);
                        newmsg.setChannel (i);
                        midiMessages.addEvent (newmsg, mi.samplePosition);
                    }
                }
                else
                {
                    //put it through regardless
                    juce::MidiMessage newmsg(message);
                    newmsg.setChannel (i);
                    midiMessages.addEvent (newmsg, mi.samplePosition);
                }
            }

            /**
             * todo: need to do the noteOn/noteOff/Both modes!
             */
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