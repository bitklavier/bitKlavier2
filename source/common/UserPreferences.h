//
// Created by Davis Polito on 8/12/24.
//

#ifndef BITKLAVIER2_USERPREFERENCES_H
#define BITKLAVIER2_USERPREFERENCES_H

#include <juce_core/juce_core.h>
#include "valuetree_utils/VariantConverters.h"

#include <juce_data_structures/juce_data_structures.h>
#include "tracktion_ValueTreeUtilities.h"
#include "Identifiers.h"
#include "PluginScannerSubprocess.h"

struct UserPreferences : private tracktion::engine::ValueTreeAllEventListener
    , public juce::ChangeListener
{
public:
    juce::File file;
    juce::ValueTree tree;

    UserPreferences()
    {
        juce::PropertiesFile::Options options;
        options.applicationName = "bitklavier";
        options.folderName = "bitklavier";
        options.filenameSuffix = ".settings";
        options.osxLibrarySubFolder = "Application Support";
        file = options.getDefaultFile();

        tree = juce::ValueTree::fromXml(file.loadFileAsString());
        if (!tree.isValid())
            tree = juce::ValueTree("Preferences");
        tree.addListener(this);

        //juce::String path_to_samples = "~/Library/Application Support/bitklavier/samples";
        juce::String path_to_samples = "~/Documents/bitKlavier/samples";
        tree.setProperty("default_sample_path", path_to_samples, nullptr);
        juce::String path_to_soundfonts = "~/Documents/bitKlavier/soundfonts";
        tree.setProperty("default_soundfonts_path", path_to_soundfonts, nullptr);

        juce::ValueTree a (IDs::midiPrefs);
        if(!tree.getChildWithName(IDs::midiPrefs).isValid())
            tree.appendChild(a, nullptr);
        if ( tree.getChildWithName("KNOWNPLUGINS").isValid()) {
            knownPluginList.recreateFromXml(*tree.getChildWithName("KNOWNPLUGINS").createXml());
        }
        auto val = int(tree.getProperty("pluginSortMethod", juce::KnownPluginList::sortByManufacturer));
        pluginSortMethod = static_cast<juce::KnownPluginList::SortMethod>(val);
        knownPluginList.setCustomScanner (std::make_unique<CustomPluginScanner>(tree));
        knownPluginList.addChangeListener(this);
        /*
         *todo: confirm this is the correct new function to use
         *      it's either this one, or addHeadlessDefaultFormatsToManager(), since addDefaultFormats has been deleted from JUCE
         */
        addDefaultFormatsToManager(formatManager);
        //formatManager.addDefaultFormats();
    }

    void changeListenerCallback(juce::ChangeBroadcaster *source) override {
        if (tree.getChildWithName("KNOWNPLUGINS").isValid()) {
            auto pluginList = tree.getChildWithName("KNOWNPLUGINS");
            pluginList.copyPropertiesAndChildrenFrom(juce::ValueTree::fromXml(*knownPluginList.createXml()),nullptr);
        } else
            tree.appendChild(juce::ValueTree::fromXml(*knownPluginList.createXml()),nullptr);
    }

    void valueTreeChanged() override {
        file.replaceWithText(tree.toXmlString());
    }
    juce::AudioPluginFormatManager formatManager;
    juce::KnownPluginList knownPluginList;
    juce::KnownPluginList::SortMethod pluginSortMethod;
    juce::Array<PluginDescriptionAndPreference> pluginDescriptionsAndPreference;
};

class UserPreferencesWrapper : private juce::ValueTree::Listener
{
public:

    juce::SharedResourcePointer<UserPreferences> userPreferences;
    juce::ValueTree tree = userPreferences->tree;
    //juce::CachedValue<juce::String> skin { tree, "skin", nullptr, "Light" };
    std::function<void(juce::String)> onSkinChange;

    UserPreferencesWrapper()
    {
        tree.addListener(this);
    }

private:
    void valueTreePropertyChanged(juce::ValueTree&, const juce::Identifier&) override
    {
        //skin.forceUpdateOfCachedValue();
        //juce::NullCheckedInvocation::invoke(onSkinChange, skin);
    }
};

namespace ProjectInfo
{
    const char* const  projectName    = "bitKlavier5";
    const char* const  companyName    = "Many Arrows Music";
    const char* const  versionString  = "0.0.1";
    const int          versionNumber  =  0x1;
}
#endif //BITKLAVIER2_USERPREFERENCES_H
