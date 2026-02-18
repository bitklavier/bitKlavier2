//
// Created by Davis Polito on 9/30/24.
//

#ifndef BITKLAVIER2_BITKLAVIER_AUDIODEVICESELECTORCOMPONENT_H
#define BITKLAVIER2_BITKLAVIER_AUDIODEVICESELECTORCOMPONENT_H

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "Identifiers.h"

namespace bitklavier {
    //==============================================================================
/**
    A component containing controls to let the user change the audio settings of
    an juce::AudioDeviceManager object.

    Very easy to use - just create one of these and show it to the user.

    @see juce::AudioDeviceManager

    @tags{Audio}
*/

class   AudioDeviceSelectorComponent  : public juce::Component,
private juce::ChangeListener
{
    public:
    //==============================================================================
    /** Creates the component.

        If your app needs only output channels, you might ask for a maximum of 0 input
        channels, and the component won't display any options for choosing the input
        channels. And likewise if you're doing an input-only app.

        @param deviceManager            the device manager that this component should control
        @param minAudioInputChannels    the minimum number of audio input channels that the application needs
        @param maxAudioInputChannels    the maximum number of audio input channels that the application needs
        @param minAudioOutputChannels   the minimum number of audio output channels that the application needs
        @param maxAudioOutputChannels   the maximum number of audio output channels that the application needs
        @param showMidiInputOptions     if true, the component will allow the user to select which midi inputs are enabled
        @param showMidiOutputSelector   if true, the component will let the user choose a default midi output device
        @param showChannelsAsStereoPairs    if true, channels will be treated as pairs; if false, channels will be
                                        treated as a set of separate mono channels.
        @param hideAdvancedOptionsWithButton    if true, only the minimum amount of UI components
                                        are shown, with an "advanced" button that shows the rest of them
    */
    AudioDeviceSelectorComponent (juce::AudioDeviceManager& deviceManager,
    int minAudioInputChannels,
    int maxAudioInputChannels,
    int minAudioOutputChannels,
    int maxAudioOutputChannels,
    bool showMidiInputOptions,
    bool showMidiOutputSelector,
    bool showChannelsAsStereoPairs,
    bool hideAdvancedOptionsWithButton, juce::ValueTree tree);

    /** Destructor */
    ~AudioDeviceSelectorComponent() override;

    /** The device manager that this component is controlling */
    juce::AudioDeviceManager& deviceManager;

    /** Sets the standard height used for items in the panel. */
    void setItemHeight (int itemHeight);

    /** Returns the standard height used for items in the panel. */
    int getItemHeight() const noexcept      { return itemHeight; }

    /** Returns the juce::ListBox that's being used to show the midi inputs, or nullptr if there isn't one. */
    juce::ListBox* getMidiInputSelectorListBox() const noexcept;

    /** Persist the current audio device setup to the provided ValueTree state. */
    void persistCurrentSetup();

    //==============================================================================
    /** @internal */
    void resized() override;

    /** @internal */
    void childBoundsChanged (juce::Component* child) override;

    private:
    //==============================================================================
    void handleBluetoothButton();
    void updateDeviceType();
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void updateAllControls();

    std::unique_ptr<juce::ComboBox> deviceTypeDropDown;
    std::unique_ptr<juce::Label> deviceTypeDropDownLabel;
    std::unique_ptr<juce::Component> audioDeviceSettingsComp;
    juce::String audioDeviceSettingsCompType;
    int itemHeight = 0;
    const int minOutputChannels, maxOutputChannels, minInputChannels, maxInputChannels;
    const bool showChannelsAsStereoPairs;
    const bool hideAdvancedOptionsWithButton;

    class MidiInputSelectorComponentListBox;
    class MidiOutputSelector;

    juce::Array<juce::MidiDeviceInfo> currentMidiOutputs;
    std::unique_ptr<MidiInputSelectorComponentListBox> midiInputsList;
    std::unique_ptr<MidiOutputSelector> midiOutputSelector;
    std::unique_ptr<juce::Label> midiInputsLabel, midiOutputLabel;
    std::unique_ptr<juce::TextButton> bluetoothButton;
    juce::ValueTree state;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioDeviceSelectorComponent)
};

}


#endif //BITKLAVIER2_BITKLAVIER_AUDIODEVICESELECTORCOMPONENT_H
