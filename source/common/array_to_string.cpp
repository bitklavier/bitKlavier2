//
// Created by Davis Polito on 6/3/25.
//
#include "array_to_string.h"

//juce::String arrayToString(const std::array<float, 12>& array) {
//    juce::String s = "";
//
//    for (auto offset : array) {
//        s += juce::String((offset)) + " ";
//    }
//    return s;
//}

template <size_t Size>
juce::String arrayToString(const std::array<float, Size>& array) {
    juce::String s = "";

    for (auto offset : array) {
        s += juce::String((offset)) + " ";
    }
    return s;
}

/**
 * Converts a std::array of std::atomic<float> values of any size into a single juce::String.
 * Each float value (obtained atomically) is converted to a string and separated by a space.
 *
 * @tparam Size The size of the std::array.
 * @param array The std::array<std::atomic<float>, Size> to convert.
 * @return A juce::String containing the space-separated float values.
 */
//template <size_t Size>
//juce::String arrayToString(const std::array<std::atomic<float>, Size>& array)
//{
//    juce::String s = ""; // Initialize an empty JUCE String
//
//    // Iterate through each std::atomic<float> value in the array
//    for (const auto& atomicOffset : array) // Changed loop variable to reflect atomic type
//    {
//        // Atomically load the float value and convert it to a juce::String,
//        // then append it, followed by a space.
//        s += juce::String(atomicOffset.load()) + " "; // Explicitly call .load()
//    }
//
//    // Return the concatenated string
//    return s;
//}

juce::String arrayToStringWithIndex(const std::array<float, 128>& array) {
    juce::String s = "";
    int key = 0;
    for (auto offset :array)
    {
        //if (offset != 0.0)  s += String(key) + ":" + String((int)(offset*100.0f)) + " ";
        //DBG("offsetArrayToString3 val = " + juce::String(offset));
        if (offset != 0.f)  s += juce::String(key) + ":" + juce::String((offset)) + " ";

        ++key;
    }
    return s;
}

template <size_t Size>
juce::String arrayToStringWithIndex(const std::array<float, Size>& array) {
    juce::String s = "";
    int key = 0;
    for (auto offset :array)
    {
        //if (offset != 0.0)  s += String(key) + ":" + String((int)(offset*100.0f)) + " ";
        //DBG("offsetArrayToString3 val = " + juce::String(offset));
        if (offset != 0.f)  s += juce::String(key) + ":" + juce::String((offset)) + " ";

        ++key;
    }
    return s;
}

/**
 * Converts a std::array of std::atomic<float> values of any size into a single juce::String,
 * including their index if the value is non-zero.
 *
 * @tparam Size The size of the std::array.
 * @param array The std::array<std::atomic<float>, Size> to convert.
 * @return A juce::String containing "index:value" pairs for non-zero values, separated by spaces.
 */
//template <size_t Size> // <--- Templated for any size
//juce::String arrayToStringWithIndex(const std::array<std::atomic<float>, Size>& array) // <--- Takes std::atomic<float> array
//{
//    juce::String s = "";
//    int key = 0;
//    for (const auto& atomicOffset : array) // <--- Iterate over std::atomic<float> elements
//    {
//        float offset = atomicOffset.load(); // <--- Atomically load the float value
//
//        // if (offset != 0.0)  s += String(key) + ":" + String((int)(offset*100.0f)) + " ";
//        // DBG("offsetArrayToString3 val = " + juce::String(offset)); // If DBG is used, ensure it loads atomically
//        if (offset != 0.f)
//        {
//            s += juce::String(key) + ":" + juce::String(offset) + " ";
//        }
//
//        ++key;
//    }
//    return s;
//}


juce::String getOnKeyString(const std::bitset<128> &bits)
    {
        std::ostringstream oss;
        bool first = true;

        for (size_t i = 0; i < bits.size(); ++i) {
            if (bits.test(i)) {
                if (!first) oss << ' ';
                oss << i;
                first = false;
            }
        }

        return juce::String(oss.str());
    }

