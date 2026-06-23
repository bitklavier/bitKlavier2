// Copyright (C) 2022-2026 Dan Trueman
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Stage C unit tests for MTS-ESP master selection persistence:
// save/load round-trip, stale-selection handling, and deletion behaviour.
// These exercise pure ValueTree logic and do not register an MTS-ESP master.

#include "helpers/test_helpers.h"
#include <catch2/catch_test_macros.hpp>

#include "MTSESPMasterCoordinator.h"
#include "Identifiers.h"

namespace
{
    // Build GALLERY -> PIANO -> PREPARATIONS -> (one tuning prep per uuid).
    juce::ValueTree makeGallery (const juce::StringArray& tuningUuids)
    {
        juce::ValueTree gallery (IDs::GALLERY);
        juce::ValueTree piano (IDs::PIANO);
        juce::ValueTree preps (IDs::PREPARATIONS);

        for (const auto& u : tuningUuids)
        {
            juce::ValueTree tuning (IDs::tuning);
            tuning.setProperty (IDs::uuid, u, nullptr);
            preps.appendChild (tuning, nullptr);
        }

        piano.appendChild (preps, nullptr);
        gallery.appendChild (piano, nullptr);
        return gallery;
    }

    const juce::String kUuidA = "aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa";
    const juce::String kUuidB = "bbbbbbbb-bbbb-bbbb-bbbb-bbbbbbbbbbbb";
}

TEST_CASE ("MTS-ESP coordinator: default state", "[mtsesp]")
{
    MTSESPMasterCoordinator coord;
    CHECK (! coord.hasSelection());
    CHECK (coord.getSelectedMasterTuningUuid().isEmpty());

    if (MTSESPMasterCoordinator::isCompiledIn())
        CHECK (coord.getStatus() == MtsStatus::Off);
    else
        CHECK (coord.getStatus() == MtsStatus::Disabled);
}

TEST_CASE ("MTS-ESP coordinator: exclusive selection", "[mtsesp]")
{
    MTSESPMasterCoordinator coord;

    coord.setSelectedMasterTuningUuid (kUuidA);
    CHECK (coord.hasSelection());
    CHECK (coord.getSelectedMasterTuningUuid() == kUuidA);
    CHECK (coord.isTuningSelectedAsMaster (kUuidA));
    CHECK (! coord.isTuningSelectedAsMaster (kUuidB));

    // Selecting another Tuning replaces the previous one (single-UUID model).
    coord.setSelectedMasterTuningUuid (kUuidB);
    CHECK (coord.isTuningSelectedAsMaster (kUuidB));
    CHECK (! coord.isTuningSelectedAsMaster (kUuidA));

    // Selecting empty clears.
    coord.setSelectedMasterTuningUuid ({});
    CHECK (! coord.hasSelection());
}

TEST_CASE ("MTS-ESP coordinator: tuningExistsInTree", "[mtsesp]")
{
    auto gallery = makeGallery ({ kUuidA });
    CHECK (MTSESPMasterCoordinator::tuningExistsInTree (gallery, kUuidA));
    CHECK (! MTSESPMasterCoordinator::tuningExistsInTree (gallery, kUuidB));
    CHECK (! MTSESPMasterCoordinator::tuningExistsInTree (gallery, {}));
    CHECK (! MTSESPMasterCoordinator::tuningExistsInTree ({}, kUuidA));
}

TEST_CASE ("MTS-ESP coordinator: save/load round-trip", "[mtsesp]")
{
    auto gallery = makeGallery ({ kUuidA, kUuidB });

    MTSESPMasterCoordinator saver;
    saver.setSelectedMasterTuningUuid (kUuidB);
    saver.syncSelectionToTree (gallery);
    CHECK (gallery.getProperty (IDs::mtsMasterTuningUuid).toString() == kUuidB);

    // A fresh coordinator adopts the saved selection on load.
    MTSESPMasterCoordinator loader;
    loader.loadSelectionFromTree (gallery);
    CHECK (loader.getSelectedMasterTuningUuid() == kUuidB);

    // Clearing removes the property.
    saver.clearSelectedMasterTuning();
    saver.syncSelectionToTree (gallery);
    CHECK (! gallery.hasProperty (IDs::mtsMasterTuningUuid));
}

TEST_CASE ("MTS-ESP coordinator: stale selection cleared on load", "[mtsesp]")
{
    // Gallery only contains kUuidA, but the saved selection points to kUuidB.
    auto gallery = makeGallery ({ kUuidA });
    gallery.setProperty (IDs::mtsMasterTuningUuid, kUuidB, nullptr);

    MTSESPMasterCoordinator coord;
    coord.loadSelectionFromTree (gallery);

    CHECK (! coord.hasSelection());
    if (MTSESPMasterCoordinator::isCompiledIn())
        CHECK (coord.getStatus() == MtsStatus::Off);
}

TEST_CASE ("MTS-ESP coordinator: deletion clears matching selection", "[mtsesp]")
{
    MTSESPMasterCoordinator coord;
    coord.setSelectedMasterTuningUuid (kUuidA);

    // Deleting a different prep leaves the selection intact.
    coord.notifyTuningDeleted (kUuidB);
    CHECK (coord.isTuningSelectedAsMaster (kUuidA));

    // Deleting the selected Tuning clears it.
    coord.notifyTuningDeleted (kUuidA);
    CHECK (! coord.hasSelection());
}
