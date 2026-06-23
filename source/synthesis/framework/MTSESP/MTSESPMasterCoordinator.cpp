// Copyright (C) 2022-2026 Dan Trueman
// SPDX-License-Identifier: GPL-3.0-or-later

#include "MTSESPMasterCoordinator.h"
#include "Identifiers.h"
#include "TuningProcessor.h"

MTSESPMasterCoordinator::MTSESPMasterCoordinator()
{
    updateStatus();
}

MTSESPMasterCoordinator::~MTSESPMasterCoordinator()
{
    stopTimer();
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
    hasPublished_ = false; // force an immediate publish on the next tick

    // TODO(Stage F): register as MTS-ESP master here and map the result into
    // status_ (Publishing / Blocked / LibraryMissing). For now the publish pump
    // builds + pushes the table, but wrapper_.setNoteTunings() no-ops until
    // registration succeeds.
    updateStatus();
    updateTimerState();
}

void MTSESPMasterCoordinator::clearSelectedMasterTuning()
{
    if (selectedUuid_.isEmpty() && getStatus() == MtsStatus::Off)
        return;

    selectedUuid_.clear();
    hasPublished_ = false;

    // Releasing the slot is harmless if we never claimed it.
    wrapper_.deregisterMaster();
    updateStatus();
    updateTimerState();
}

bool MTSESPMasterCoordinator::isTuningSelectedAsMaster (const juce::String& uuid) const
{
    return uuid.isNotEmpty() && uuid == selectedUuid_;
}

void MTSESPMasterCoordinator::notifyTuningChanged (const juce::String& uuid)
{
    if (! isTuningSelectedAsMaster (uuid))
        return;

    // Force the publish pump to re-push on its next tick. (For dynamic tuning
    // types the pump publishes every tick anyway; this matters for static/scala
    // types, where the pump otherwise only publishes on detected content change.)
    hasPublished_ = false;
}

void MTSESPMasterCoordinator::notifyTuningDeleted (const juce::String& uuid)
{
    if (isTuningSelectedAsMaster (uuid))
        clearSelectedMasterTuning();
}

bool MTSESPMasterCoordinator::tuningExistsInTree (const juce::ValueTree& galleryRoot,
                                                  const juce::String& uuid)
{
    if (uuid.isEmpty() || ! galleryRoot.isValid())
        return false;

    // A Tuning may live in any piano's PREPARATIONS list.
    for (int p = 0; p < galleryRoot.getNumChildren(); ++p)
    {
        auto piano = galleryRoot.getChild (p);
        if (! piano.hasType (IDs::PIANO))
            continue;

        auto preps = piano.getChildWithName (IDs::PREPARATIONS);
        for (int i = 0; i < preps.getNumChildren(); ++i)
        {
            auto prep = preps.getChild (i);
            if (prep.hasType (IDs::tuning)
                && prep.getProperty (IDs::uuid).toString() == uuid)
                return true;
        }
    }
    return false;
}

void MTSESPMasterCoordinator::loadSelectionFromTree (const juce::ValueTree& galleryRoot)
{
    const juce::String saved = galleryRoot.getProperty (IDs::mtsMasterTuningUuid).toString();

    // Adopt the saved selection only if it still resolves to a Tuning in the
    // gallery; otherwise treat it as stale and clear it.
    if (saved.isNotEmpty() && tuningExistsInTree (galleryRoot, saved))
        selectedUuid_ = saved;
    else
        selectedUuid_.clear();

    hasPublished_ = false;

    if (selectedUuid_.isEmpty())
        wrapper_.deregisterMaster(); // harmless if never registered

    // TODO(Stage F): if a valid selection was adopted, attempt registration +
    // initial publish here.
    updateStatus();
    updateTimerState();
}

void MTSESPMasterCoordinator::syncSelectionToTree (juce::ValueTree& galleryRoot) const
{
    if (! galleryRoot.isValid())
        return;

    if (selectedUuid_.isNotEmpty())
        galleryRoot.setProperty (IDs::mtsMasterTuningUuid, selectedUuid_, nullptr);
    else
        galleryRoot.removeProperty (IDs::mtsMasterTuningUuid, nullptr);
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

    // A selection exists. The publish pump runs, but until Stage F wires real
    // registration the wrapper no-ops, so status stays "Selected". Stage F
    // refines this into Publishing / Blocked_OtherMasterExists / LibraryMissing.
    status_.store (MtsStatus::Selected_NotPublishing, std::memory_order_relaxed);
}

void MTSESPMasterCoordinator::updateTimerState()
{
    const bool shouldRun = isCompiledIn() && selectedUuid_.isNotEmpty();

    if (shouldRun && ! isTimerRunning())
        startTimer (kPublishIntervalMs);
    else if (! shouldRun && isTimerRunning())
        stopTimer();
}

void MTSESPMasterCoordinator::timerCallback()
{
    if (selectedUuid_.isEmpty() || ! tuningLookup_)
        return;

    // Resolve the selected Tuning fresh each tick — no cached raw pointer, so a
    // deleted/reloaded processor can never dangle here.
    auto* proc = tuningLookup_ (selectedUuid_);
    if (proc == nullptr)
        return; // e.g. graph not fully built yet after a load; try again next tick

    const bool dynamic = proc->isDynamicTuningType();

    double table[128];
    proc->fillTuningTable (table); // side-effect free; safe off the audio thread

    bool changed = ! hasPublished_;
    if (! changed)
    {
        for (int i = 0; i < 128; ++i)
            if (table[i] != lastPublished_[i]) { changed = true; break; }
    }

    if (dynamic || changed)
    {
        // No-op until Stage F registers us as master; harmless to call regardless.
        wrapper_.setNoteTunings (table);
        std::copy (table, table + 128, lastPublished_.begin());
        hasPublished_ = true;
    }
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
