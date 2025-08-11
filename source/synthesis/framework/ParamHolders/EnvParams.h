//
// Created by Dan Trueman on 11/5/24.
//
#pragma once

#ifndef BITKLAVIER2_ENVPARAMS_H
#define BITKLAVIER2_ENVPARAMS_H
#include <PreparationStateImpl.h>
#include <chowdsp_plugin_utils/chowdsp_plugin_utils.h>

struct EnvParams : public chowdsp::ParamHolder
{
    EnvParams() : chowdsp::ParamHolder("ENV")
    {
        add(
            attackParam,
            decayParam,
            sustainParam,
            releaseParam,
            attackPowerParam,
            decayPowerParam,
            releasePowerParam,
            holdParam,
            delayParam,
            notify
            );
    }

    /**
     * note: we are not currently using delay or hold but need to add
     *          these for now to work with envelope_editor without
     *          doing a lot of surgery on that code.
     *          also, we might want to use them later, so we keep
     *          them as placeholders for now
     */

    // Delay param
    chowdsp::TimeMsParameter::Ptr delayParam {
        juce::ParameterID { "delay", 100 },
        "Delay",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 1000.0f, 500.0f),
        0.0f, true
    };

    // Attack param
    chowdsp::TimeMsParameter::Ptr attackParam {
        juce::ParameterID { "attack", 100 },
        "Attack",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 10000.0f, 500.0f),
        3.0f, true
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
        10.0f,true
    };

    // Decay Power param
    chowdsp::FloatParameter::Ptr decayPowerParam {
        juce::ParameterID { "decaypower", 100 },
        "Decay Power",
        chowdsp::ParamUtils::createNormalisableRange (-10.0f, 10.0f, 0.0f),
        0.0f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };

    // Sustain param
    chowdsp::FloatParameter::Ptr sustainParam {
        juce::ParameterID { "sustain", 100 },
        "Sustain",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 1.0f, 0.5f),
        1.0f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal, true
    };

    // Release param
    chowdsp::TimeMsParameter::Ptr releaseParam {
        juce::ParameterID { "release", 100 },
        "Release",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 10000.0f, 500.0f),
        50.0f,true
    };

    // release Power param
    chowdsp::FloatParameter::Ptr releasePowerParam {
        juce::ParameterID { "releasepower", 100 },
        "Release Power",
        chowdsp::ParamUtils::createNormalisableRange (-10.0f, 10.0f, 0.0f),
        0.0f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };

    chowdsp::BoolParameter::Ptr notify {
        juce::ParameterID { "notify", 100 },
        "notify",
        false
    };
};
#endif //BITKLAVIER2_ENVPARAMS_H
