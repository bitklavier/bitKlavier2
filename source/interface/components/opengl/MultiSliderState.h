//
// Created by Dan Trueman on 7/25/25.
//

#pragma once
#ifndef BITKLAVIER0_MULTISLIDERSTATE_H
#define BITKLAVIER0_MULTISLIDERSTATE_H

#include <chowdsp_plugin_state/chowdsp_plugin_state.h>
#define MAXMULTISLIDERLENGTH 128

struct MultiSliderState : bitklavier::StateChangeableParameter
{
    std::array<float, MAXMULTISLIDERLENGTH> sliderVals = {1.f};

    /*
     * how many sliders is the user actually working with (int)
     *      - there will always be at least 1
     *      - and 12 displayed
     *      - but the user might be using any number 1 up to MAXMULTISLIDERLENGTH
     */
    chowdsp::FloatParameter::Ptr numSlidersActual {
        juce::ParameterID { "numSlidersActual", 100 },
        "numSlidersActual",
        chowdsp::ParamUtils::createNormalisableRange (1.0f, static_cast<float>(MAXMULTISLIDERLENGTH), 64.0f),
        1.0f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };
};

#endif //BITKLAVIER0_MULTISLIDERSTATE_H
