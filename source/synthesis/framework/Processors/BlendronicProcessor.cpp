//
// Created by Joshua Warner on 6/27/24.
//

#include "BlendronicProcessor.h"

BlendronicProcessor::BlendronicProcessor() = default;

void BlendronicProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const auto spec = juce::dsp::ProcessSpec { sampleRate, (uint32_t) samplesPerBlock, (uint32_t) getMainBusNumInputChannels() };

    gain.prepare (spec);
    gain.setRampDurationSeconds (0.05);
}

template <typename Serializer>
typename Serializer::SerializedType BlendronicParams::serialize (const BlendronicParams& paramHolder)
{
    /*
     * first, call the default serializer, which gets all the simple params
     */
    auto ser = chowdsp::ParamHolder::serialize<Serializer> (paramHolder);

    /**
     * todo: make these using blendronicState.beatLengthsActual, rather than MAXMULTISLIDERLENGTH, so we don't write so much to the xml
     */
    Serializer::template addChildElement<MAXMULTISLIDERLENGTH> (ser, "blendronic_beatLengths", paramHolder.beatLengths.sliderVals, arrayToString);

    return ser;
}

template <typename Serializer>
void BlendronicParams::deserialize (typename Serializer::DeserializedType deserial, BlendronicParams& paramHolder)
{
    chowdsp::ParamHolder::deserialize<Serializer> (deserial, paramHolder);

    auto myStr = deserial->getStringAttribute ("blendronic_beatLengths");
    paramHolder.beatLengths.sliderVals = parseFloatStringToArrayCircular<MAXMULTISLIDERLENGTH> (myStr.toStdString());
}


void BlendronicProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{

}


