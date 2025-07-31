//
// Created by Davis Polito on 6/3/25.
//
#pragma once

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

/**
 * Parses a string containing "index:value" pairs and stores them into a
 * pre-existing std::array<std::atomic<float>, N>.
 * Values are stored atomically.
 *
 * @tparam N The size of the std::array.
 * @param input The input string (e.g., "0:1.0 5:2.5 12:3.0").
 * @param outputArray A reference to the std::array<std::atomic<float>, N> to populate.
 * It should be initialized by the caller (e.g., all zeros).
 */
template <std::size_t N>
void parseIndexValueStringToAtomicArray(const std::string& input,
    std::array<std::atomic<float>, N>& outputArray) // <--- Pass by reference
{
    std::istringstream iss(input);
    std::string token;

    while (iss >> token)
    {
        auto colonPos = token.find(':');
        if (colonPos == std::string::npos)
        {
            std::cerr << "Warning: Malformed token '" << token << "' in input string." << std::endl;
            continue;
        }

        try
        {
            int index = std::stoi(token.substr(0, colonPos));
            float value = std::stof(token.substr(colonPos + 1));

            if (index >= 0 && static_cast<std::size_t>(index) < N)
            {
                outputArray[index].store(value); // <--- Use .store() for atomic write
            }
            else
            {
                std::cerr << "Warning: Index " << index << " out of bounds [0, " << N - 1 << "] for token '" << token << "'." << std::endl;
            }
        }
        catch (const std::invalid_argument& e)
        {
            std::cerr << "Error: Invalid argument for stoi/stof in token '" << token << "': " << e.what() << std::endl;
        }
        catch (const std::out_of_range& e)
        {
            std::cerr << "Error: Value out of range for stoi/stof in token '" << token << "': " << e.what() << std::endl;
        }
    }
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

/**
 * Parses a string containing space-separated float values and stores them into a
 * pre-existing std::array<std::atomic<float>, N> in a circular fashion.
 * Values are stored atomically.
 *
 * @tparam N The size of the std::array.
 * @param input The input string (e.g., "1.0 2.5 3.0").
 * @param outputArray A reference to the std::array<std::atomic<float>, N> to populate.
 * It should be initialized by the caller (e.g., all zeros).
 */
template <std::size_t N>
void parseFloatStringToAtomicArrayCircular(const std::string& input, std::array<std::atomic<float>, N>& outputArray)
{
    std::istringstream iss(input);
    float value;
    std::size_t i = 0;

    // Read values from the string until the array is full or no more values are available
    while (i < N && iss >> value)
    {
        outputArray[i++].store(value); // Atomically store the value
    }

    // If fewer floats than N were read, the remaining atomic values will retain
    // their initial state (e.g., 0.0f if the array was pre-initialized to zeros).
    // No explicit action needed here to set remaining to zero, as it's the caller's
    // responsibility to initialize outputArray.
}

/**
 * Copies values from one std::array of std::atomic<float> to another std::array of std::atomic<float>.
 * Each value is atomically loaded from the source and atomically stored into the destination.
 *
 * @tparam Size The size of the arrays.
 * @param sourceArray The std::array<std::atomic<float>, Size> to copy from.
 * @param destinationArray The std::array<std::atomic<float>, Size> to copy to.
 */
template <size_t Size>
void copyAtomicArrayToAtomicArray(const std::array<std::atomic<float>, Size>& sourceArray,
    std::array<std::atomic<float>, Size>& destinationArray)
{
    for (size_t i = 0; i < Size; ++i)
    {
        // Atomically load the value from the source atomic array
        float value = sourceArray[i].load();
        // Atomically store the value into the destination atomic array
        destinationArray[i].store(value);
    }
}

juce::String getOnKeyString(const std::bitset<128> &bits);

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
 * This is a direct atomic version of the original arrayToString.
 *
 * @tparam Size The size of the std::array.
 * @param array The std::array<std::atomic<float>, Size> to convert.
 * @return A juce::String containing the space-separated float values.
 */
template <size_t Size>
juce::String atomicArrayToString(const std::array<std::atomic<float>, Size>& array)
{
    juce::String s = ""; // Initialize an empty JUCE String

    // Iterate through each std::atomic<float> value in the array
    for (const auto& atomicOffset : array)
    {
        // Atomically load the float value and convert it to a juce::String,
        // then append it, followed by a space.
        s += juce::String(atomicOffset.load()) + " "; // Explicitly call .load()
    }

    // Return the concatenated string
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
template <size_t Size>
juce::String arrayOfAtomicsToString(const std::array<std::atomic<float>, Size>& array)
{
    juce::String s = ""; // Initialize an empty JUCE String

    // Iterate through each std::atomic<float> value in the array
    for (const auto& atomicOffset : array) // Changed loop variable to reflect atomic type
    {
        // Atomically load the float value and convert it to a juce::String,
        // then append it, followed by a space.
        s += juce::String(atomicOffset.load()) + " "; // Explicitly call .load()
    }

    // Return the concatenated string
    return s;
}

template <size_t Size>
void copyAtomicArrayToFloatArray(const std::array<std::atomic<float>, Size>& sourceArray, std::array<float, Size>& destinationArray)
{
    for (size_t i = 0; i < Size; ++i)
    {
        // Atomically load the value from the source atomic array
        // and assign it to the corresponding element in the destination float array.
        destinationArray[i] = sourceArray[i].load();
    }
}

//juce::String arrayToStringWithIndex(const std::array<float, 128>& array);

/**
 * Converts a std::array of std::atomic<float> values of any size into a single juce::String,
 * including their index if the value is non-zero.
 *
 * @tparam Size The size of the std::array.
 * @param array The std::array<std::atomic<float>, Size> to convert.
 * @return A juce::String containing "index:value" pairs for non-zero values, separated by spaces.
 */
template <size_t Size>
juce::String atomicArrayToStringWithIndex(const std::array<std::atomic<float>, Size>& array)
{
    juce::String s = "";
    int key = 0; // Use int for index as it's common for loop counters and string conversion

    // Iterate through each std::atomic<float> element in the array
    for (const auto& atomicOffset : array)
    {
        // Atomically load the float value from the current atomic element
        float offset = atomicOffset.load();

        // Check if the value is non-zero (using a small epsilon for float comparison is safer,
        // but direct comparison to 0.f is often sufficient if 0.f is an exact sentinel value).
        if (offset != 0.f)
        {
            // Append "index:value " to the string
            s += juce::String(key) + ":" + juce::String(offset) + " ";
        }

        ++key; // Increment the index for the next element
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

template <size_t Size1, size_t Size2>
std::array<float, Size1> multiSliderArraysToFloatArray(const std::array<std::atomic<float>, Size1>& sliderVals, const std::array<std::atomic<bool>, Size2>& activeSliders, int outputValSize) {

    std::array<float, Size1> returnArray;

    int stateCtr = 0;       // counting through the activeSliders array
    int valueCounter = 0;   // counting through the sliderVals and returnArray arrays

    //if a slider is active, store its value in the output array, otherwise ignore
    for (auto& bval : activeSliders)
    {
        if (bval)
        {
            returnArray[valueCounter] = sliderVals[valueCounter].load(); // 1d for now....
            valueCounter++;
        }
        stateCtr++;

        if (valueCounter >= outputValSize)
        {
            return returnArray;
        }
    }

    return returnArray;
}


#endif //ARRAY_TO_STRING_H
