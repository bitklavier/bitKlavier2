//
// Created by Dan Trueman on 7/6/25.
//

#include "TuningUtils.h"

int intFromFundamental(Fundamental p) {
    // Cast the enum class value to its underlying integer type
    auto pitchValue = static_cast<std::underlying_type_t<Fundamental>>(p);

    // If the value is 0 (PitchClassNil), return -1 or handle as an error
    if (pitchValue == 0) {
        // You can return a special value like -1 to indicate an invalid input
        return -1;
    }

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

    // If the value is 0 (PitchClassNil), return -1 or handle as an error
    if (pitchValue == 0) {
        // You can return a special value like -1 to indicate an invalid input
        return -1;
    }

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

std::string fundamentalToString(Fundamental value) {
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
        default: return "Unknown";
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
        default: return "Unknown";
    }
}