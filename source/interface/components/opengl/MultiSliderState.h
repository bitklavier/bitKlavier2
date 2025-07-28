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
    std::array<std::atomic<float>, MAXMULTISLIDERLENGTH> sliderVals = {1.f};
    std::array<std::atomic<bool>, MAXMULTISLIDERLENGTH> activeSliders = {true};
    std::atomic<int> numActiveSliders = 1;
};

#endif //BITKLAVIER0_MULTISLIDERSTATE_H
