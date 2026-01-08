//
// Created by Davis Polito on 11/8/23.
//

#ifndef BITKLAVIER2_COMMON_H
#define BITKLAVIER2_COMMON_H
/* Copyright 2013-2019 Matt Tytel
 *
 * vital is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * vital is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with vital.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once


// Debugging.
#if DEBUG
    #include <cassert>
    #define _ASSERT(x) assert(x)
#else
    #define _ASSERT(x) ((void)0)
#endif // DEBUG

#define UNUSED(x) ((void)x)

#if !defined(force_inline)
    #if defined (_MSC_VER)
        #define force_inline __forceinline
        #define vector_call __vectorcall
    #else
        #define force_inline inline __attribute__((always_inline))
        #define vector_call
    #endif
#endif

#include "melatonin_audio_sparklines/melatonin_audio_sparklines.h"
namespace bitklavier {


    constexpr float kPi = 3.1415926535897932384626433832795f;
    constexpr float kSqrt2 = 1.414213562373095048801688724209698f;
    constexpr float kEpsilon = 1e-16f;
    constexpr int kMaxBufferSize = 128;
    constexpr int kMaxOversample = 8;
    constexpr int kDefaultSampleRate = 44100;
    constexpr float kMinNyquistMult = 0.45351473923f;
    constexpr int kMaxSampleRate = 192000;
    constexpr int kMidiSize = 128;
    constexpr int kMidiTrackCenter = 60;

    constexpr float kMidi0Frequency = 8.1757989156f;
    constexpr float kDbfsIncrease = 6.0f;
    constexpr int kDegreesPerCycle = 360;
    constexpr int kMsPerSec = 1000;
    constexpr int kNotesPerOctave = 12;
    constexpr int kCentsPerNote = 100;
    constexpr int kCentsPerOctave = kNotesPerOctave * kCentsPerNote;

    constexpr int kPpq = 960; // Pulses per quarter note.
    constexpr float kVoiceKillTime = 0.05f;
    constexpr int kNumChannels = 2;
    constexpr int kNumMidiChannels = 16;
    constexpr int kFirstMidiChannel = 0;
    constexpr int kLastMidiChannel = kNumMidiChannels - 1;
    constexpr int kMaxModulationConnections = 50;
    constexpr int kMaxStateConnections = 50;


    enum VoiceEvent {
        kInvalid,
        kVoiceIdle,
        kVoiceOn,
        kVoiceHold,
        kVoiceDecay,
        kVoiceOff,
        kVoiceKill,
        kNumVoiceEvents
    };

    // order should match preparationIDs in Identifiers.h and AllPrepInfo below
    typedef enum BKPreparationType {
        PreparationTypeKeymap = 0,
        PreparationTypeDirect,
        PreparationTypeSynchronic,
        PreparationTypeNostalgic,
        PreparationTypeBlendronic,
        PreparationTypeResonance,
        PreparationTypeTuning,
        PreparationTypeTempo,
        PreparationTypeMidiFilter,
        PreparationTypeMidiTarget,
        PreparationTypeModulation,
        PreparationTypeReset,
        PreparationTypePianoMap,
        PreparationTypeComment,
        PreparationTypeCompressor,
        PreparationTypeEQ,
        PreparationTypeVST,
        BKPreparationTypeNil,
    } BKPreparationType;

    struct PreparationTypeInfo {
        BKPreparationType type;
        std::string name;
    };

    static const std::vector<PreparationTypeInfo> AllPrepInfo = {
        {PreparationTypeKeymap,     "Keymap"},
        {PreparationTypeDirect,     "Direct"},
        {PreparationTypeSynchronic, "Synchronic"},
        {PreparationTypeNostalgic,  "Nostalgic"},
        {PreparationTypeBlendronic, "Blendrónic"}, // unless special character 'ó' is an issue
        {PreparationTypeResonance,  "Resonance"},
        {PreparationTypeTuning,     "Tuning"},
        {PreparationTypeTempo,      "Tempo"},
        {PreparationTypeMidiFilter, "MIDI Filter"},
        {PreparationTypeMidiTarget, "MIDI Target"},
        {PreparationTypeModulation, "Modification"},
        {PreparationTypeReset,      "Reset"},
        {PreparationTypePianoMap,   "Piano Switch"},
        {PreparationTypeComment,    "Comment"},
        {PreparationTypeCompressor, "Compressor"},
        {PreparationTypeEQ,         "EQ"},
        {PreparationTypeVST,        "VST"}
    };

} // namespace bitKlavier


enum class Direction
{
    forward,
    backward
};

enum class LoopMode
{
    none,
    forward,
    pingpong
};



#endif //BITKLAVIER2_COMMON_H
