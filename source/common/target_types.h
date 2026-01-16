//
// Created by Dan Trueman on 8/2/25.
//
#pragma once

#ifndef BITKLAVIER0_TARGET_TYPES_H
#define BITKLAVIER0_TARGET_TYPES_H

/*
 * each PreparationType bounded by PreparationTypeFirst and PreparationTypeNil
 */

typedef enum PreparationParameterTargetType {
    BlendronicTargetFirst = 0,
    BlendronicTargetPatternSync,
    BlendronicTargetBeatSync,
    BlendronicTargetClear,
    BlendronicTargetPausePlay,
    BlendronicTargetInput,
    BlendronicTargetOutput,
    BlendronicTargetModReset,
    BlendronicTargetNil,
    SynchronicTargetFirst,
    SynchronicTargetDefault,
    SynchronicTargetPatternSync,
    SynchronicTargetBeatSync,
    SynchronicTargetAddNotes,
    SynchronicTargetClear,
    SynchronicTargetPausePlay,
    //SynchronicTargetDeleteOldest,
    //SynchronicTargetDeleteNewest,
    //SynchronicTargetRotate,
    SynchronicTargetModReset,
    SynchronicTargetNil,
    ResonanceTargetFirst,
    ResonanceTargetDefault,
    ResonanceTargetRing,
    ResonanceTargetAdd,
    ResonanceTargetModReset,
    ResonanceTargetNil,
    NostalgicTargetFirst,
    NostalgicTargetDefault,
    NostalgicTargetClear,
    NostalgicTargetModReset,
    NostalgicTargetNil

} PreparationParameterTargetType;

enum TriggerType {
    _NoteOn = 1 << 0,
    _NoteOff = 1 << 1,
    _Both = 1 << 2,
};


#endif //BITKLAVIER0_TARGET_TYPES_H
