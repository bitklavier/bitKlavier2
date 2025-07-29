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

    /*
     * how many sliders is the user actually working with (int)
     *      - there will always be at least 1
     *      - and 12 displayed (handled internally by BKSliders::BKMultiSlider)
     *      - but the user might be using any number 1 up to MAXMULTISLIDERLENGTH
     *      - and some might be inactive
     */
    std::atomic<int> numActiveSliders = 1;
};

#endif //BITKLAVIER0_MULTISLIDERSTATE_H
