//
// Created by Dan Trueman on 8/2/25.
//
#pragma once

#ifndef BITKLAVIER0_TARGET_TYPES_H
#define BITKLAVIER0_TARGET_TYPES_H

/**
 * todo: rename this PreparationTargetType
 *          and put ALL the prep targets in here
 *          will have to subtract the value of the first target
 *          for each prep type to get the correct channel (+1)
 *          so, outchannel = PreparationTargetType::myPrepTarget - myPrepFirstTarget + 1
 *          so, outchannel = BlendronicTargetClear - BlendronicTargetNormal + 1;
 *          or some such
 */
typedef enum PreparationParameterTargetType {
    BlendronicTargetNormal = 1,
    BlendronicTargetPatternSync,
    BlendronicTargetBeatSync,
    BlendronicTargetClear,
    BlendronicTargetPausePlay,
    BlendronicTargetInput,
    BlendronicTargetOutput,
    BlendronicTargetNil
} PreparationParameterTargetType;

enum TriggerType {
    _NoteOn = 1 << 0,
    _NoteOff = 1 << 1,
    _Both = 1 << 2,
};


#endif //BITKLAVIER0_TARGET_TYPES_H
