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
        add(velocityMinParam, velocityMaxParam,lastVelocityParam);
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
    /**
     * this will be updated in BKSynthesizer (via lastSynthState), and then can be accessed for display
     * in the velocityMinMaxslider
     *
     * note that we need to add a callback in OpenGL_VelocityMinMaxSlider.h so that the slider is notified
     * when this value changes. since we are using a legacy bK UI component for the velocityMinMax slider
     * we need this callback, to trigger a redraw and so on, something we don't need to do with the
     * newer OpenGL components we're using (like the levelMeter)
     */
    chowdsp::FloatParameter::Ptr lastVelocityParam {
        juce::ParameterID { "LastVelocity", 100 },
        "LastVelocity",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 127.0f, 63.f),
        127.0f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };

    /**
     * todo: is this needed? velocitymin and max are handled above, no?
     */
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
