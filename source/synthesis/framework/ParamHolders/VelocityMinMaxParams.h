//
// Created by Dan Trueman on 5/7/25.
//

#ifndef BITKLAVIER2_VELOCITYMINMAXPARAMS_H
#define BITKLAVIER2_VELOCITYMINMAXPARAMS_H
#include <PreparationStateImpl.h>
#include <chowdsp_plugin_utils/chowdsp_plugin_utils.h>

struct VelocityMinMaxParams : chowdsp::ParamHolder
{
    VelocityMinMaxParams() : chowdsp::ParamHolder("VELOCITYMINMAX")
    {
        add(velocityMinParam, velocityMaxParam);
    }

    chowdsp::FloatParameter::Ptr velocityMinParam {
        juce::ParameterID { "velocitymin", 100 },
        "Min Velocity",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 128.0f, 64.0f),
        0.0f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };

    chowdsp::FloatParameter::Ptr velocityMaxParam {
        juce::ParameterID { "velocitymax", 100 },
        "Max Velocity",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 128.0f, 64.0f),
        128.0f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };

    // Last velocity param
    /*
     * this will be updated in BKSynthesizer, and then can be accessed for display in the velocityMinMaxslider
     *      it should NOT be added to the param list above, as we are not going to save/recall it, etc...
     */
//    chowdsp::FloatParameter::Ptr lastVelocityParam {
//        juce::ParameterID { "LastVelocity", 100 },
//        "LastVelocity",
//        chowdsp::ParamUtils::createNormalisableRange (0.0f, 127.0f, 63.f), // FIX
//        127.0f,
//        &chowdsp::ParamUtils::floatValToString,
//        &chowdsp::ParamUtils::stringToFloatVal
//    };

    void processStateChanges() override
    {
        auto float_params = getFloatParams();
        for(auto [index, change] : stateChanges.changeState)
        {
            auto vminval = change.getProperty("velocitymin");
            velocityMinParam->setParameterValue(vminval);

            auto vmaxval = change.getProperty("velocitymax");
            velocityMaxParam->setParameterValue(vmaxval);
        }
        stateChanges.changeState.clear();
    }
};

#endif //BITKLAVIER2_VELOCITYMINMAXPARAMS_H
