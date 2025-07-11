//
// Created by Dan Trueman on 7/10/25.
//

#include "MidiFilterProcessor.h"

MidiFilterProcessor::MidiFilterProcessor (const juce::ValueTree& v, SynthBase& parent) : PluginBase (parent, v, nullptr, midiFilterBusLayout())
//MidiFilterProcessor::MidiFilterProcessor (const juce::ValueTree& v, SynthBase& parent) : juce::AudioProcessor(BusesProperties().withInput("disabled",juce::AudioChannelSet::mono(),false))
{

}

std::unique_ptr<juce::AudioProcessor> MidiFilterProcessor::create (SynthBase& parent, const juce::ValueTree& v)
{
    return std::make_unique<MidiFilterProcessor> (v, parent);
}

void MidiFilterProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{

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