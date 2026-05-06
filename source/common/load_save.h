// Copyright (C) 2022-2025 Dan Trueman
// SPDX-License-Identifier: GPL-3.0-or-later

/* Copyright 2013-2019 Matt Tytel
 *
 * vital is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * vital is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with vital.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <map>
#include <set>
#include <string>

#include "chowdsp_json/third_party/nlohmann/json.hpp"
#include <juce_core/juce_core.h>

using json = nlohmann::json;

namespace bitklavier {
    const std::string kPresetExtension = "bk2";
    const std::string kSampleExtensionsList = "*.wav;*.flac";
  class StringLayout;
}

class MidiManager;
class SynthBase;

class LoadSave {
  public:
    class FileSorterAscending {
      public:
        FileSorterAscending() { }

        static int compareElements (juce::File a, juce::File b) {
          return a.getFullPathName().toLowerCase().compareNatural(b.getFullPathName().toLowerCase());
        }

      private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FileSorterAscending)
    };

    enum PresetStyle {
//      kBass,
//      kLead,
//      kKeys,
//      kPad,
//      kPercussion,
//      kSequence,
//      kExperimental,
//      kSfx,
      kTemplate,
      kNumPresetStyles
    };

    static const int kMaxCommentLength = 500;
    static const std::string kUserDirectoryName;
    static const std::string kPresetFolderName;
    static const std::string kWavetableFolderName;
    static const std::string kSkinFolderName;
    static const std::string kSampleFolderName;
    static const std::string kLfoFolderName;
    static const std::string kAdditionalWavetableFoldersName;
    static const std::string kAdditionalSampleFoldersName;

    static juce::String getAuthorFromFile(const juce::File& file);
    static juce::String getStyleFromFile(const juce::File& file);
    static std::string getAuthor(json file);
    static std::string getLicense(json state);

    static juce::File getConfigFile();
    static void writeCrashLog(juce::String crash_log);
    static void writeErrorLog(juce::String error_log);
    static juce::File getFavoritesFile();
    static juce::File getDefaultSkin();
    static std::string getAuthor();
    static std::vector<std::string> getAdditionalFolders(const std::string& name);

    static juce::File getDataDirectory();
    static std::vector<juce::File> getDirectories(const juce::String& folder_name);
    static std::vector<juce::File> getPresetDirectories();
    static std::vector<juce::File> getSampleDirectories();
    static juce::File getUserDirectory();
    static juce::File getUserPresetDirectory();
    static juce::File getUserSampleDirectory();
    static void getAllFilesOfTypeInDirectories(juce::Array<juce::File>& files, const juce::String& extensions,
                                               const std::vector<juce::File>& directories);
    static void getAllSamples(juce::Array<juce::File>& samples);
    static void getAllUserPresets(juce::Array<juce::File>& presets);
    static void getAllUserSamples(juce::Array<juce::File>& samples);

    static juce::File getShiftedFile(const juce::String directory_name, const juce::String& extensions,
                               const std::string& additional_folders_name, const juce::File& current_file, int shift);

  private:
    LoadSave() { }
};