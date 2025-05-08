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

//    void processStateChanges() override
//    {
//        auto float_params = getFloatParams();
//        for(auto [index, change] : stateChanges.changeState)
//        {
//
//            static juce::var nullVar;
//            for (int i = 0; i< 11; i++)
//            {
//                auto str = "t" + juce::String(i);
//                auto val = change.getProperty(str);
//                if (val == nullVar)
//                    break;
//                auto& float_param = float_params->at(i);
//                float_param.get()->setParameterValue(val);
//                //float_params[i].data()->get()->setParameterValue(val);
//            }
//        }
//        stateChanges.changeState.clear();
//    }
};

#endif //BITKLAVIER2_VELOCITYMINMAXPARAMS_H
