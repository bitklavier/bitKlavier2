//
// Created by Davis Polito on 6/3/25.
//

#ifndef ARRAY_TO_STRING_H
#define ARRAY_TO_STRING_H
#include <array>
#include <sstream>
#include <string>
#include <juce_core/juce_core.h>
template <std::size_t N>
std::array<float, N> parseIndexValueStringToArrayAbsolute(const std::string& input)
{
    std::array<float, N> result{0};
    std::istringstream iss(input);
    std::string token;

    while (iss >> token)
    {
        auto colonPos = token.find(':');
        if (colonPos == std::string::npos) continue;

        int index = std::stoi(token.substr(0, colonPos));
        float value = std::stof(token.substr(colonPos + 1));

        if (index >= 0 && static_cast<std::size_t>(index) < N)
            result[index] = value;
    }

    return result;
}
template <std::size_t N>
std::array<float, N> parseFloatStringToArrayCircular(const std::string& input){
    std::array<float, N> result{};
    std::istringstream iss(input);
    float value;
    std::size_t i = 0;

    while (i < N && iss >> value)
    {
        result[i++] = value;
    }

    // If fewer floats than N, remaining values stay zero (default initialized)
    return result;
}

juce::String getOnKeyString(const std::bitset<128>& bits);

//juce::String arrayToString(const std::array<float, 12>& array);
template <size_t Size>
juce::String arrayToString(const std::array<float, Size>& array);
//template <size_t Size> // <--- Templated for any size
//juce::String arrayToString(const std::array<std::atomic<float>, Size>& array); // <--- Takes std::atomic<float> array

//juce::String arrayToStringWithIndex(const std::array<float, 128>& array);
template <size_t Size>
juce::String arrayToStringWithIndex(const std::array<float, Size>& array);
//template <size_t Size> // <--- Templated for any size
//juce::String arrayToStringWithIndex(const std::array<std::atomic<float>, Size>& array) // <--- Takes std::atomic<float> array

#endif //ARRAY_TO_STRING_H
