// Copyright (C) 2022-2026 Dan Trueman
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <atomic>
#include <cmath>

/**
 * MTSESPClientWrapper
 *
 * Thin boundary around the ODDSound MTS-ESP *client* SDK, used by Tuning
 * preparations in MTS-ESP "receive" mode (TuningType::MTS_Client). Each such
 * Tuning owns one wrapper, which holds an opaque MTSClient* handle.
 *
 * The implementation (MTSESPClientWrapper.cpp) is the only translation unit that
 * includes libMTSClient.h; the handle is stored here as an opaque void* so this
 * header stays free of the SDK (and of JUCE).
 *
 * THREADING:
 *  - registerClient() / deregisterClient() allocate / touch IPC and must be
 *    called from the MESSAGE thread. deregisterClient() additionally requires
 *    that the audio thread is NOT mid-query (e.g. processing paused), because it
 *    frees the handle.
 *  - noteToFrequency() / hasMaster() / shouldFilterNote() are lock-free
 *    shared-memory reads and are SAFE on the audio thread. They are exactly what
 *    the MTS-ESP client API is designed to be polled with while sound plays.
 *
 * The handle is an std::atomic<void*>: written on the message thread, read on the
 * audio thread. Since the handle is only freed at destruction (with audio
 * paused), the audio thread never sees a freed pointer.
 */
class MTSESPClientWrapper
{
public:
    MTSESPClientWrapper() = default;
    ~MTSESPClientWrapper();

    MTSESPClientWrapper (const MTSESPClientWrapper&) = delete;
    MTSESPClientWrapper& operator= (const MTSESPClientWrapper&) = delete;

    /** True iff MTS-ESP support was compiled in (BITKLAVIER_ENABLE_MTS_ESP). */
    static constexpr bool isCompiledIn() noexcept
    {
       #if BITKLAVIER_ENABLE_MTS_ESP
        return true;
       #else
        return false;
       #endif
    }

    /** Register this client with MTS-ESP (idempotent). Message thread only. */
    void registerClient();

    /** Release the client handle. Message thread only; audio must not be querying. */
    void deregisterClient();

    bool isRegistered() const noexcept { return client_.load (std::memory_order_acquire) != nullptr; }

    /**
     * Frequency (Hz) for a MIDI note from the connected MTS-ESP master.
     * Audio-thread safe. Falls back to 12-TET (A4=440) when not registered; when
     * registered with no master connected, the SDK itself returns its local
     * fallback table (12-TET by default).
     */
    double noteToFrequency (int midiNote, int midiChannel = -1) const;

    /** Whether an MTS-ESP master is currently connected. Audio-thread safe. */
    bool hasMaster() const;

    /** Whether the master asked clients to ignore this note. Audio-thread safe. */
    bool shouldFilterNote (int midiNote, int midiChannel = -1) const;

private:
    std::atomic<void*> client_ { nullptr }; // opaque MTSClient*
};
