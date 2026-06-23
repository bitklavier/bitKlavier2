// Copyright (C) 2022-2026 Dan Trueman
// SPDX-License-Identifier: GPL-3.0-or-later

#include "MTSESPClientWrapper.h"

// Only translation unit that includes the ODDSound client SDK header.
#if BITKLAVIER_ENABLE_MTS_ESP
 #include "libMTSClient.h"
#endif

MTSESPClientWrapper::~MTSESPClientWrapper()
{
    deregisterClient();
}

void MTSESPClientWrapper::registerClient()
{
   #if BITKLAVIER_ENABLE_MTS_ESP
    if (client_.load (std::memory_order_acquire) != nullptr)
        return; // already registered
    MTSClient* c = MTS_RegisterClient();
    client_.store (c, std::memory_order_release);
   #endif
}

void MTSESPClientWrapper::deregisterClient()
{
   #if BITKLAVIER_ENABLE_MTS_ESP
    // Take the handle atomically so the audio thread sees null before we free.
    // The caller must ensure the audio thread isn't mid-query (audio paused).
    if (auto* c = static_cast<MTSClient*> (client_.exchange (nullptr, std::memory_order_acq_rel)))
        MTS_DeregisterClient (c);
   #endif
}

double MTSESPClientWrapper::noteToFrequency (int midiNote, int midiChannel) const
{
   #if BITKLAVIER_ENABLE_MTS_ESP
    if (auto* c = static_cast<MTSClient*> (client_.load (std::memory_order_acquire)))
        return MTS_NoteToFrequency (c, (char) midiNote, (char) midiChannel);
   #endif
    return 440.0 * std::pow (2.0, (midiNote - 69) / 12.0); // 12-TET fallback
}

bool MTSESPClientWrapper::hasMaster() const
{
   #if BITKLAVIER_ENABLE_MTS_ESP
    if (auto* c = static_cast<MTSClient*> (client_.load (std::memory_order_acquire)))
        return MTS_HasMaster (c);
   #endif
    return false;
}

bool MTSESPClientWrapper::shouldFilterNote (int midiNote, int midiChannel) const
{
   #if BITKLAVIER_ENABLE_MTS_ESP
    if (auto* c = static_cast<MTSClient*> (client_.load (std::memory_order_acquire)))
        return MTS_ShouldFilterNote (c, (char) midiNote, (char) midiChannel);
   #endif
    return false;
}
