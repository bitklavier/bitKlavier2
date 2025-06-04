//
// Created by Joshua Warner on 6/27/24.
//

#include "TuningProcessor.h"
#include "synth_base.h"
TuningProcessor::TuningProcessor(SynthBase* parent,const juce::ValueTree& v) : PluginBase(parent,v,nullptr,  tuningBusLayout())
{
    parent->getStateBank().addParam(std::make_pair<std::string,bitklavier::ParameterChangeBuffer*>(v.getProperty(IDs::uuid).toString().toStdString() + "_" + "absoluteTuning", &(state.params.keyboardState.stateChanges)));
    parent->getStateBank().addParam(std::make_pair<std::string,bitklavier::ParameterChangeBuffer*>(v.getProperty(IDs::uuid).toString().toStdString() + "_" + "circularTuning", &(state.params.keyboardState.stateChanges)));
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
    auto serial = Serializer::createBaseElement("tuning");
    paramHolder.doForAllParameters (
        [&serial] (auto& param, size_t)
        {
            chowdsp::ParameterTypeHelpers::serializeParameter<Serializer> (serial, param);
        });
    Serializer::template addChildElement<12>(serial,"circularTuning",paramHolder.keyboardState.circularTuningOffset,arrayToString);
    Serializer::template addChildElement<128>(serial,"absoluteTuning",paramHolder.keyboardState.absoluteTuningOffset,arrayToStringWithIndex);

    return serial;
}

template <typename Serializer>
void TuningParams::deserialize (typename Serializer::DeserializedType deserial, TuningParams& paramHolder)
{

    juce::StringArray paramIDsThatHaveBeenDeserialized {};
    if (const auto numParamIDsAndVals = Serializer::getNumAttributes (deserial))
    {
        for (int i = 0; i < numParamIDsAndVals; i += 1)
        {
            juce::String paramID {};
            paramID = Serializer::getAttributeName (deserial, i);

            paramHolder.doForAllParameters (
                [&deserial,
                 &paramID = std::as_const (paramID),
                 &paramIDsThatHaveBeenDeserialized] (auto& param, size_t)
                {
                    if (param.paramID == paramID)
                    {
                        chowdsp::ParameterTypeHelpers::deserializeParameter<Serializer> (deserial, param);
                        paramIDsThatHaveBeenDeserialized.add (paramID);

                    }
                });

        }
    }
    else
    {
       // jassertfalse; // state loading error
    }

    for(auto id: paramIDsThatHaveBeenDeserialized)
    {
        DBG("deserialzied " + id);
    }
    // set all un-matched objects to their default values
//    if (! paramIDsThatHaveBeenDeserialized.empty())
//    {
        paramHolder.doForAllParameters (
            [&paramIDsThatHaveBeenDeserialized] (auto& param, size_t)
            {
                if (! paramIDsThatHaveBeenDeserialized.contains (param.paramID))
                    chowdsp::ParameterTypeHelpers::resetParameter (param);
            });
//    }
    auto myStr  = deserial->getStringAttribute ("circularTuning");
    paramHolder.keyboardState.circularTuningOffset = parseFloatStringToArrayCircular<12>(myStr.toStdString());
    myStr  = deserial->getStringAttribute ("absoluteTuning");
    paramHolder.keyboardState.absoluteTuningOffset = parseIndexValueStringToArrayAbsolute<128>(myStr.toStdString());
    paramHolder.keyboardState.fundamental = paramHolder.fundamental->getIndex();



}
