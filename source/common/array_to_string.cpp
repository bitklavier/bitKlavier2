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

//juce::String arrayToStringWithIndex(const std::array<float, 128>& array) {
//    juce::String s = "";
//    int key = 0;
//    for (auto offset :array)
//    {
//        //if (offset != 0.0)  s += String(key) + ":" + String((int)(offset*100.0f)) + " ";
//        //DBG("offsetArrayToString3 val = " + juce::String(offset));
//        if (offset != 0.f)  s += juce::String(key) + ":" + juce::String((offset)) + " ";
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

