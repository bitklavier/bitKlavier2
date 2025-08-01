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

juce::String getFirstValueFromSubarrays(const juce::Array<juce::Array<float>>& values);
juce::String arrayBoolToString(const juce::Array<bool>& states);

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
 * @brief Parses a string of numbers into a std::vector.
 *
 * This templated function takes a juce::String and a delimiter and
 * extracts space-separated numerical values, converting them to the
 * specified type (e.g., float, int).
 *
 * @tparam T The type of the numbers to parse (e.g., float, int).
 * @param input The juce::String containing the space-separated numbers.
 * @return A std::vector<T> containing the parsed numbers.
 */
template <typename T>
std::vector<T> parseStringToVector(const juce::String& input)
{
    std::vector<T> result;

    // The JUCE String can be converted to a std::string to use with stringstream.
    std::string s = input.toStdString();
    std::stringstream ss(s);

    T value;

    // The loop will automatically extract whitespace-separated values.
    while (ss >> value)
    {
        result.push_back(value);
    }

    return result;
}

/**
 * @brief Specialization for boolean parsing.
 *
 * This specialized function handles parsing "true" and "false" string literals
 * into a std::vector<bool>. It uses std::boolalpha to correctly interpret the strings.
 *
 * @param input The juce::String containing the space-separated boolean values.
 * @return A std::vector<bool> containing the parsed booleans.
 */
std::vector<bool> parseStringToBoolVector(const juce::String& input);

/**
 * @brief Fills a std::array of std::atomic objects with a specified value.
 *
 * This templated function iterates through an array of atomic types
 * and uses the thread-safe `.store()` method to assign the provided value
 * to each element. This avoids the "explicitly deleted function" error
 * that occurs when using `std::array::fill` on atomic types.
 *
 * @tparam T The type of the value stored in the atomic objects.
 * @tparam Size The compile-time size of the std::array.
 * @param arr The std::array of std::atomic<T> to be filled.
 * @param value The value of type T to store in each element.
 */
template <typename T, size_t Size>
void fillAtomicArray(std::array<std::atomic<T>, Size>& arr, const T& value) {
    for (size_t i = 0; i < Size; ++i) {
        arr[i].store(value);
    }
}

/**
 * @brief Fills an atomic array with a default value and then populates it
 * with values from a vector.
 *
 * This function first fills the entire `std::array` of `std::atomic<T>`
 * with a `fillValue` to ensure all elements are in a valid state. It then
 * iterates through the provided `std::vector<T>` and stores those values
 * in the atomic array, up to the smaller of the two container sizes.
 * This is a thread-safe operation.
 *
 * @tparam T The type of the values in the atomic objects and vector.
 * @tparam Size The compile-time size of the std::array.
 * @param arr The std::array of std::atomic<T> to be filled and populated.
 * @param fillValue The default value used to fill the entire array initially.
 * @param sourceVector The std::vector<T> containing the new values to store.
 */
template <typename T, size_t Size>
void populateAtomicArrayFromVector(
    std::array<std::atomic<T>, Size>& arr,
    const T& fillValue,
    const std::vector<T>& sourceVector)
{
    // Step 1: Fill the entire array with the default value
    fillAtomicArray(arr, fillValue);

    // Step 2: Iterate and populate from the vector
    // We use std::min to prevent out-of-bounds access
    size_t numToCopy = std::min(Size, sourceVector.size());

    for (size_t i = 0; i < numToCopy; ++i) {
        arr[i].store(sourceVector[i]);
    }
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
 * @brief Converts a std::array of floats into a single JUCE string,
 * using only a specified number of elements.
 *
 * This is a templated function that takes a std::array by const reference
 * and an integer count. It iterates up to the lesser of the array's size
 * and the provided count, converting each element to a string.
 *
 * @tparam Size The size of the std::array, known at compile-time.
 * @param array The std::array<float, Size> to convert.
 * @param count The number of elements to use from the array.
 * @return A juce::String containing the space-separated float values.
 */
template <size_t Size>
juce::String arrayToStringLimited(const std::array<float, Size>& array, int count) {
    juce::String s = "";

    // The loop iterates up to the smaller of the array's size and the provided count.
    for (size_t i = 0; i < Size && i < static_cast<size_t>(count); ++i) {
        s += juce::String(array[i]) + " ";
    }
    return s;
}

/**
 * @brief Converts a std::array of atomics of floats into a single JUCE string,
 * using only a specified number of elements.
 *
 * This is a templated function that takes a std::array of std::atomic by const reference
 * and an integer count. It iterates up to the lesser of the array's size
 * and the provided count, safely loading and converting each atomic element to a string.
 *
 * @tparam Size The size of the std::array, known at compile-time.
 * @param array The std::array<std::atomic<float>, Size> to convert.
 * @param count The number of elements to use from the array.
 * @return A juce::String containing the space-separated float values.
 */
template <size_t Size>
juce::String atomicArrayToStringLimited(const std::array<std::atomic<float>, Size>& array, int count) {
    juce::String s = "";

    // The loop iterates up to the smaller of the array's size and the provided count.
    for (size_t i = 0; i < Size && i < static_cast<size_t>(count); ++i) {
        // We use .load() to safely get the value from the atomic
        s += juce::String(array[i].load()) + " ";
    }
    return s;
}

/**
 * @brief Converts a std::array of atomics of booleans into a single JUCE string,
 * using only a specified number of elements.
 *
 * This is a templated function that takes a std::array of std::atomic<bool> by const reference
 * and an integer count. It iterates up to the lesser of the array's size
 * and the provided count, safely loading each atomic element and converting it to a string.
 *
 * @tparam Size The size of the std::array, known at compile-time.
 * @param array The std::array<std::atomic<bool>, Size> to convert.
 * @param count The number of elements to use from the array.
 * @return A juce::String containing the space-separated boolean values as "true" or "false".
 */
template <size_t Size>
juce::String atomicArrayToStringLimited(const std::array<std::atomic<bool>, Size>& array, int count) {
    juce::String s = "";

    // The loop iterates up to the smaller of the array's size and the provided count.
    for (size_t i = 0; i < Size && i < static_cast<size_t>(count); ++i) {
        // We use .load() to safely get the value from the atomic
        s += (array[i].load() ? "true" : "false") + juce::String(" ");
    }
    return s;
}

/**
 * @brief Converts a std::array of atomics of floats to a juce::Array.
 *
 * This templated function takes a std::array of std::atomic<float> by const reference
 * and an integer count. It iterates up to the lesser of the array's size
 * and the provided count, safely loading each atomic element and adding it to a juce::Array.
 *
 * @tparam Size The size of the std::array, known at compile-time.
 * @param array The std::array<std::atomic<float>, Size> to convert.
 * @param count The number of elements to use from the array.
 * @return A juce::Array<float> containing the float values.
 */
template <size_t Size>
juce::Array<float> atomicArrayToJuceArrayLimited(const std::array<std::atomic<float>, Size>& array, int count)
{
    juce::Array<float> result;

    // Use std::min to ensure we don't go out of bounds
    size_t numToCopy = std::min(Size, static_cast<size_t>(count));

    for (size_t i = 0; i < numToCopy; ++i)
    {
        // Use .load() to safely get the value from the atomic and add it to the JUCE array
        result.add(array[i].load());
    }

    return result;
}

/**
 * @brief Converts a std::array of atomics of booleans to a juce::Array<bool>.
 *
 * This templated function takes a std::array of std::atomic<bool> by const reference
 * and an integer count. It iterates up to the lesser of the array's size
 * and the provided count, safely loading each atomic element and adding it to a juce::Array.
 *
 * @tparam Size The size of the std::array, known at compile-time.
 * @param array The std::array<std::atomic<bool>, Size> to convert.
 * @param count The number of elements to use from the array.
 * @return A juce::Array<bool> containing the boolean values.
 */
template <size_t Size>
juce::Array<bool> atomicBoolArrayToJuceArrayLimited(const std::array<std::atomic<bool>, Size>& array, int count)
{
    juce::Array<bool> result;

    // Use std::min to ensure we don't go out of bounds
    size_t numToCopy = std::min(Size, static_cast<size_t>(count));

    for (size_t i = 0; i < numToCopy; ++i)
    {
        // Use .load() to safely get the value from the atomic and add it to the JUCE array
        result.add(array[i].load());
    }

    return result;
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
std::array<float, Size1> multiSliderArraysToFloatArray(const std::array<std::atomic<float>, Size1>& sliderVals, const std::array<std::atomic<bool>, Size2>& activeSliders) {

    std::array<float, Size1> returnArray;

    // counting through the sliderVals and returnArray arrays
    int valueCounter = 0;

    //if a slider is active, store its value in the output array, otherwise ignore
    for (auto& bval : activeSliders)
    {
        if (bval)
        {
            returnArray[valueCounter] = sliderVals[valueCounter].load(); // 1d for now....
            valueCounter++;
        }

        if (valueCounter > Size1)
        {
            return returnArray;
        }
    }

    return returnArray;
}



#endif //ARRAY_TO_STRING_H
