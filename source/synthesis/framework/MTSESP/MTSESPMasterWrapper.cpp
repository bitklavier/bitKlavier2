// Copyright (C) 2022-2026 Dan Trueman
// SPDX-License-Identifier: GPL-3.0-or-later

#include "MTSESPMasterWrapper.h"

// This is the ONLY translation unit that includes the ODDSound SDK header.
#if BITKLAVIER_ENABLE_MTS_ESP
 #include "libMTSMaster.h"
#endif

MTSESPMasterWrapper::~MTSESPMasterWrapper()
{
    // Never leave the master slot claimed when the wrapper goes away.
    deregisterMaster();
}

bool MTSESPMasterWrapper::canRegisterMaster()
{
   #if BITKLAVIER_ENABLE_MTS_ESP
    return MTS_CanRegisterMaster();
   #else
    return false;
   #endif
}

bool MTSESPMasterWrapper::registerMaster()
{
   #if BITKLAVIER_ENABLE_MTS_ESP
    if (registered_)
        return true;

    if (! MTS_CanRegisterMaster())
        return false;

    MTS_RegisterMaster();

    // MTS_RegisterMaster() returns void and is a silent no-op when libMTS isn't
    // installed. Infer success: if we can no longer register, we hold the slot.
    registered_ = ! MTS_CanRegisterMaster();
    return registered_;
   #else
    return false;
   #endif
}

void MTSESPMasterWrapper::deregisterMaster()
{
   #if BITKLAVIER_ENABLE_MTS_ESP
    if (registered_)
    {
        MTS_DeregisterMaster();
        registered_ = false;
    }
   #endif
}

bool MTSESPMasterWrapper::hasIPC()
{
   #if BITKLAVIER_ENABLE_MTS_ESP
    return MTS_HasIPC();
   #else
    return false;
   #endif
}

void MTSESPMasterWrapper::reinitialize()
{
   #if BITKLAVIER_ENABLE_MTS_ESP
    MTS_Reinitialize();
   #endif
}

void MTSESPMasterWrapper::setNoteTunings (const double freqs[128])
{
   #if BITKLAVIER_ENABLE_MTS_ESP
    if (registered_)
        MTS_SetNoteTunings (freqs);
   #else
    juce::ignoreUnused (freqs);
   #endif
}

void MTSESPMasterWrapper::setScaleName (const char* name)
{
   #if BITKLAVIER_ENABLE_MTS_ESP
    if (registered_)
        MTS_SetScaleName (name);
   #else
    juce::ignoreUnused (name);
   #endif
}

int MTSESPMasterWrapper::getNumClients() const
{
   #if BITKLAVIER_ENABLE_MTS_ESP
    return MTS_GetNumClients();
   #else
    return 0;
   #endif
}
