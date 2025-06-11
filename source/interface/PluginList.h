//
// Created by Davis Polito on 6/10/25.
//

#ifndef PLUGINLIST_H
#define PLUGINLIST_H
#include "synth_gui_interface.h"
#include "UserPreferences.h"
//==============================================================================
class CustomPluginListComponent final : public juce::PluginListComponent
{
public:
    CustomPluginListComponent (juce::AudioPluginFormatManager& manager,
                               juce::KnownPluginList& listToRepresent,
                               const juce::File& pedal,
                               UserPreferencesWrapper& _prefs,
                               bool async)
        : PluginListComponent (manager, listToRepresent, pedal, nullptr, async),prefs(_prefs)
    {
        addAndMakeVisible (validationModeLabel);
        addAndMakeVisible (validationModeBox);

        validationModeLabel.attachToComponent (&validationModeBox, true);
        validationModeLabel.setJustificationType (juce::Justification::right);
        validationModeLabel.setSize (100, 30);

        auto unusedId = 1;

        for (const auto mode : { "In-process", "Out-of-process" })
            validationModeBox.addItem (mode, unusedId++);

        validationModeBox.setSelectedItemIndex (prefs.tree.getProperty ("pluginScanMode"));

        validationModeBox.onChange = [this]
        {
            prefs.tree.setProperty ("pluginScanMode", validationModeBox.getSelectedItemIndex(),nullptr);
        };

        handleResize();
    }

    void resized() override
    {
        handleResize();
    }

private:
    void handleResize()
    {
        PluginListComponent::resized();

        const auto& buttonBounds = getOptionsButton().getBounds();
        validationModeBox.setBounds (buttonBounds.withWidth (130).withRightX (getWidth() - buttonBounds.getX()));
    }


    juce::Label validationModeLabel { {}, "Scan mode" };
    juce::ComboBox validationModeBox;
    UserPreferencesWrapper& prefs;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CustomPluginListComponent)
};

//==============================================================================
class SynthGuiInterface::PluginListWindow final : public juce::DocumentWindow
{
public:
    PluginListWindow ( SynthGuiInterface& owner, UserPreferencesWrapper& preferences, juce::AudioPluginFormatManager& pluginFormatManager)
        : DocumentWindow ("Available Plugins",
                          juce::LookAndFeel::getDefaultLookAndFeel().findColour (ResizableWindow::backgroundColourId),
                          DocumentWindow::minimiseButton | DocumentWindow::closeButton),owner(owner)

    {
        auto deadMansPedalFile = preferences.userPreferences->file.getSiblingFile ("RecentlyCrashedPluginsList");


        setContentOwned (new CustomPluginListComponent (pluginFormatManager,
                                                        preferences.userPreferences->knownPluginList,
                                                        deadMansPedalFile,
                                                        preferences,
                                                        true), true);

        setResizable (true, false);
        setResizeLimits (300, 400, 800, 1500);
        setTopLeftPosition (60, 60);

        //restoreWindowStateFromString (getAppProperties().getUserSettings()->getValue ("listWindowPos"));
        setVisible (true);
    }

    ~PluginListWindow() override
    {
        //getAppProperties().getUserSettings()->setValue ("listWindowPos", getWindowStateAsString());
        clearContentComponent();
    }

    void closeButtonPressed() override
    {
        owner.pluginListWindow = nullptr;
    }

private:

    SynthGuiInterface& owner;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginListWindow)
};
#endif //PLUGINLIST_H
