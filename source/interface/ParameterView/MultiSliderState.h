//
// Created by Dan Trueman on 7/25/25.
//

#pragma once
#ifndef BITKLAVIER0_MULTISLIDERSTATE_H
#define BITKLAVIER0_MULTISLIDERSTATE_H

    #include "array_to_string.h"
    #include "chowdsp_plugin_state/chowdsp_plugin_state.h"

    /**
 * todo: make much larger (2048?) mostly for Pascal!
 * or only do that for MultiSlider2dState?
 */
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

    std::atomic<bool> updateUI;

    void processStateChanges() override
    {
        updateUI = false;
        for(auto [index, change] : stateChanges.changeState)
        {
            static juce::var nullVar;

            auto sval = change.getProperty (IDs::multislider_vals);
            auto svalsize = change.getProperty (IDs::multislider_size);
            auto aval = change.getProperty (IDs::multislider_states);
            auto avalsize = change.getProperty (IDs::multislider_states_size);

            if (sval != nullVar) {
                stringToAtomicArray(sliderVals, sval.toString(), 1.);
            }
            if (svalsize != nullVar) {
                sliderVals_size.store(int(svalsize));
            }
            if (aval != nullVar) {
                stringToAtomicBoolArray(activeSliders, aval.toString(), false);
            }
            if (avalsize != nullVar) {
                activeVals_size.store(int(avalsize));
            }
            updateUI = true;
        }

        // must clear at the end, otherwise they'll get reapplied again and again
        stateChanges.changeState.clear();

    }
};

/**
 * @brief Serializes a MultiSlider parameter set into a Serializer object.
 *
 * This function takes a struct containing multi-slider state, converts its
 * atomic arrays into string representations, and adds them to a Serializer
 * object with a given base ID.
 *
 * @tparam Serializer A template parameter for the Serializer type.
 * @param ser A pointer to the Serializer object.
 * @param msliderParam A const reference to the struct containing the multi-slider data.
 * @param thisSliderID A const reference to the base ID string for this slider set.
 */
template <typename Serializer>
void serializeMultiSliderParam(
    typename Serializer::SerializedType& ser,
    const MultiSliderState& msliderParam,
    const juce::String& thisSliderID)
{
    // Define the specific string IDs for serialization
    juce::String thisSlider_sizeID = thisSliderID + "_sliderVals_size";
    juce::String activeSlidersID = thisSliderID + "_activeVals";
    juce::String activeSliders_sizeID = activeSlidersID + "_size";

    // Serialize the float slider values
    juce::String sliderVals_str = atomicArrayToStringLimited(msliderParam.sliderVals, msliderParam.sliderVals_size);
    Serializer::addChildElement(ser, thisSlider_sizeID, juce::String(msliderParam.sliderVals_size));
    Serializer::addChildElement(ser, thisSliderID+"_sliderVals", sliderVals_str);

    // Serialize the boolean active sliders
    juce::String activeSliders_str = atomicArrayToStringLimited(msliderParam.activeSliders, msliderParam.activeVals_size);
    Serializer::addChildElement(ser, activeSliders_sizeID, juce::String(msliderParam.activeVals_size));
    Serializer::addChildElement(ser, activeSlidersID, activeSliders_str);
}

/**
 * @brief Deserializes and populates a MultiSliderState struct from a deserialized object.
 *
 * This function reads specific string attributes from a deserialized object,
 * parses them, and populates the members of a MultiSliderState struct, including
 * atomic arrays for slider values and active states.
 *
 * @tparam Serializer A template parameter for the Serializer type.
 * @param deserial The deserialized object to read attributes from.
 * @param msliderParam A reference to the MultiSliderState struct to be populated.
 * @param thisSliderID The base ID string for this slider set.
 */
template <typename Serializer>
void deserializeMultiSliderParam(
    typename Serializer::DeserializedType deserial,
    MultiSliderState& msliderParam,
    const juce::String& thisSliderID,
    float defaultVal = 1.f)
{
    // Reconstruct the attribute names using the base ID
    juce::String thisSlider_sizeID = thisSliderID + "_sliderVals_size";
    juce::String activeSlidersID = thisSliderID + "_activeVals";
    juce::String activeSliders_sizeID = activeSlidersID + "_size";

    // Deserialize the slider values
    auto myStr = deserial->getStringAttribute(thisSlider_sizeID);
    msliderParam.sliderVals_size = myStr.getIntValue();
    myStr = deserial->getStringAttribute(thisSliderID+"_sliderVals");
    std::vector<float> sliderVals_vec = parseStringToVector<float>(myStr);
    populateAtomicArrayFromVector(msliderParam.sliderVals, defaultVal, sliderVals_vec);

    // Deserialize the active sliders
    myStr = deserial->getStringAttribute(activeSliders_sizeID);
    msliderParam.activeVals_size = myStr.getIntValue();
    myStr = deserial->getStringAttribute(activeSlidersID);
    std::vector<bool> activeSliders_vec = parseStringToBoolVector(myStr);
    populateAtomicArrayFromVector(msliderParam.activeSliders, false, activeSliders_vec);
}

#endif //BITKLAVIER0_MULTISLIDERSTATE_H
