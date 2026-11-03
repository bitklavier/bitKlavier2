// Copyright (C) 2022-2025 Dan Trueman
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <juce_core/juce_core.h>

namespace bitklavier
{
    // The user's Documents folder. Uses JUCE's userDocumentsDirectory (not "<home>/Documents"),
    // so on Windows it follows a OneDrive-redirected Documents folder; on macOS it is ~/Documents.
    inline juce::File getUserDocumentsDir()
    {
        return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory);
    }

    // <Documents>/bitKlavier/galleries
    inline juce::File getBitKlavierGalleriesDir()
    {
        return getUserDocumentsDir().getChildFile ("bitKlavier").getChildFile ("galleries");
    }

    // Expands a leading "~/" to the home directory, except "~/Documents[/...]" which is resolved
    // through the real Documents folder (see above).
    inline juce::File expandUserPath (const juce::String& path)
    {
        if (path.isEmpty())
            return {};

        if (path == "~")
            return juce::File::getSpecialLocation (juce::File::userHomeDirectory);

        if (path == "~/Documents")
            return getUserDocumentsDir();

        if (path.startsWith ("~/Documents/"))
            return getUserDocumentsDir().getChildFile (path.fromFirstOccurrenceOf ("~/Documents/", false, false));

        if (path.startsWith ("~/"))
            return juce::File::getSpecialLocation (juce::File::userHomeDirectory)
                .getChildFile (path.fromFirstOccurrenceOf ("~/", false, false));

        return juce::File (path);
    }
}
