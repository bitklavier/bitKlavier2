// Copyright (C) 2022-2025 Dan Trueman
// SPDX-License-Identifier: GPL-3.0-or-later

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
        add(clusterMinParam, clusterMaxParam);
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

    /**
     * Last measured cluster size, written from the audio thread by the owning processor
     * and polled by the parameter view's Timer. Lock-free std::atomic; mirrors the
     * VelocityMinMaxParams::lastVelocityParam pattern. Not part of the persisted state.
     */
    std::atomic<float> lastClusterParam { 1.0f };

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
