//
// Created by Dan Trueman on 8/2/25.
//

#ifndef BITKLAVIER0_TARGET_TYPES_H
#define BITKLAVIER0_TARGET_TYPES_H

typedef enum BlendronicTargetType {
    BlendronicTargetNormal = 1,
    BlendronicTargetPatternSync,
    BlendronicTargetBeatSync,
    BlendronicTargetClear,
    BlendronicTargetPausePlay,
    BlendronicTargetNil
} BlendronicTargetType;

enum TriggerType {
    _NoteOn = 1 << 0,
    _NoteOff = 1 << 1,
    _Both = 1 << 2,
};


#endif //BITKLAVIER0_TARGET_TYPES_H
