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

#endif //BITKLAVIER0_TARGET_TYPES_H
