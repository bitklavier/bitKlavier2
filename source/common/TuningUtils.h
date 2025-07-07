//
// Created by Dan Trueman on 7/6/25.
//
#pragma once

#ifndef BITKLAVIER0_TUNINGUTILS_H
#define BITKLAVIER0_TUNINGUTILS_H

#include "utils.h"
#include "open_gl_combo_box.h"
#include "tuning_systems.h"
#include "chowdsp_parameters/chowdsp_parameters.h"

const std::vector<Fundamental> fundamentalValues = {
    Fundamental::C,
    Fundamental::C41D5,
    Fundamental::D,
    Fundamental::D41E5,
    Fundamental::E,
    Fundamental::F,
    Fundamental::F41G5,
    Fundamental::G,
    Fundamental::G41A5,
    Fundamental::A,
    Fundamental::A41B5,
    Fundamental::B,
    Fundamental::none,       // Included
    Fundamental::lowest,     // Included
    Fundamental::highest,    // Included
    Fundamental::last,       // Included
    Fundamental::automatic,  // Included
};

// Create a vector with the enum values you want to iterate over
const std::vector<PitchClass> pitchClassValues = {
    PitchClass::C,
    PitchClass::C41D5,
    PitchClass::D,
    PitchClass::D41E5,
    PitchClass::E,
    PitchClass::F,
    PitchClass::F41G5,
    PitchClass::G,
    PitchClass::G41A5,
    PitchClass::A,
    PitchClass::A41B5,
    PitchClass::B,
};

// Create a corresponding vector of strings
const std::vector<std::string> fundamentalStrings = {
    "C",
    "C#/Db", // Using the correct substitution
    "D",
    "D#/Eb",
    "E",
    "F",
    "F#/Gb",
    "G",
    "G#/Ab",
    "A",
    "A#/Bb",
    "B",
    "none",
    "lowest",
    "highest",
    "last",
    "automatic",
    "FundamentalNil"
};

const std::vector<std::string> pitchClassStrings = {
    "C",
    "C#/Db", // Using the correct substitution
    "D",
    "D#/Eb",
    "E",
    "F",
    "F#/Gb",
    "G",
    "G#/Ab",
    "A",
    "A#/Bb",
    "B",
    // "PitchClassNil" // Excluded
};

int intFromFundamental(Fundamental p);
int intFromPitchClass(PitchClass p);
std::string fundamentalToString(Fundamental value);
std::string pitchClassToString(PitchClass value);
PitchClass getPitchClassFromInt(int bitPosition);
Fundamental getFundamentalFromInt(int bitPosition);
std::array<float, 12> getOffsetsFromTuningSystem (TuningSystem ts);

void setupTuningSystemMenu(std::unique_ptr<OpenGLComboBox> &tuning_combo_box_);
void setOffsetsFromTuningSystem(TuningSystem t, int newFund, std::array<float, 12>& circularTuningVec);

static std::array<float, 12> rotateValuesByFundamental (std::array<float, 12> vals, int fundamental);

#endif //BITKLAVIER0_TUNINGUTILS_H
