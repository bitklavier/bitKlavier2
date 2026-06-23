// Copyright (C) 2022-2026 Dan Trueman
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>
#include <array>
#include <atomic>
#include <functional>

#include "MTSESPMasterWrapper.h"

class TuningProcessor;

/**
 * Transient runtime status of MTS-ESP master publishing. This is NOT saved — it
 * reflects what is actually happening right now, as opposed to the saved
 * *intent* (which Tuning the user picked, stored as a UUID in the gallery).
 */
enum class MtsStatus
{
    Disabled,                   ///< MTS-ESP was compiled out (BITKLAVIER_ENABLE_MTS_ESP=0).
    LibraryMissing,             ///< Compiled in, but the runtime libMTS library isn't installed.
    Off,                        ///< No Tuning selected as master.
    Selected_NotPublishing,     ///< A Tuning is selected but we aren't publishing (yet / not registered).
    Publishing,                 ///< Registered as master and pushing tunings.
    Blocked_OtherMasterExists   ///< Another app/plugin already owns the MTS-ESP master slot.
};

/**
 * MTSESPMasterCoordinator
 *
 * Plugin/SynthBase-level owner of the MTS-ESP master lifecycle and policy. There
 * is exactly one per bitKlavier instance (owned by SynthBase). It holds:
 *   - the *selected* master Tuning UUID (mirror of the saved gallery property),
 *   - the transient runtime status,
 *   - the single MTSESPMasterWrapper that talks to the SDK.
 *
 * Selection is exclusive across the whole gallery: only one Tuning can be the
 * master source, so the state is a single UUID rather than per-Tuning booleans.
 *
 * STAGE B SCOPE: this is the skeleton. Selection + status bookkeeping work, but
 * the coordinator does NOT yet register as master, build tuning tables, or
 * publish — those are wired in later stages (saved state: Stage C; UI: Stage D;
 * publish pump + Timer: Stage E; real registration/status mapping: Stage F).
 *
 * THREADING: all public methods are message-thread only. getStatus() is atomic
 * and safe to read from any thread (e.g. a UI timer). No method touches the
 * audio thread. See docs/MTS-ESP-Master-Spec.md.
 */
class MTSESPMasterCoordinator : private juce::Timer
{
public:
    MTSESPMasterCoordinator();
    ~MTSESPMasterCoordinator() override;

    // ---- selection (mirror of the saved intent) -----------------------------

    /** Select the Tuning with this UUID as the MTS-ESP master source. Empty clears. */
    void setSelectedMasterTuningUuid (const juce::String& uuid);

    /** Clear the selection (no Tuning drives MTS-ESP). */
    void clearSelectedMasterTuning();

    /** The currently selected master Tuning UUID, or empty if none. */
    juce::String getSelectedMasterTuningUuid() const { return selectedUuid_; }

    /** Whether the given Tuning UUID is the currently selected master. */
    bool isTuningSelectedAsMaster (const juce::String& uuid) const;

    /** Whether any Tuning is currently selected as master. */
    bool hasSelection() const { return selectedUuid_.isNotEmpty(); }

    // ---- status (transient runtime) -----------------------------------------

    MtsStatus getStatus() const { return status_.load (std::memory_order_relaxed); }
    juce::String getStatusText() const { return statusToText (getStatus()); }
    static juce::String statusToText (MtsStatus);

    // ---- change / lifecycle notifications (message thread) -------------------

    /** A Tuning's tuning state changed; if it's the selected master, schedule a republish. */
    void notifyTuningChanged (const juce::String& uuid);

    /** A Tuning was deleted; if it's the selected master, clear the selection. */
    void notifyTuningDeleted (const juce::String& uuid);

    // ---- wiring (populated by SynthBase / later stages) ---------------------

    /** Provide a resolver from Tuning UUID to its live processor (used when publishing). */
    void setTuningLookup (std::function<TuningProcessor* (const juce::String&)> fn)
    {
        tuningLookup_ = std::move (fn);
    }

    /**
     * Read the saved selection from the gallery root tree (call after load).
     * Reads IDs::mtsMasterTuningUuid, validates it still resolves to a Tuning
     * preparation in the tree, and adopts it (clearing the selection if stale).
     */
    void loadSelectionFromTree (const juce::ValueTree& galleryRoot);

    /**
     * Write the current selection into the gallery root tree (call before save)
     * so it serializes with the gallery. Removes the property when nothing is
     * selected.
     */
    void syncSelectionToTree (juce::ValueTree& galleryRoot) const;

    /** True iff a Tuning preparation with this UUID exists anywhere in the gallery tree. */
    static bool tuningExistsInTree (const juce::ValueTree& galleryRoot, const juce::String& uuid);

    /** True iff MTS-ESP support was compiled in. */
    static constexpr bool isCompiledIn() noexcept { return MTSESPMasterWrapper::isCompiledIn(); }

private:
    /** Recompute status_ from the current selection and wrapper state. */
    void updateStatus();

    /** Start the publish timer when a selection exists (and MTS is compiled in); stop otherwise. */
    void updateTimerState();

    /** Publish pump (message thread): rebuild + push the selected Tuning's table. */
    void timerCallback() override;

    static constexpr int kPublishIntervalMs = 20; // ~50 Hz; tracks Spring sim smoothly

    MTSESPMasterWrapper wrapper_;
    juce::String selectedUuid_;                       // empty == no selection
    std::atomic<MtsStatus> status_ { MtsStatus::Off };
    std::function<TuningProcessor* (const juce::String&)> tuningLookup_;

    // Publish-pump state (message thread only).
    std::array<double, 128> lastPublished_ {};
    bool hasPublished_ = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MTSESPMasterCoordinator)
};
