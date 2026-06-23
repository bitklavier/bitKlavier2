// Copyright (C) 2022-2026 Dan Trueman
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <juce_core/juce_core.h>

/**
 * MTSESPMasterWrapper
 *
 * Thin boundary around the ODDSound MTS-ESP *master* SDK. This class is the only
 * place in bitKlavier that is allowed to touch the SDK: the implementation
 * (MTSESPMasterWrapper.cpp) is the single translation unit that includes
 * libMTSMaster.h. Everything else (the coordinator, the Tuning UI) talks to
 * MTS-ESP through this interface.
 *
 * The whole feature is gated at compile time by BITKLAVIER_ENABLE_MTS_ESP (see
 * CMakeLists.txt). When the feature is disabled, every method is a no-op and
 * isCompiledIn() returns false, so the rest of the code compiles and links
 * unchanged.
 *
 * THREADING: all methods must be called from the message thread. The MTS-ESP
 * master API may allocate / lock / touch shared memory and must never be called
 * from the audio thread. See docs/MTS-ESP-Master-Spec.md.
 */
class MTSESPMasterWrapper
{
public:
    MTSESPMasterWrapper() = default;
    ~MTSESPMasterWrapper();

    /** True iff MTS-ESP support was compiled in (BITKLAVIER_ENABLE_MTS_ESP). */
    static constexpr bool isCompiledIn() noexcept
    {
       #if BITKLAVIER_ENABLE_MTS_ESP
        return true;
       #else
        return false;
       #endif
    }

    /** MTS_CanRegisterMaster(): false if another master already owns the slot. */
    bool canRegisterMaster();

    /**
     * Attempt to claim the MTS-ESP master slot. Returns true on success.
     *
     * Because MTS_RegisterMaster() has no return value (and is a silent no-op
     * when the runtime libMTS library isn't installed), success is inferred by
     * re-checking MTS_CanRegisterMaster() afterwards: if we can no longer
     * register, we took the slot. Idempotent — returns true if already held.
     */
    bool registerMaster();

    /** MTS_DeregisterMaster(): release the master slot if we hold it. */
    void deregisterMaster();

    /** MTS_HasIPC(): whether the process is using IPC to share MTS-ESP data. */
    bool hasIPC();

    /**
     * MTS_Reinitialize(): reset the MTS-ESP library to its default state.
     * Only valid after a host crash left stale IPC shared memory; callers must
     * gate this on (!canRegisterMaster() && hasIPC()).
     */
    void reinitialize();

    /** MTS_SetNoteTunings(): push all 128 MIDI-note frequencies (Hz). No-op unless registered. */
    void setNoteTunings (const double freqs[128]);

    /** MTS_SetScaleName(): set a display name for clients. No-op unless registered. */
    void setScaleName (const char* name);

    /** MTS_GetNumClients(): number of connected MTS-ESP clients. */
    int getNumClients() const;

    /** Whether this wrapper currently believes it holds the master registration. */
    bool isMasterRegistered() const noexcept { return registered_; }

private:
    bool registered_ = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MTSESPMasterWrapper)
};
