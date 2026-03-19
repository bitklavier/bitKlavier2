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

        if (file.existsAsFile())
            tree = juce::ValueTree::fromXml (file.loadFileAsString());

        if (! tree.isValid())
            tree = juce::ValueTree ("Preferences");

        tree.addListener (this);

        //juce::String path_to_samples = "~/Library/Application Support/bitklavier/samples";
        juce::String path_to_samples = "~/Documents/bitKlavier/samples";
        if (! tree.hasProperty ("default_sample_path"))
            tree.setProperty ("default_sample_path", path_to_samples, nullptr);

        juce::String path_to_soundfonts = "~/Documents/bitKlavier/soundfonts";
        if (! tree.hasProperty ("default_soundfonts_path"))
            tree.setProperty ("default_soundfonts_path", path_to_soundfonts, nullptr);

        juce::String path_to_galleries = "~/Documents/bitKlavier/galleries";
        if (! tree.hasProperty ("default_galleries_path"))
            tree.setProperty ("default_galleries_path", path_to_galleries, nullptr);

        // Remember last opened gallery path (if any). Empty by default.
        if (! tree.hasProperty ("last_gallery_path"))
            tree.setProperty ("last_gallery_path", juce::String(), nullptr);

        // Recent galleries list (child ValueTree, up to kMaxRecentGalleries entries)
        if (! tree.getChildWithName ("recentGalleries").isValid())
            tree.appendChild (juce::ValueTree ("recentGalleries"), nullptr);

        if (! tree.getChildWithName (IDs::midiPrefs).isValid())
            tree.appendChild (juce::ValueTree (IDs::midiPrefs), nullptr);

        if (tree.getChildWithName ("KNOWNPLUGINS").isValid())
        {
            knownPluginList.recreateFromXml (*tree.getChildWithName ("KNOWNPLUGINS").createXml());
        }

        auto val = int (tree.getProperty ("pluginSortMethod", juce::KnownPluginList::sortByManufacturer));
        pluginSortMethod = static_cast<juce::KnownPluginList::SortMethod> (val);
        knownPluginList.setCustomScanner (std::make_unique<CustomPluginScanner> (tree));
        knownPluginList.addChangeListener (this);
        /*
         *todo: confirm this is the correct new function to use
         *      it's either this one, or addHeadlessDefaultFormatsToManager(), since addDefaultFormats has been deleted from JUCE
         */
        addDefaultFormatsToManager (formatManager);
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

    static constexpr int kMaxRecentGalleries = 10;

    // Prepend a file to the recent-galleries list, deduplicating and capping at kMaxRecentGalleries.
    void addRecentGallery (const juce::File& f)
    {
        auto recentNode = tree.getChildWithName ("recentGalleries");
        if (! recentNode.isValid())
            return;

        const juce::String path = f.getFullPathName();

        // Remove any existing entry for this path so it will move to the front
        for (int i = recentNode.getNumChildren() - 1; i >= 0; --i)
        {
            if (recentNode.getChild (i).getProperty ("path").toString() == path)
                recentNode.removeChild (i, nullptr);
        }

        // Insert at front
        juce::ValueTree entry ("gallery");
        entry.setProperty ("path", path, nullptr);
        recentNode.addChild (entry, 0, nullptr);

        // Trim to max
        while (recentNode.getNumChildren() > kMaxRecentGalleries)
            recentNode.removeChild (recentNode.getNumChildren() - 1, nullptr);
    }

    // Returns up to kMaxRecentGalleries recent gallery files (skips missing files).
    juce::Array<juce::File> getRecentGalleries() const
    {
        juce::Array<juce::File> result;
        auto recentNode = tree.getChildWithName ("recentGalleries");
        if (! recentNode.isValid())
            return result;

        for (int i = 0; i < recentNode.getNumChildren(); ++i)
        {
            juce::File f (recentNode.getChild (i).getProperty ("path").toString());
            if (f.existsAsFile())
                result.add (f);
        }
        return result;
    }

    void clearRecentGalleries()
    {
        auto recentNode = tree.getChildWithName ("recentGalleries");
        if (recentNode.isValid())
            recentNode.removeAllChildren (nullptr);
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
    const char* const  projectName    = "bitKlavier4";
    const char* const  companyName    = "Many Arrows Music";
#ifdef VERSION
    const char* const  versionString  = VERSION;
#else
    const char* const  versionString  = "4.0.0";
#endif
    const int          versionNumber  =  0x40000;
}
#endif //BITKLAVIER2_USERPREFERENCES_H
