//
// Created by Dan Trueman on 7/6/25.
//
#pragma once

#ifndef BITKLAVIER0_TUNINGUTILS_H
#define BITKLAVIER0_TUNINGUTILS_H

#include "utils.h"

int intFromFundamental(Fundamental p);
int intFromPitchClass(PitchClass p);
std::string fundamentalToString(Fundamental value);
std::string pitchClassToString(PitchClass value);

#endif //BITKLAVIER0_TUNINGUTILS_H
