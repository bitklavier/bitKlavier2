//
// Created by Dan Trueman on 11/5/24.
//

#ifndef BITKLAVIER2_ENVPARAMS_H
#define BITKLAVIER2_ENVPARAMS_H
#include <PreparationStateImpl.h>
#include <chowdsp_plugin_utils/chowdsp_plugin_utils.h>
struct EnvParams : public chowdsp::ParamHolder
{
    EnvParams() : chowdsp::ParamHolder("ENV")
    {
        add(decayParam, sustainParam, releaseParam, attackParam, holdParam, delayParam);
    }

    // Delay param
    chowdsp::TimeMsParameter::Ptr delayParam {
        juce::ParameterID { "delay", 100 },
        "Delay",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 1000.0f, 500.0f),
        0.0f
    };

    // Attack param
    chowdsp::TimeMsParameter::Ptr attackParam {
        juce::ParameterID { "attack", 100 },
        "Attack",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 1000.0f, 500.0f),
        0.0f
    };

    // Attack Power param
    chowdsp::FloatParameter::Ptr attackPowerParam {
        juce::ParameterID { "attackpower", 100 },
        "Attack Power",
        chowdsp::ParamUtils::createNormalisableRange (-10.0f, 10.0f, 0.0f),
        0.0f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };

    // Hold param
    chowdsp::TimeMsParameter::Ptr holdParam {
        juce::ParameterID { "hold", 100 },
        "Hold",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 1000.0f, 500.0f),
        0.0f
    };

    // Decay param
    chowdsp::TimeMsParameter::Ptr decayParam {
        juce::ParameterID { "decay", 100 },
        "Decay",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 1000.0f, 500.0f),
        0.0f
    };

    // Sustain param
    chowdsp::FloatParameter::Ptr sustainParam {
        juce::ParameterID { "sustain", 100 },
        "Sustain",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 1.0f, 0.5f),
        1.0f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };

    // Release param
    chowdsp::TimeMsParameter::Ptr releaseParam {
        juce::ParameterID { "release", 100 },
        "Release",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 1000.0f, 500.0f),
        10.0f
    };

};
#endif //BITKLAVIER2_ENVPARAMS_H
