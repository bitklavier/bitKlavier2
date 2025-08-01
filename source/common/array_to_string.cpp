//
// Created by Davis Polito on 6/3/25.
//
#include "array_to_string.h"
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

std::vector<bool> parseStringToBoolVector(const juce::String& input)
{
    std::vector<bool> result;
    std::string s = input.toStdString();
    std::stringstream ss(s);

    bool value;

    // Use std::boolalpha to tell the stream to parse "true" and "false"
    ss >> std::boolalpha;

    while (ss >> value)
    {
        result.push_back(value);
    }

    return result;
}

/**
 * @brief Creates a JUCE string from the first value of each subarray.
 *
 * This function takes a nested JUCE Array and returns a single JUCE String
 * containing the first float value from each inner array, separated by a space.
 * It iterates through the main array and safely accesses the first element
 * of each sub-array, handling cases where a sub-array might be empty.
 *
 * @param values The juce::Array<juce::Array<float>> to process.
 * @return A juce::String containing the space-separated float values.
 */
juce::String getFirstValueFromSubarrays(const juce::Array<juce::Array<float>>& values)
{
    juce::String result;

    for (int i = 0; i < values.size(); ++i)
    {
        // Get the current sub-array
        const juce::Array<float>& subArray = values.getReference(i);

        // Check if the sub-array is not empty before trying to access the first element.
        if (subArray.size() > 0)
        {
            // Append the first value of the sub-array to the result string.
            // The juce::String constructor can take a float directly.
            result += juce::String(subArray[0]);

            // Add a space after each value, except for the last one.
            if (i < values.size() - 1)
            {
                result += " ";
            }
        }
    }

    return result;
}

/**
 * @brief Converts a juce::Array of booleans to a single JUCE string.
 *
 * This function takes an array of boolean values and returns a single JUCE String
 * containing the "true" or "false" representation of each element, separated by a space.
 *
 * @param states The juce::Array<bool> to convert.
 * @return A juce::String containing the space-separated boolean values as "true" or "false".
 */
juce::String arrayBoolToString(const juce::Array<bool>& states)
{
    juce::String result;

    for (int i = 0; i < states.size(); ++i)
    {
        // Use a ternary operator to convert bool to a string literal
        result += (states[i] ? "true" : "false");

        // Add a space after each value, except for the last one.
        if (i < states.size() - 1)
        {
            result += " ";
        }
    }

    return result;
}
