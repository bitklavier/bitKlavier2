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

juce::String getOnKeyString(const std::bitset<128>& bits);

//juce::String arrayToString(const std::array<float, 12>& array);

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

#endif //ARRAY_TO_STRING_H
