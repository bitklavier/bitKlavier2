// Copyright (C) 2022-2025 Dan Trueman
// SPDX-License-Identifier: GPL-3.0-or-later

//
// Created by Dan Trueman on 8/6/25.
//

#ifndef BITKLAVIER0_HOLDTIMEMINMAXPARAMS_H
#define BITKLAVIER0_HOLDTIMEMINMAXPARAMS_H

#include <PreparationStateImpl.h>
#include <chowdsp_plugin_utils/chowdsp_plugin_utils.h>

static float holdTimeMinMax_rangeMin = 0.f;
static float holdTimeMinMax_rangeMax = 120000.f;
static float holdTimeMinMax_rangeMid = 60000.f;

struct HoldTimeMinMaxParams : chowdsp::ParamHolder
{
    HoldTimeMinMaxParams() : chowdsp::ParamHolder("HoldTime")
    {
        add(holdTimeMinParam, holdTimeMaxParam);
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
        60000.0f
    };

    /**
     * Last measured hold time (ms), written from the audio thread by the owning processor
     * and polled by the parameter view's Timer. Lock-free std::atomic; mirrors the
     * VelocityMinMaxParams::lastVelocityParam pattern. Not part of the persisted state.
     */
    std::atomic<float> lastHoldTimeParam { 0.0f };

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
