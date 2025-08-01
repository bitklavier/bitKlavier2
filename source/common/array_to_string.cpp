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