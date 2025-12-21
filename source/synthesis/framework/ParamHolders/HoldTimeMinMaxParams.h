//
// Created by Dan Trueman on 8/6/25.
//

#ifndef BITKLAVIER0_HOLDTIMEMINMAXPARAMS_H
#define BITKLAVIER0_HOLDTIMEMINMAXPARAMS_H

#include <PreparationStateImpl.h>
#include <chowdsp_plugin_utils/chowdsp_plugin_utils.h>

static float holdTimeMinMax_rangeMin = 0.f;
static float holdTimeMinMax_rangeMax = 12000.f;
static float holdTimeMinMax_rangeMid = 1500.f;

struct HoldTimeMinMaxParams : chowdsp::ParamHolder
{
    HoldTimeMinMaxParams() : chowdsp::ParamHolder("HoldTime")
    {
        add(holdTimeMinParam, holdTimeMaxParam, lastHoldTimeParam);
    }

    chowdsp::TimeMsParameter::Ptr holdTimeMinParam {
        juce::ParameterID { "holdTimeMinParam", 100 },
        "Hold Time Min",
        chowdsp::ParamUtils::createNormalisableRange (holdTimeMinMax_rangeMin, holdTimeMinMax_rangeMax, holdTimeMinMax_rangeMid),
        0.0f // these are state modulatable, so not true for last arg, which is for continuous mod
    };

    chowdsp::TimeMsParameter::Ptr holdTimeMaxParam {
        juce::ParameterID { "holdTimeMaxParam", 100 },
        "Hold Time Max",
        chowdsp::ParamUtils::createNormalisableRange (holdTimeMinMax_rangeMin, holdTimeMinMax_rangeMax, holdTimeMinMax_rangeMid),
        12000.0f
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
    chowdsp::TimeMsParameter::Ptr lastHoldTimeParam {
        juce::ParameterID { "lastHoldTimeParam", 100 },
        "lastHoldTimeParam",
        chowdsp::ParamUtils::createNormalisableRange (holdTimeMinMax_rangeMin, holdTimeMinMax_rangeMax, holdTimeMinMax_rangeMid),
        0.0f
    };

    /**
     * this is called every block, but doesn't do anything unless there is a "changeState"
     * in "stateChanges" to take care of, initiated by a state change modulation triggered by the user
     */
    void processStateChanges() override
    {
        //auto float_params = getFloatParams();
        for(auto [index, change] : stateChanges.changeState)
        {
            auto vminval = change.getProperty("holdtimemin");
            holdTimeMinParam->setParameterValue(vminval);

            auto vmaxval = change.getProperty("holdtimemax");
            holdTimeMaxParam->setParameterValue(vmaxval);
        }
        stateChanges.changeState.clear();
    }
};

#endif //BITKLAVIER0_HOLDTIMEMINMAXPARAMS_H
