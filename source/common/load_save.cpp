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

#include "load_save.h"
#include "midi_manager.h"
#include "UserPreferences.h"

#define QUOTE(x) #x
#define STRINGIFY(x) QUOTE(x)

namespace {
  const std::string kLinuxUserDataDirectory = "~/.local/share/vital/";


  juce::Time getBuildTime() {
    juce::StringArray date_tokens;
    date_tokens.addTokens(STRINGIFY(BUILD_DATE), true);
    if (date_tokens.size() != 5)
      return juce::Time::getCompilationDate();

    int year = date_tokens[0].getIntValue();
    int month = date_tokens[1].getIntValue() - 1;
    int day = date_tokens[2].getIntValue();
    int hour = date_tokens[3].getIntValue();
    int minute = date_tokens[4].getIntValue();

    return juce::Time(year, month, day, hour, minute);
  }
} // namespace

const std::string LoadSave::kUserDirectoryName = "User";
const std::string LoadSave::kPresetFolderName = "Presets";
const std::string LoadSave::kSampleFolderName = "Samples";




//void LoadSave::loadSample(SynthBase* mainSynth, const json& json_sample) {
////  vital::Sample* sample = mainSynth->getSample();
////  if (sample)
////    sample->jsonToState(json_sample);
//}
//


//
//void LoadSave::initSaveInfo(std::map<std::string, juce::String>& save_info) {
//  save_info["preset_name"] = "";
//  save_info["author"] = "";
//  save_info["comments"] = "";
//  save_info["style"] = "";
//
////  for (int i = 0; i < vital::kNumMacros; ++i)
////    save_info["macro" + std::to_string(i + 1)] = "MACRO " + std::to_string(i + 1);
//}

//
//juce::String LoadSave::getAuthorFromFile(const juce::File& file) {
//  static constexpr int kMaxCharacters = 40;
//  static constexpr int kMinSize = 60;
//  juce::FileInputStream file_stream(file);
//
//  if (file_stream.getTotalLength() < kMinSize)
//    return "";
//
//  file_stream.readByte();
//  file_stream.readByte();
//  juce::MemoryBlock author_memory_block;
//  file_stream.readIntoMemoryBlock(author_memory_block, 6);
//
//  char end_quote = file_stream.readByte();
//  char colon = file_stream.readByte();
//  char begin_quote = file_stream.readByte();
//  if (author_memory_block.toString() != "author" || end_quote != '"' || colon != ':' || begin_quote != '"') {
//    try {
//      json parsed_json_state = json::parse(file.loadFileAsString().toStdString(), nullptr, false);
//      return getAuthor(parsed_json_state);
//    }
//    catch (const json::exception& e) {
//      return "";
//    }
//  }
//
//  juce::MemoryBlock name_memory_block;
//  file_stream.readIntoMemoryBlock(name_memory_block, kMaxCharacters);
//  juce::String name = name_memory_block.toString();
//
//  if (!name.contains("\""))
//    return name.toStdString();
//
//  juce::StringArray tokens;
//  tokens.addTokens(name, "\"", "");
//
//  return tokens[0];
//}
//
//juce::String LoadSave::getStyleFromFile(const juce::File& file) {
//  static constexpr int kMinSize = 5000;
//  juce::FileInputStream file_stream(file);
//
//  if (file_stream.getTotalLength() < kMinSize)
//    return "";
//
//  juce::MemoryBlock style_memory_block;
//  file_stream.readIntoMemoryBlock(style_memory_block, kMinSize);
//
//  juce::StringArray tokens;
//  tokens.addTokens(style_memory_block.toString(), "\"", "");
//  bool found_style = false;
//  for (juce::String token : tokens) {
//    if (found_style && token.trim() != ":")
//      return token;
//    if (token == "preset_style")
//      found_style = true;
//  }
//
//  return "";
//}
//


void LoadSave::writeCrashLog(juce::String crash_log) {
  juce::File data_dir = getDataDirectory();
  if (!data_dir.exists() || !data_dir.isDirectory())
    return;
  juce::File file = data_dir.getChildFile("crash.txt");
  file.replaceWithText(crash_log);
}

void LoadSave::writeErrorLog(juce::String error_log) {
  juce::File data_dir = getDataDirectory();
  if (!data_dir.exists() || !data_dir.isDirectory())
    return; 

  juce::File file = getDataDirectory().getChildFile("errors.txt");
  file.appendText(error_log + "\n");
}


juce::File LoadSave::getFavoritesFile() {
//    //using namespace ProjectInfo;
//    #if defined(JUCE_DATA_STRUCTURES_H_INCLUDED)
//    juce::PropertiesFile::Options config_options;
//    config_options.applicationName = "Vial";
//    config_options.osxLibrarySubFolder = "Application Support";
//    config_options.filenameSuffix = "favorites";
//
//    #ifdef LINUX
//    config_options.folderName = "." + juce::String(ProjectInfo::projectName).toLowerCase();
//    #else
//    config_options.folderName = juce::String(ProjectInfo::projectName).toLowerCase();
//    #endif
//
//    return config_options.getDefaultFile();
//    #else
//    return juce::File();
//    #endif
}

juce::File LoadSave::getDefaultSkin() {
#if defined(JUCE_DATA_STRUCTURES_H_INCLUDED)
  juce::PropertiesFile::Options config_options;
  config_options.applicationName = "Vial";
  config_options.osxLibrarySubFolder = "Application Support";
  config_options.filenameSuffix = "skin";

#ifdef LINUX
  config_options.folderName = "." + juce::String(ProjectInfo::projectName).toLowerCase();
#else
  config_options.folderName = juce::String(ProjectInfo::projectName).toLowerCase();
#endif

  return config_options.getDefaultFile();
#else
  return juce::File();
#endif
}


//
//bool LoadSave::isInstalled() {
//  return getDataDirectory().exists();
////}
////
//
//std::string LoadSave::getAuthor() {
//  json data = getConfigJson();
//
//  if (data.count("author"))
//    return data["author"];
//
//  return "";
//}



std::vector<juce::File> LoadSave::getPresetDirectories() {
  return getDirectories(kPresetFolderName);
}


std::vector<juce::File> LoadSave::getSampleDirectories() {
  return getDirectories(kSampleFolderName);
}

juce::File LoadSave::getUserDirectory() {
  juce::File directory = getDataDirectory().getChildFile(kUserDirectoryName);
  if (!directory.exists())
    directory.createDirectory();
  return directory;
}

juce::File LoadSave::getUserPresetDirectory() {
  juce::File directory = getUserDirectory().getChildFile(kPresetFolderName);
  if (!directory.exists())
    directory.createDirectory();
  return directory;
}

juce::File LoadSave::getUserSampleDirectory() {
  juce::File directory = getUserDirectory().getChildFile(kSampleFolderName);
  if (!directory.exists())
    directory.createDirectory();
  return directory;
}

void LoadSave::getAllFilesOfTypeInDirectories(juce::Array<juce::File>& files, const juce::String& extensions,
                                              const std::vector<juce::File>& directories) {
  files.clear();
  for (const juce::File& directory : directories) {
    if (directory.exists() && directory.isDirectory())
      directory.findChildFiles(files, juce::File::findFiles, true, extensions);
  }
}
std::vector<std::string> LoadSave::getAdditionalFolders(const std::string& name) {
//    json data = getConfigJson();
//    std::vector<std::string> folders;
//
//    if (data.count(name)) {
//        json folder_list = data[name];
//        for (json& folder : folder_list)
//            folders.push_back(folder);
//    }
//
//    return folders;
}

juce::File LoadSave::getDataDirectory() {
//    json data = getConfigJson();
//    if (data.count("data_directory")) {
//        std::string path = data["data_directory"];
//        juce::File folder(path);
//        if (folder.exists() && folder.isDirectory())
//            return folder;
//    }
//
//#ifdef LINUX
//    juce::File directory = juce::File(kLinuxUserDataDirectory);
//  juce::String xdg_data_home = juce::SystemStats::getEnvironmentVariable ("XDG_DATA_HOME", {});
//
//  if (!xdg_data_home.trim().isEmpty())
//    directory = juce::File(xdg_data_home).getChildFile("vial");
//
//#elif defined(__APPLE__)
//    juce::File home_directory = juce::File::getSpecialLocation(juce::File::userHomeDirectory);
//    juce::File directory = home_directory.getChildFile("Music").getChildFile("Vial");
//#else
//    juce::File documents_dir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
//  juce::File directory = documents_dir.getChildFile("Vial");
//#endif
//
//    return directory;
}

std::vector<juce::File> LoadSave::getDirectories(const juce::String& folder_name) {
    juce::File data_dir = getDataDirectory();
    std::vector<juce::File> directories;

    if (!data_dir.exists() || !data_dir.isDirectory())
        return directories;

    juce::Array<juce::File> sub_folders;
    sub_folders.add(data_dir);
    data_dir.findChildFiles(sub_folders, juce::File::findDirectories, false);
    for (const juce::File& sub_folder : sub_folders) {
        juce::File directory = sub_folder.getChildFile(folder_name);
        if (directory.exists() && directory.isDirectory())
            directories.push_back(directory);
    }

    return directories;
}

void LoadSave::getAllSamples(juce::Array<juce::File>& samples) {
  getAllFilesOfTypeInDirectories(samples, "*.wav", getSampleDirectories());
}

void LoadSave::getAllUserPresets(juce::Array<juce::File>& presets) {
  std::vector<juce::File> directories = {
    getDataDirectory().getChildFile(kPresetFolderName),
    getUserPresetDirectory()
  };
  getAllFilesOfTypeInDirectories(presets, juce::String("*.") + bitklavier::kPresetExtension, directories);
}

void LoadSave::getAllUserSamples(juce::Array<juce::File>& samples) {
  std::vector<juce::File> directories = {
    getDataDirectory().getChildFile(kSampleFolderName),
    getUserSampleDirectory()
  }; 
  getAllFilesOfTypeInDirectories(samples, "*.wav", directories);
}



juce::File LoadSave::getShiftedFile(const juce::String directory_name, const juce::String& extensions,
                              const std::string& additional_folders_name, const juce::File& current_file, int shift) {
  FileSorterAscending file_sorter;

  std::vector<juce::File> directories = getDirectories(directory_name);
  std::vector<std::string> additional = getAdditionalFolders(additional_folders_name);
  for (const std::string& path : additional)
    directories.push_back(juce::File(path));

  juce::Array<juce::File> all_files;
  getAllFilesOfTypeInDirectories(all_files, extensions, directories);
  if (all_files.isEmpty())
    return juce::File();

  all_files.sort(file_sorter);
  int index = all_files.indexOf(current_file);
  if (index < 0)
    return all_files[0];
  return all_files[(index + shift + all_files.size()) % all_files.size()];
}
