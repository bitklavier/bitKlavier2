//
// Created by Dan Trueman on 7/25/25.
//

#pragma once
#ifndef BITKLAVIER0_MULTISLIDERSTATE_H
#define BITKLAVIER0_MULTISLIDERSTATE_H

#include <chowdsp_plugin_state/chowdsp_plugin_state.h>
#include "array_to_string.h"

#define MAXMULTISLIDERLENGTH 128

/**
 * for a multislider, we have a sequence of values, sometimes with gaps,
 *  inactive sliders that are skipped but available for the user to change anytime.
 *
 *  sliderVals holds the sequence of values without gaps, and activeSliders holds which sliders are active
 *
 *  ex:
 *      values in the UI slider: 1 2 / 3 / 4
 *          (/ => gap, as represented in the text editor for the slider)
 *
 *  leads to:
 *      sliderVals      = {1, 2, 3, 4},                             with sliderVals_size = 4
 *      activeSliders   = {true, true, false, true, false, true},   with activeSliders_size = 6
 *
 * for blendrónic at least (and I think all of the bK preps),we only need the activeSliders for saving/restoring the UI.
 *      internally, blendrónic only needs sliderVals and sliderVals_size
 */
struct MultiSliderState : bitklavier::StateChangeableParameter
{
    std::array<std::atomic<float>, MAXMULTISLIDERLENGTH> sliderVals = {1.f};
    std::array<std::atomic<bool>, MAXMULTISLIDERLENGTH> activeSliders = {true}; // could change to std::bitset

    /*
     * how many sliders is the user actually working with (int)
     *      - there will always be at least 1
     *      - and 12 displayed (handled internally by BKSliders::BKMultiSlider)
     *      - but the user might be using any number 1 up to MAXMULTISLIDERLENGTH
     *      - and some might be inactive
     *      - need this in part because we can't use sliderVals.size(), since sliderVals is fixed length
     */
    std::atomic<int> sliderVals_size = 1;
    std::atomic<int> activeVals_size = 1;

};

#endif //BITKLAVIER0_MULTISLIDERSTATE_H
