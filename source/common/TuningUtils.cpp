//
// Created by Dan Trueman on 7/6/25.
//

#include "TuningUtils.h"

/**
 * since the PitchClass and Fundamental enums are uint_32, we can't just map them to pitch classes
 *      - these two functions are here to help
 *
 * todo: could probably combine these into a single template function
 *          but these are very clear, so maybe better for readability
 * @param Fundamental p
 * @return int corresponding to PitchClass of Fundamental p
 */
int intFromFundamental(Fundamental p) {
    // Cast the enum class value to its underlying integer type
    auto pitchValue = static_cast<std::underlying_type_t<Fundamental>>(p);

    // If the value is 0 (PitchClassNil), return -1 or handle as an error
//    if (pitchValue == 0) {
//        // You can return a special value like -1 to indicate an invalid input
//        return -1;
//    }

    // Since each enumerator is a power of 2 (1 << N), we need to find N.
    // We can do this by counting the number of trailing zeros.
    // For example:
    // 1 (0001 in binary) has 0 trailing zeros -> bit shift 0
    // 2 (0010 in binary) has 1 trailing zero -> bit shift 1
    // 4 (0100 in binary) has 2 trailing zeros -> bit shift 2

    // A generic approach using a loop
    int shift = 0;
    while ((pitchValue & 1) == 0) {
        pitchValue >>= 1;
        shift++;
    }
    return shift;
}

// Function to convert a single PitchClass flag to its bit shift value (0-11)
int intFromPitchClass(PitchClass p) {
    // Cast the enum class value to its underlying integer type
    auto pitchValue = static_cast<std::underlying_type_t<PitchClass>>(p);

//    // If the value is 0 (PitchClassNil), return -1 or handle as an error
//    if (pitchValue == 0) {
//        // You can return a special value like -1 to indicate an invalid input
//        return -1;
//    }

    // Since each enumerator is a power of 2 (1 << N), we need to find N.
    // We can do this by counting the number of trailing zeros.
    // For example:
    // 1 (0001 in binary) has 0 trailing zeros -> bit shift 0
    // 2 (0010 in binary) has 1 trailing zero -> bit shift 1
    // 4 (0100 in binary) has 2 trailing zeros -> bit shift 2

    // A generic approach using a loop
    int shift = 0;
    while ((pitchValue & 1) == 0) {
        pitchValue >>= 1;
        shift++;
    }
    return shift;
}

/**
 * @brief Converts an integer representing a bit position to a PitchClass.
 *
 * This function takes an integer 'bitPosition' and left-shifts 1 by that
 * amount to generate the corresponding PitchClass enum value. It returns
 * std::nullopt if the bit position is outside the valid range (0-11).
 *
 * @param bitPosition An integer representing the bit position (0 for C, 1 for C#/Db, etc.).
 * @return An std::optional<PitchClass> containing the corresponding PitchClass if valid,
 * or std::nullopt if the bitPosition is out of range.
 */
PitchClass getPitchClassFromInt(int bitPosition) {
    if (bitPosition >= 0 && bitPosition <= 11) {
        return static_cast<PitchClass>(1U << bitPosition);
    } else {
        // Handle out-of-range or invalid bit positions
        return PitchClass::PitchClassNil;
    }
}


// Helper function to get the string representation of a Fundamental value
std::string fundamentalToString(Fundamental value) {
    // You'll need a way to map the enum value to its corresponding string.
    // Since you have a fixed order in both vectors, you could potentially
    // find the index of the enum value in fundamentalValues and use that
    // index to get the string from fundamentalStrings.
    // A switch statement is also a very common and clear approach for this mapping.
    switch (value) {
        case Fundamental::C: return "C";
        case Fundamental::C41D5: return "C#/Db";
        case Fundamental::D: return "D";
        case Fundamental::D41E5: return "D#/Eb";
        case Fundamental::E: return "E";
        case Fundamental::F: return "F";
        case Fundamental::F41G5: return "F#/Gb";
        case Fundamental::G: return "G";
        case Fundamental::G41A5: return "G#/Ab";
        case Fundamental::A: return "A";
        case Fundamental::A41B5: return "A#/Bb";
        case Fundamental::B: return "B";
        case Fundamental::none: return "none";
        case Fundamental::lowest: return "lowest";
        case Fundamental::highest: return "highest";
        case Fundamental::last: return "last";
        case Fundamental::automatic: return "automatic";
        case Fundamental::FundamentalNil: return "FundamentalNil";
        default: return "Unknown Fundamental"; // Handle potential unknown values
    }
}

std::string pitchClassToString(PitchClass value) {
    switch (value) {
        case PitchClass::C: return "C";
        case PitchClass::C41D5: return "C#/Db";
        case PitchClass::D: return "D";
        case PitchClass::D41E5: return "D#/Eb";
        case PitchClass::E: return "E";
        case PitchClass::F: return "F";
        case PitchClass::F41G5: return "F#/Gb";
        case PitchClass::G: return "G";
        case PitchClass::G41A5: return "G#/Ab";
        case PitchClass::A: return "A";
        case PitchClass::A41B5: return "A#/Bb";
        case PitchClass::B: return "B";
        case PitchClass::PitchClassNil: return "PitchClassNil"; // Include if you choose to iterate over it
        default: return "Unknown PitchClass";
    }
}

void setupTuningSystemMenu(std::unique_ptr<OpenGLComboBox> &tuning_combo_box_)
{
    // clear the default menu so we can make submenus
    tuning_combo_box_->clear(juce::sendNotificationSync);
    juce::OwnedArray<juce::PopupMenu> submenus;
    submenus.add(new juce::PopupMenu());
    submenus.add(new juce::PopupMenu());
    int i = 0;
    for (auto choice : TuningSystemNames) {
        if (i <= 6)
            tuning_combo_box_->addItem(choice,i+1);
        if (i>6 && i <33) {
            submenus.getUnchecked(0)->addItem(i+1,choice);
        }
        else if (i>=33) {
            if (choice == "TuningSystemNil") continue;
            submenus.getUnchecked(1)->addItem(i+1,choice);
        }
        i++;
    }

    tuning_combo_box_->addSeparator();
    auto* pop_up = tuning_combo_box_->getRootMenu();
    pop_up->addSubMenu("Historical",*submenus.getUnchecked(0));
    pop_up->addSubMenu("Various",*submenus.getUnchecked(1));
}

void setOffsetsFromTuningSystem(const TuningSystem t, const int newFund, std::array<float, 12>& circularTuningVec)
{
    //if (!params.tuningState.setFromAudioThread) { // it's not clear whether this is necessary; see bitKlavierDevNotes

    auto it = std::find_if(tuningMap.begin(), tuningMap.end(),
        [t](const auto& pair) {
            return pair.first == t;
        });
    if (it->first == TuningSystem::Custom) {

    }
    else if (it != tuningMap.end()) {
        const auto& tuning = it->second;
        const auto tuningArray = rotateValuesByFundamental(tuning, newFund);
        int index  = 0;
        for (const auto val :tuningArray) {
            circularTuningVec[index] = val * 100;
            DBG("new tuning " + juce::String(index) + " " + juce::String(circularTuningVec[index]));
            index++;
        }
    }
}

std::array<float, 12> rotateValuesByFundamental (std::array<float, 12> vals, int fundamental)
{
    int offset;
    if (fundamental <= 0)
        offset = 0;
    else
        offset = fundamental;

    std::array<float, 12> new_vals = { 0.f };
    for (int i = 0; i < 12; i++)
    {
        int index = ((i - offset) + 12) % 12;
        new_vals[i] = vals[index];
    }
    return new_vals;
}
