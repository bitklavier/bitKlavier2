// Copyright (C) 2022-2025 Dan Trueman
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once
#include <atomic>

// Interface implemented by all sound-making processors that support mute/solo.
// SynthBase::coordinateSoloChanged / coordinateMuteChanged iterate the active
// PreparationList, dynamic_cast each wrapper->proc to IMuteSolable*, and call
// updateEffectiveMute() after adjusting the solo/mute fields.
class IMuteSolable
{
public:
    virtual ~IMuteSolable() = default;

    // effective mute (read by audio thread — already exists as muted_ in each params struct)
    virtual std::atomic<bool>& getMuted() = 0;
    // set by the user pressing the M button
    virtual std::atomic<bool>& getUserMuted() = 0;
    // set by the user pressing the S button
    virtual std::atomic<bool>& getSoloed() = 0;
    // set by the coordinator when another prep is soloed
    virtual std::atomic<bool>& getSoloMuted() = 0;

    // recomputes getMuted() = getUserMuted() || getSoloMuted()
    void updateEffectiveMute()
    {
        getMuted().store (getUserMuted().load (std::memory_order_relaxed)
                              || getSoloMuted().load (std::memory_order_relaxed),
                          std::memory_order_relaxed);
    }
};