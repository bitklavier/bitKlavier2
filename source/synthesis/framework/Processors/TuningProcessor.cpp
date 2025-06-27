//
// Created by Joshua Warner on 6/27/24.
//

#include "TuningProcessor.h"
#include "synth_base.h"
TuningProcessor::TuningProcessor (SynthBase& parent, const juce::ValueTree& v) : PluginBase (parent, v, nullptr, tuningBusLayout())
{
    parent.getStateBank().addParam (std::make_pair<std::string, bitklavier::ParameterChangeBuffer*> (v.getProperty (IDs::uuid).toString().toStdString() + "_" + "absoluteTuning", &(state.params.tuningState.stateChanges)));
    parent.getStateBank().addParam (std::make_pair<std::string, bitklavier::ParameterChangeBuffer*> (v.getProperty (IDs::uuid).toString().toStdString() + "_" + "circularTuning", &(state.params.tuningState.stateChanges)));
}

void TuningProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const auto spec = juce::dsp::ProcessSpec { sampleRate, (uint32_t) samplesPerBlock, (uint32_t) getMainBusNumInputChannels() };

    gain.prepare (spec);
    gain.setRampDurationSeconds (0.05);
}

void TuningProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
}

template <typename Serializer>
typename Serializer::SerializedType TuningParams::serialize (const TuningParams& paramHolder)
{
    auto ser = chowdsp::ParamHolder::serialize<Serializer> (paramHolder);
    Serializer::template addChildElement<12> (ser, "circularTuning", paramHolder.tuningState.circularTuningOffset, arrayToString);
    Serializer::template addChildElement<128> (ser, "absoluteTuning", paramHolder.tuningState.absoluteTuningOffset, arrayToStringWithIndex);

    return ser;
}

template <typename Serializer>
void TuningParams::deserialize (typename Serializer::DeserializedType deserial, TuningParams& paramHolder)
{
    chowdsp::ParamHolder::deserialize<Serializer> (deserial, paramHolder);
    auto myStr = deserial->getStringAttribute ("circularTuning");
    paramHolder.tuningState.circularTuningOffset = parseFloatStringToArrayCircular<12> (myStr.toStdString());
    myStr = deserial->getStringAttribute ("absoluteTuning");
    paramHolder.tuningState.absoluteTuningOffset = parseIndexValueStringToArrayAbsolute<128> (myStr.toStdString());
    paramHolder.tuningState.fundamental = paramHolder.fundamental->getIndex();
}
