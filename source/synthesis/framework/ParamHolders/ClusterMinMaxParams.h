//
// Created by Dan Trueman on 8/6/25.
//

/**
 * Yes, we should be able to generalize the Min/Max params
 * and sliders and not have separate ones, but it is
 * surprisingly tricky to make work with the chowdsp param
 * implementation. Since we really only have three types
 * of min/max sliders in bK, and no more than two in any
 * particular prep, I've decided to make the three types
 * individually for now.
 *
 * Can revisit later if there is a good reason to do so,
 * but after many hours in conversation with Gemini about this,
 * I'm moving on.... and the complex pointer-based solution
 * I had is considerably less readable than this solution,
 * and readability is a prioirty here.
 */

#ifndef BITKLAVIER0_CLUSTERMINMAXPARAMS_H
#define BITKLAVIER0_CLUSTERMINMAXPARAMS_H
#include <PreparationStateImpl.h>
#include <chowdsp_plugin_utils/chowdsp_plugin_utils.h>

static float clusterMinMax_rangeMin = 1.f;
static float clusterMinMax_rangeMax = 12.f;
static float clusterMinMax_rangeMid = 6.f;

struct ClusterMinMaxParams : chowdsp::ParamHolder
{
    ClusterMinMaxParams() : chowdsp::ParamHolder("Cluster")
    {
        add(clusterMinParam, clusterMaxParam, lastClusterParam);
    }

    chowdsp::FloatParameter::Ptr clusterMinParam {
        juce::ParameterID { "clustermin", 100 },
        "Cluster Min",
        chowdsp::ParamUtils::createNormalisableRange (clusterMinMax_rangeMin, clusterMinMax_rangeMax, clusterMinMax_rangeMid),
        1.0f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };

    chowdsp::FloatParameter::Ptr clusterMaxParam {
        juce::ParameterID { "clustermax", 100 },
        "Cluster Max",
        chowdsp::ParamUtils::createNormalisableRange (clusterMinMax_rangeMin, clusterMinMax_rangeMax, clusterMinMax_rangeMid),
        12.0f,
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
    chowdsp::FloatParameter::Ptr lastClusterParam {
        juce::ParameterID { "LastCluster", 100 },
        "LastCluster",
        chowdsp::ParamUtils::createNormalisableRange (clusterMinMax_rangeMin, clusterMinMax_rangeMax, clusterMinMax_rangeMid),
        1.0f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
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
            auto vminval = change.getProperty("clustermin");
            clusterMinParam->setParameterValue(vminval);

            auto vmaxval = change.getProperty("clustermax");
            clusterMaxParam->setParameterValue(vmaxval);
        }
        stateChanges.changeState.clear();
    }
};

#endif //BITKLAVIER0_CLUSTERMINMAXPARAMS_H
