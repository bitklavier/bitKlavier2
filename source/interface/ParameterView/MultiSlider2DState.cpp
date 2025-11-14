//
// Created by Dan Trueman on 11/13/25.
//

#include "MultiSlider2DState.h"

juce::String sliderValsToString(
    const std::array<std::array<std::atomic<float>, MAXMULTISLIDER2DVALS>, MAXMULTISLIDER2DLENGTH>& sliderVals,
    const std::atomic<int>& sliderVals_size,
    const std::array<std::atomic<int>, MAXMULTISLIDER2DLENGTH>& sliderDepths)
{
    juce::String result;

    // Atomically load the number of active rows
    const int N_used = sliderVals_size.load();

    // Loop through the active rows
    for (int i = 0; i < N_used; ++i)
    {
        // Atomically load the depth (number of active columns) for this row
        const int M_used_i = sliderDepths[i].load();

        // Loop through the active values in the current row
        for (int j = 0; j < M_used_i; ++j)
        {
            // Load the atomic float value and append it (6 decimal places)
            result += juce::String(sliderVals[i][j].load(), 6);

            // Add a comma separator, unless it's the last value in this row
            if (j < M_used_i - 1)
                result += ",";
        }

        // Add the " / " row separator to mark the end of the inner array
        result += " / ";
    }

    return result;
}

void stringToSliderVals(
    const juce::String& savedState,
    std::array<std::array<std::atomic<float>, MAXMULTISLIDER2DVALS>, MAXMULTISLIDER2DLENGTH>& sliderVals,
    std::atomic<int>& sliderVals_size,
    std::array<std::atomic<int>, MAXMULTISLIDER2DLENGTH>& sliderDepths,
    float defaultValue)
{
    // A. Use temporary, non-atomic structures for safe parsing
    std::array<std::array<float, MAXMULTISLIDER2DVALS>, MAXMULTISLIDER2DLENGTH> temp_sliderVals;
    std::array<int, MAXMULTISLIDER2DLENGTH> temp_sliderDepths = {};
    int temp_sliderVals_size = 0;

    // --- NEW: Step 0. Initialize the entire temporary array ---
    // This sets all MAXMULTISLIDER2DLENGTH * MAXMULTISLIDER2DVALS elements
    // to the default value. Loaded data will overwrite this.
    for (size_t i = 0; i < MAXMULTISLIDER2DLENGTH; ++i)
    {
        for (size_t j = 0; j < MAXMULTISLIDER2DVALS; ++j)
        {
            temp_sliderVals[i][j] = defaultValue;
        }
    }

    // 1. Split the string into rows using " / " as the delimiter.
    auto rows = juce::StringArray::fromTokens(savedState, " / ", "");

    // Iterate through the rows, respecting the maximum allowed size
    for (int i = 0; i < std::min((int)rows.size(), (int)MAXMULTISLIDER2DLENGTH); ++i)
    {
        // 2. Split each row (token) into values using ','
        auto values = juce::StringArray::fromTokens(rows[i], ",", "");

        // Determine the actual number of values parsed for this row, respecting MAXMULTISLIDER2DVALS
        int M_parsed = std::min((int)values.size(), (int)MAXMULTISLIDER2DVALS);

        // Store the depth for this row
        temp_sliderDepths[i] = M_parsed;

        // Populate the temporary values (overwriting the default)
        for (int j = 0; j < M_parsed; ++j)
        {
            // 3. Convert string token to float
            temp_sliderVals[i][j] = (float)values[j].getFloatValue();
        }

        // Increment the number of active rows only if the row had valid data
        if (M_parsed > 0)
            temp_sliderVals_size++;
    }

    // B. Safely update the atomics (Critical Section)

    // 1. Update the actual slider values
    // We update all slots that had data *or* were set to the default value above.
    // The current implementation is simple: update the whole target area (0 to temp_sliderVals_size)
    // to ensure consistency.
    for (size_t i = 0; i < MAXMULTISLIDER2DLENGTH; ++i)
    {
        for (size_t j = 0; j < MAXMULTISLIDER2DVALS; ++j)
        {
            sliderVals[i][j].store(temp_sliderVals[i][j]);
        }
    }

    // 2. Update the depths (M_used(i))
    // This remains the same as it correctly sets the used depths and zeros out the unused ones.
    for (int i = 0; i < temp_sliderVals_size; ++i)
    {
        sliderDepths[i].store(temp_sliderDepths[i]);
    }
    // Clean up/zero out unused depths for safety
    for (int i = temp_sliderVals_size; i < MAXMULTISLIDER2DLENGTH; ++i)
    {
        sliderDepths[i].store(0);
    }

    // 3. Update the overall size (N_used) LAST.
    sliderVals_size.store(temp_sliderVals_size);
}
