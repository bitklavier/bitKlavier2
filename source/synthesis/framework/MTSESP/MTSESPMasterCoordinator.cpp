// Copyright (C) 2022-2026 Dan Trueman
// SPDX-License-Identifier: GPL-3.0-or-later

#include "MTSESPMasterCoordinator.h"

MTSESPMasterCoordinator::MTSESPMasterCoordinator()
{
    updateStatus();
}

MTSESPMasterCoordinator::~MTSESPMasterCoordinator()
{
    // The wrapper's destructor also deregisters, but be explicit: release the
    // MTS-ESP master slot before we tear down.
    wrapper_.deregisterMaster();
}

void MTSESPMasterCoordinator::setSelectedMasterTuningUuid (const juce::String& uuid)
{
    if (uuid.isEmpty())
    {
        clearSelectedMasterTuning();
        return;
    }

    if (selectedUuid_ == uuid)
        return;

    selectedUuid_ = uuid;

    // TODO(Stage E/F): register as master (if possible) and publish the selected
    // Tuning's 128-note table via the coordinator's publish pump.
    updateStatus();
}

void MTSESPMasterCoordinator::clearSelectedMasterTuning()
{
    if (selectedUuid_.isEmpty() && getStatus() == MtsStatus::Off)
        return;

    selectedUuid_.clear();

    // Releasing the slot is harmless if we never claimed it (Stage B never does).
    wrapper_.deregisterMaster();
    updateStatus();
}

bool MTSESPMasterCoordinator::isTuningSelectedAsMaster (const juce::String& uuid) const
{
    return uuid.isNotEmpty() && uuid == selectedUuid_;
}

void MTSESPMasterCoordinator::notifyTuningChanged (const juce::String& uuid)
{
    if (! isTuningSelectedAsMaster (uuid))
        return;

    // TODO(Stage E): mark a republish pending so the publish pump pushes the
    // updated tuning table on its next message-thread tick.
}

void MTSESPMasterCoordinator::notifyTuningDeleted (const juce::String& uuid)
{
    if (isTuningSelectedAsMaster (uuid))
        clearSelectedMasterTuning();
}

void MTSESPMasterCoordinator::loadSelectionFromTree (const juce::ValueTree& /*galleryRoot*/)
{
    // TODO(Stage C): read IDs::mtsMasterTuningUuid from the gallery root tree,
    // validate it resolves to a live Tuning, and adopt it (clearing if stale).
}

void MTSESPMasterCoordinator::updateStatus()
{
    if (! isCompiledIn())
    {
        status_.store (MtsStatus::Disabled, std::memory_order_relaxed);
        return;
    }

    if (selectedUuid_.isEmpty())
    {
        status_.store (MtsStatus::Off, std::memory_order_relaxed);
        return;
    }

    // Stage B: a selection exists but publishing/registration isn't wired yet.
    // Stage F refines this into Publishing / Blocked_OtherMasterExists /
    // LibraryMissing based on the wrapper's registration result.
    status_.store (MtsStatus::Selected_NotPublishing, std::memory_order_relaxed);
}

juce::String MTSESPMasterCoordinator::statusToText (MtsStatus status)
{
    switch (status)
    {
        case MtsStatus::Disabled:                  return "MTS-ESP disabled in this build";
        case MtsStatus::LibraryMissing:            return "MTS-ESP library not installed";
        case MtsStatus::Off:                       return "Off";
        case MtsStatus::Selected_NotPublishing:    return "Selected";
        case MtsStatus::Publishing:                return "Publishing";
        case MtsStatus::Blocked_OtherMasterExists: return "Blocked — another MTS-ESP master is running";
    }
    return {};
}
