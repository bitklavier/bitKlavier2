// Copyright (C) 2022-2026 Dan Trueman
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Unit tests for the MTS-ESP client wrapper's fallback behaviour. These do NOT
// register a client (which would touch the system-global MTS-ESP state); they
// only exercise the unregistered 12-TET fallback path, which is what a Tuning in
// MTS_Client mode produces when no master is connected.

#include "helpers/test_helpers.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "MTSESPClientWrapper.h"

TEST_CASE ("MTS-ESP client wrapper: unregistered 12-TET fallback", "[mtsesp]")
{
    MTSESPClientWrapper client;

    CHECK (! client.isRegistered());
    CHECK (! client.hasMaster());
    CHECK (! client.shouldFilterNote (60));

    using Catch::Matchers::WithinRel;
    CHECK_THAT (client.noteToFrequency (69), WithinRel (440.0, 1e-9));  // A4
    CHECK_THAT (client.noteToFrequency (57), WithinRel (220.0, 1e-9));  // A3
    CHECK_THAT (client.noteToFrequency (81), WithinRel (880.0, 1e-9));  // A5
    CHECK_THAT (client.noteToFrequency (60), WithinRel (261.625565, 1e-4)); // middle C
}
