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

    void processStateChanges() override
    {
        for(auto [index, change] : stateChanges.changeState)
        {
            static juce::var nullVar;
            /**
             * todo: change all these property names to IDs.
             */
            auto sval = change.getProperty ("sliderVals");
            auto svalsize = change.getProperty ("sliderVals_size");
            auto aval = change.getProperty ("activeVals");
            auto avalsize = change.getProperty ("activeVals_size");

            if (sval != nullVar) {
                DBG("MultiSliderState sliderVals: " + sval.toString());
                stringToAtomicArray(sliderVals, sval.toString(), 1.);
            }
            if (svalsize != nullVar) {
                DBG("MultiSliderState sliderVals_size: " + svalsize.toString());
                sliderVals_size.store(int(svalsize));
            }
            if (aval != nullVar) {
                DBG("MultiSliderState activeVals: " + aval.toString());
                stringToAtomicBoolArray(activeSliders, aval.toString(), false);
            }
            if (avalsize != nullVar) {
                DBG("MultiSliderState activeVals_size " + avalsize.toString());
                activeVals_size.store(int(avalsize));
            }
        }

        // must clear at the end, otherwise they'll get reapplied again and again
        stateChanges.changeState.clear();
    }
};

#endif //BITKLAVIER0_MULTISLIDERSTATE_H
