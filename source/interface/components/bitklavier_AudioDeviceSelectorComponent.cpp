//
// Created by Davis Polito on 9/30/24.
//

#include "bitklavier_AudioDeviceSelectorComponent.h"
/*
  ==============================================================================

   This file is part of the JUCE framework.
   Copyright (c) Raw Material Software Limited

   JUCE is an open source framework subject to commercial or open source
   licensing.

   By downloading, installing, or using the JUCE framework, or combining the
   JUCE framework with any other source code, object code, content or any other
   copyrightable work, you agree to the terms of the JUCE End User Licence
   Agreement, and all incorporated terms including the JUCE Privacy Policy and
   the JUCE Website Terms of Service, as applicable, which will bind you. If you
   do not agree to the terms of these agreements, we will not license the JUCE
   framework to you, and you must discontinue the installation or download
   process and cease use of the JUCE framework.

   JUCE End User Licence Agreement: https://juce.com/legal/juce-8-licence/
   JUCE Privacy Policy: https://juce.com/juce-privacy-policy
   JUCE Website Terms of Service: https://juce.com/juce-website-terms-of-service/

   Or:

   You may also use this code under the terms of the AGPLv3:
   https://www.gnu.org/licenses/agpl-3.0.en.html

   THE JUCE FRAMEWORK IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL
   WARRANTIES, WHETHER EXPRESSED OR IMPLIED, INCLUDING WARRANTY OF
   MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE, ARE DISCLAIMED.

  ==============================================================================
*/

namespace bitklavier
{

    struct SimpleDeviceManagerInputLevelMeter final : public juce::Component,
                                                      public juce::Timer
    {
        SimpleDeviceManagerInputLevelMeter (juce::AudioDeviceManager& m)  : manager (m)
        {
            startTimerHz (20);
            inputLevelGetter = manager.getInputLevelGetter();
        }

        void timerCallback() override
        {
            if (isShowing())
            {
                auto newLevel = (float) inputLevelGetter->getCurrentLevel();

                if (std::abs (level - newLevel) > 0.005f)
                {
                    level = newLevel;
                    repaint();
                }
            }
            else
            {
                level = 0;
            }
        }

        void paint (juce::Graphics& g) override
        {
            // (add a bit of a skew to make the level more obvious)
            getLookAndFeel().drawLevelMeter (g, getWidth(), getHeight(),
                                             (float) std::exp (std::log (level) / 3.0));
        }

        juce::AudioDeviceManager& manager;
        juce::AudioDeviceManager::LevelMeter::Ptr inputLevelGetter;
        float level = 0;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleDeviceManagerInputLevelMeter)
    };

    static void drawTextLayout (juce::Graphics& g, juce::Component& owner, juce::StringRef text, const juce::Rectangle<int>& textBounds, bool enabled)
    {
        const auto textColour = owner.findColour (juce::ListBox::textColourId, true).withMultipliedAlpha (enabled ? 1.0f : 0.6f);

        juce::AttributedString attributedString { text };
        attributedString.setColour (textColour);
        attributedString.setFont (owner.withDefaultMetrics (juce::FontOptions { (float) textBounds.getHeight() * 0.6f }));
        attributedString.setJustification (juce::Justification::centredLeft);
        attributedString.setWordWrap (juce::AttributedString::WordWrap::none);

        juce::TextLayout textLayout;
        textLayout.createLayout (attributedString,
                                 (float) textBounds.getWidth(),
                                 (float) textBounds.getHeight());
        textLayout.draw (g, textBounds.toFloat());
    }


//==============================================================================
    class AudioDeviceSelectorComponent::MidiInputSelectorComponentListBox final : public juce::ListBox,
                                                                                  private juce::ListBoxModel
    {
    public:
//        struct MidiDeviceInfoWrapper : public juce::MidiDeviceInfo
//        {
//            MidiDeviceInfoWrapper(const juce::ValueTree& v) : state(v)
//            {
//                jassert(v.hasType(IDs::midiInput));
//                _name.referTo(state,IDs::name, nullptr, this->name);
//                _identifier.referTo(state,IDs::name, nullptr, this->identifier);
//            }
//
//
//
//            juce::ValueTree state;
//            juce::CachedValue <juce::String> _name;
//            juce::CachedValue <juce::String> _identifier;
//        };
        MidiInputSelectorComponentListBox (juce::AudioDeviceManager& dm, const juce::String& noItems)
                : juce::ListBox ({}, nullptr),
                  deviceManager (dm),
                  noItemsMessage (noItems)

        {
            updateDevices();
            setModel (this);
            setOutlineThickness (1);
        }

        void updateDevices()
        {
            items = juce::MidiInput::getAvailableDevices();
        }

        int getNumRows() override
        {
            return items.size();
        }

        void paintListBoxItem (int row, juce::Graphics& g, int width, int height, bool rowIsSelected) override
        {
            if (juce::isPositiveAndBelow (row, items.size()))
            {
                if (rowIsSelected)
                    g.fillAll (findColour (juce::TextEditor::highlightColourId)
                                       .withMultipliedAlpha (0.3f));

                auto item = items[row];
                bool enabled = deviceManager.isMidiInputDeviceEnabled (item.identifier);

                auto x = getTickX();
                auto tickW = (float) height * 0.75f;

                getLookAndFeel().drawTickBox (g, *this, (float) x - tickW, ((float) height - tickW) * 0.5f, tickW, tickW,
                                              enabled, true, true, false);

                drawTextLayout (g, *this, item.name, { x + 5, 0, width - x - 5, height }, enabled);
            }
        }

        void listBoxItemClicked (int row, const juce::MouseEvent& e) override
        {
            selectRow (row);

            if (e.x < getTickX())
                flipEnablement (row);
        }

        void listBoxItemDoubleClicked (int row, const juce::MouseEvent&) override
        {
            flipEnablement (row);
        }

        void returnKeyPressed (int row) override
        {
            flipEnablement (row);
        }

        void paint (juce::Graphics& g) override
        {
            juce::ListBox::paint (g);

            if (items.isEmpty())
            {
                g.setColour (juce::Colours::grey);
                g.setFont (0.5f * (float) getRowHeight());
                g.drawText (noItemsMessage,
                            0, 0, getWidth(), getHeight() / 2,
                            juce::Justification::centred, true);
            }
        }

        int getBestHeight (int preferredHeight)
        {
            auto extra = getOutlineThickness() * 2;

            return juce::jmax (getRowHeight() * 2 + extra,
                         juce::jmin (getRowHeight() * getNumRows() + extra,
                               preferredHeight));
        }

    private:
        //==============================================================================
        juce::AudioDeviceManager& deviceManager;
        const juce::String noItemsMessage;
        juce::Array<juce::MidiDeviceInfo> items;
        void flipEnablement (const int row)
        {
            if (juce::isPositiveAndBelow (row, items.size()))
            {
                auto identifier = items[row].identifier;
                deviceManager.setMidiInputDeviceEnabled (identifier, ! deviceManager.isMidiInputDeviceEnabled (identifier));
            }
        }

        int getTickX() const
        {
            return getRowHeight();
        }

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiInputSelectorComponentListBox)
    };


//==============================================================================
    struct AudioDeviceSetupDetails
    {
        juce::AudioDeviceManager* manager;
        int minNumInputChannels, maxNumInputChannels;
        int minNumOutputChannels, maxNumOutputChannels;
        bool useStereoPairs;
    };

    static juce::String getNoDeviceString()   { return "<< " + TRANS ("none") + " >>"; }

//==============================================================================
    class AudioDeviceSelectorComponent::MidiOutputSelector final : public juce::Component,
                                                                   private juce::ChangeListener
    {
    public:
        explicit MidiOutputSelector (juce::AudioDeviceManager& dm)
                : deviceManager (dm)
        {
            deviceManager.addChangeListener (this);
            selector.onChange = [&]
            {
                const auto selectedId = selector.getSelectedId();
                jassert (selectedId != 0);

                const auto deviceId = selectedId == -1
                                      ? juce::String{}
                                      : juce::MidiOutput::getAvailableDevices()[selectedId - 1].identifier;
                deviceManager.setDefaultMidiOutputDevice (deviceId);
            };

            updateListOfDevices();
            addAndMakeVisible (selector);
        }

        ~MidiOutputSelector() final
        {
            deviceManager.removeChangeListener (this);
        }

        void resized() final { selector.setBounds (getLocalBounds()); }

    private:
        void updateListOfDevices()
        {
            selector.clear();

            const auto midiOutputs = juce::MidiOutput::getAvailableDevices();

            selector.addItem (getNoDeviceString(), -1);
            selector.setSelectedId (-1, juce::dontSendNotification);
            selector.addSeparator();

            for (auto [id, midiOutput] : enumerate (midiOutputs, 1))
            {
                selector.addItem (midiOutput.name, id);

                if (midiOutput.identifier == deviceManager.getDefaultMidiOutputIdentifier())
                    selector.setSelectedId (id, juce::dontSendNotification);
            }
        }

        void changeListenerCallback (juce::ChangeBroadcaster*) final { updateListOfDevices(); }

        juce::ComboBox selector;
        juce::AudioDeviceManager& deviceManager;
    };

//==============================================================================
    class AudioDeviceSettingsPanel : public juce::Component,
                                     private juce::ChangeListener
    {
    public:
        AudioDeviceSettingsPanel (juce::AudioIODeviceType& t, AudioDeviceSetupDetails& setupDetails,
                                  const bool hideAdvancedOptionsWithButton,
                                  AudioDeviceSelectorComponent& p)
                : type (t), setup (setupDetails), parent (p)
        {
            if (hideAdvancedOptionsWithButton)
            {
                showAdvancedSettingsButton = std::make_unique <juce::TextButton> (TRANS ("Show advanced settings..."));
                addAndMakeVisible (showAdvancedSettingsButton.get());
                showAdvancedSettingsButton->setClickingTogglesState (true);
                showAdvancedSettingsButton->onClick = [this] { toggleAdvancedSettings(); };
            }

            type.scanForDevices();

            setup.manager->addChangeListener (this);

            updateAllControls();
        }

        ~AudioDeviceSettingsPanel() override
        {
            setup.manager->removeChangeListener (this);
        }

        void resized() override
        {
            juce::Rectangle<int> r (proportionOfWidth (0.35f), 0, proportionOfWidth (0.6f), 3000);

            const int maxListBoxHeight = 100;
            const int h = parent.getItemHeight();
            const int space = h / 4;

            if (outputDeviceDropDown != nullptr)
            {
                auto row = r.removeFromTop (h);

                if (testButton != nullptr)
                {
                    testButton->changeWidthToFitText (h);
                    testButton->setBounds (row.removeFromRight (testButton->getWidth()));
                    row.removeFromRight (space);
                }

                outputDeviceDropDown->setBounds (row);
                r.removeFromTop (space);
            }

            if (inputDeviceDropDown != nullptr)
            {
                auto row = r.removeFromTop (h);

                inputLevelMeter->setBounds (row.removeFromRight (testButton != nullptr ? testButton->getWidth() : row.getWidth() / 6));
                row.removeFromRight (space);
                inputDeviceDropDown->setBounds (row);
                r.removeFromTop (space);
            }

            if (outputChanList != nullptr)
            {
                outputChanList->setRowHeight (juce::jmin (22, h));
                outputChanList->setBounds (r.removeFromTop (outputChanList->getBestHeight (maxListBoxHeight)));
                outputChanLabel->setBounds (0, outputChanList->getBounds().getCentreY() - h / 2, r.getX(), h);
                r.removeFromTop (space);
            }

            if (inputChanList != nullptr)
            {
                inputChanList->setRowHeight (juce::jmin (22, h));
                inputChanList->setBounds (r.removeFromTop (inputChanList->getBestHeight (maxListBoxHeight)));
                inputChanLabel->setBounds (0, inputChanList->getBounds().getCentreY() - h / 2, r.getX(), h);
                r.removeFromTop (space);
            }

            r.removeFromTop (space * 2);

            if (showAdvancedSettingsButton != nullptr
                && sampleRateDropDown != nullptr && bufferSizeDropDown != nullptr)
            {
                showAdvancedSettingsButton->setBounds (r.removeFromTop (h));
                r.removeFromTop (space);
                showAdvancedSettingsButton->changeWidthToFitText();
            }

            auto advancedSettingsVisible = showAdvancedSettingsButton == nullptr
                                           || showAdvancedSettingsButton->getToggleState();

            if (sampleRateDropDown != nullptr)
            {
                sampleRateDropDown->setVisible (advancedSettingsVisible);

                if (advancedSettingsVisible)
                {
                    sampleRateDropDown->setBounds (r.removeFromTop (h));
                    r.removeFromTop (space);
                }
            }

            if (bufferSizeDropDown != nullptr)
            {
                bufferSizeDropDown->setVisible (advancedSettingsVisible);

                if (advancedSettingsVisible)
                {
                    bufferSizeDropDown->setBounds (r.removeFromTop (h));
                    r.removeFromTop (space);
                }
            }

            r.removeFromTop (space);

            if (showUIButton != nullptr || resetDeviceButton != nullptr)
            {
                auto buttons = r.removeFromTop (h);

                if (showUIButton != nullptr)
                {
                    showUIButton->setVisible (advancedSettingsVisible);
                    showUIButton->changeWidthToFitText (h);
                    showUIButton->setBounds (buttons.removeFromLeft (showUIButton->getWidth()));
                    buttons.removeFromLeft (space);
                }

                if (resetDeviceButton != nullptr)
                {
                    resetDeviceButton->setVisible (advancedSettingsVisible);
                    resetDeviceButton->changeWidthToFitText (h);
                    resetDeviceButton->setBounds (buttons.removeFromLeft (resetDeviceButton->getWidth()));
                }

                r.removeFromTop (space);
            }

            setSize (getWidth(), r.getY());
        }

        void updateConfig (bool updateOutputDevice, bool updateInputDevice, bool updateSampleRate, bool updateBufferSize)
        {
            auto config = setup.manager->getAudioDeviceSetup();
            juce::String error;

            if (updateOutputDevice || updateInputDevice)
            {
                if (outputDeviceDropDown != nullptr)
                    config.outputDeviceName = outputDeviceDropDown->getSelectedId() < 0 ? juce::String()
                                                                                        : outputDeviceDropDown->getText();

                if (inputDeviceDropDown != nullptr)
                    config.inputDeviceName = inputDeviceDropDown->getSelectedId() < 0 ? juce::String()
                                                                                      : inputDeviceDropDown->getText();

                if (! type.hasSeparateInputsAndOutputs())
                    config.inputDeviceName = config.outputDeviceName;

                if (updateInputDevice)
                    config.useDefaultInputChannels = true;
                else
                    config.useDefaultOutputChannels = true;

                error = setup.manager->setAudioDeviceSetup (config, true);

                updateSelectedInput();
                updateSelectedOutput();
                updateControlPanelButton();
                resized();
            }
            else if (updateSampleRate)
            {
                if (sampleRateDropDown->getSelectedId() > 0)
                {
                    config.sampleRate = sampleRateDropDown->getSelectedId();
                    error = setup.manager->setAudioDeviceSetup (config, true);
                }
            }
            else if (updateBufferSize)
            {
                if (bufferSizeDropDown->getSelectedId() > 0)
                {
                    config.bufferSize = bufferSizeDropDown->getSelectedId();
                    error = setup.manager->setAudioDeviceSetup (config, true);
                }
            }

            if (error.isNotEmpty())
                messageBox = juce::AlertWindow::showScopedAsync (juce::MessageBoxOptions().withIconType (juce::MessageBoxIconType::WarningIcon)
                                                                   .withTitle (TRANS ("Error when trying to open audio device!"))
                                                                   .withMessage (error)
                                                                   .withButton (TRANS ("OK")),
                                                           nullptr);
        }

        bool showDeviceControlPanel()
        {
            if (auto* device = setup.manager->getCurrentAudioDevice())
            {
                juce::Component modalWindow;
                modalWindow.setOpaque (true);
                modalWindow.addToDesktop (0);
                modalWindow.enterModalState();

                return device->showControlPanel();
            }

            return false;
        }

        void toggleAdvancedSettings()
        {
            showAdvancedSettingsButton->setButtonText ((showAdvancedSettingsButton->getToggleState() ? "Hide " : "Show ")
                                                       + juce::String ("advanced settings..."));
            resized();
        }

        void showDeviceUIPanel()
        {
            if (showDeviceControlPanel())
            {
                setup.manager->closeAudioDevice();
                setup.manager->restartLastAudioDevice();
                getTopLevelComponent()->toFront (true);
            }
        }

        void playTestSound()
        {
            setup.manager->playTestSound();
        }

        void updateAllControls()
        {
            updateOutputsComboBox();
            updateInputsComboBox();

            updateControlPanelButton();
            updateResetButton();

            if (auto* currentDevice = setup.manager->getCurrentAudioDevice())
            {
                if (setup.maxNumOutputChannels > 0
                    && setup.minNumOutputChannels < setup.manager->getCurrentAudioDevice()->getOutputChannelNames().size())
                {
                    if (outputChanList == nullptr)
                    {
                        outputChanList = std::make_unique<ChannelSelectorListBox> (setup, ChannelSelectorListBox::audioOutputType,
                                                                                   TRANS ("(no audio output channels found)"));
                        addAndMakeVisible (outputChanList.get());
                        outputChanLabel = std::make_unique<juce::Label> (juce::String{}, TRANS ("Active output channels:"));
                        outputChanLabel->setJustificationType (juce::Justification::centredRight);
                        outputChanLabel->attachToComponent (outputChanList.get(), true);
                    }

                    outputChanList->refresh();
                }
                else
                {
                    outputChanLabel.reset();
                    outputChanList.reset();
                }

                if (setup.maxNumInputChannels > 0
                    && setup.minNumInputChannels < setup.manager->getCurrentAudioDevice()->getInputChannelNames().size())
                {
                    if (inputChanList == nullptr)
                    {
                        inputChanList = std::make_unique<ChannelSelectorListBox> (setup, ChannelSelectorListBox::audioInputType,
                                                                                  TRANS ("(no audio input channels found)"));
                        addAndMakeVisible (inputChanList.get());
                        inputChanLabel = std::make_unique<juce::Label> (juce::String{}, TRANS ("Active input channels:"));
                        inputChanLabel->setJustificationType (juce::Justification::centredRight);
                        inputChanLabel->attachToComponent (inputChanList.get(), true);
                    }

                    inputChanList->refresh();
                }
                else
                {
                    inputChanLabel.reset();
                    inputChanList.reset();
                }

                updateSampleRateComboBox (currentDevice);
                updateBufferSizeComboBox (currentDevice);
            }
            else
            {
                jassert (setup.manager->getCurrentAudioDevice() == nullptr); // not the correct device type!

                inputChanLabel.reset();
                outputChanLabel.reset();
                sampleRateLabel.reset();
                bufferSizeLabel.reset();

                inputChanList.reset();
                outputChanList.reset();
                sampleRateDropDown.reset();
                bufferSizeDropDown.reset();

                if (outputDeviceDropDown != nullptr)
                    outputDeviceDropDown->setSelectedId (-1, juce::NotificationType::dontSendNotification);

                if (inputDeviceDropDown != nullptr)
                    inputDeviceDropDown->setSelectedId (-1, juce::NotificationType::dontSendNotification);
            }

            sendLookAndFeelChange();
            resized();
            setSize (getWidth(), getLowestY() + 4);
        }

        void changeListenerCallback (juce::ChangeBroadcaster*) override
        {
            updateAllControls();
        }

        void resetDevice()
        {
            setup.manager->closeAudioDevice();
            setup.manager->restartLastAudioDevice();
        }

    private:
        juce::AudioIODeviceType& type;
        const AudioDeviceSetupDetails setup;
        AudioDeviceSelectorComponent& parent;

        std::unique_ptr<juce::ComboBox> outputDeviceDropDown, inputDeviceDropDown, sampleRateDropDown, bufferSizeDropDown;
        std::unique_ptr<juce::Label> outputDeviceLabel, inputDeviceLabel, sampleRateLabel, bufferSizeLabel, inputChanLabel, outputChanLabel;
        std::unique_ptr<juce::TextButton> testButton;
        std::unique_ptr<juce::Component> inputLevelMeter;
        std::unique_ptr<juce::TextButton> showUIButton, showAdvancedSettingsButton, resetDeviceButton;

        int findSelectedDeviceIndex (bool isInput) const
        {
            const auto device = setup.manager->getAudioDeviceSetup();
            const auto deviceName = isInput ? device.inputDeviceName : device.outputDeviceName;
            return type.getDeviceNames (isInput).indexOf (deviceName);
        }

        void updateSelectedInput()
        {
            showCorrectDeviceName (inputDeviceDropDown.get(), true);
        }

        void updateSelectedOutput()
        {
            constexpr auto isInput = false;
            showCorrectDeviceName (outputDeviceDropDown.get(), isInput);

            if (testButton != nullptr)
                testButton->setEnabled (findSelectedDeviceIndex (isInput) >= 0);
        }

        void showCorrectDeviceName (juce::ComboBox* box, bool isInput)
        {
            if (box == nullptr)
                return;

            const auto index = findSelectedDeviceIndex (isInput);
            box->setSelectedId (index < 0 ? index : index + 1, juce::NotificationType::dontSendNotification);
        }

        void addNamesToDeviceBox (juce::ComboBox& combo, bool isInputs)
        {
            const juce::StringArray devs (type.getDeviceNames (isInputs));

            combo.clear (juce::NotificationType::dontSendNotification);

            for (int i = 0; i < devs.size(); ++i)
                combo.addItem (devs[i], i + 1);

            combo.addItem (getNoDeviceString(), -1);
            combo.setSelectedId (-1, juce::NotificationType::dontSendNotification);
        }

        int getLowestY() const
        {
            int y = 0;

            for (auto* c : getChildren())
                y = juce::jmax (y, c->getBottom());

            return y;
        }

        void updateControlPanelButton()
        {
            auto* currentDevice = setup.manager->getCurrentAudioDevice();
            showUIButton.reset();

            if (currentDevice != nullptr && currentDevice->hasControlPanel())
            {
                showUIButton = std::make_unique<juce::TextButton> (TRANS ("Control Panel"),
                                                             TRANS ("Opens the device's own control panel"));
                addAndMakeVisible (showUIButton.get());
                showUIButton->onClick = [this] { showDeviceUIPanel(); };
            }

            resized();
        }

        void updateResetButton()
        {
            if (auto* currentDevice = setup.manager->getCurrentAudioDevice())
            {
                if (currentDevice->hasControlPanel())
                {
                    if (resetDeviceButton == nullptr)
                    {
                        resetDeviceButton = std::make_unique<juce::TextButton> (TRANS ("Reset juce::Device"),
                                                                          TRANS ("Resets the audio interface - sometimes needed after changing a device's properties in its custom control panel"));
                        addAndMakeVisible (resetDeviceButton.get());
                        resetDeviceButton->onClick = [this] { resetDevice(); };
                        resized();
                    }

                    return;
                }
            }

            resetDeviceButton.reset();
        }

        void updateOutputsComboBox()
        {
            if (setup.maxNumOutputChannels > 0 || ! type.hasSeparateInputsAndOutputs())
            {
                if (outputDeviceDropDown == nullptr)
                {
                    outputDeviceDropDown = std::make_unique<juce::ComboBox>();
                    outputDeviceDropDown->onChange = [this] { updateConfig (true, false, false, false); };

                    addAndMakeVisible (outputDeviceDropDown.get());

                    outputDeviceLabel = std::make_unique<juce::Label> (juce::String{}, type.hasSeparateInputsAndOutputs() ? TRANS ("Output:")
                                                                                                              : TRANS ("Device:"));
                    outputDeviceLabel->attachToComponent (outputDeviceDropDown.get(), true);

                    if (setup.maxNumOutputChannels > 0)
                    {
                        testButton = std::make_unique<juce::TextButton> (TRANS ("Test"), TRANS ("Plays a test tone"));
                        addAndMakeVisible (testButton.get());
                        testButton->onClick = [this] { playTestSound(); };
                    }
                }

                addNamesToDeviceBox (*outputDeviceDropDown, false);
            }

            updateSelectedOutput();
        }

        void updateInputsComboBox()
        {
            if (setup.maxNumInputChannels > 0 && type.hasSeparateInputsAndOutputs())
            {
                if (inputDeviceDropDown == nullptr)
                {
                    inputDeviceDropDown = std::make_unique<juce::ComboBox>();
                    inputDeviceDropDown->onChange = [this] { updateConfig (false, true, false, false); };
                    addAndMakeVisible (inputDeviceDropDown.get());

                    inputDeviceLabel = std::make_unique<juce::Label> (juce::String{}, TRANS ("Input:"));
                    inputDeviceLabel->attachToComponent (inputDeviceDropDown.get(), true);

                    inputLevelMeter = std::make_unique<SimpleDeviceManagerInputLevelMeter> (*setup.manager);
                    addAndMakeVisible (inputLevelMeter.get());
                }

                addNamesToDeviceBox (*inputDeviceDropDown, true);
            }

            updateSelectedInput();
        }

        void updateSampleRateComboBox (juce::AudioIODevice* currentDevice)
        {
            if (sampleRateDropDown == nullptr)
            {
                sampleRateDropDown = std::make_unique<juce::ComboBox>();
                addAndMakeVisible (sampleRateDropDown.get());

                sampleRateLabel = std::make_unique<juce::Label> (juce::String{}, TRANS ("Sample rate:"));
                sampleRateLabel->attachToComponent (sampleRateDropDown.get(), true);
            }
            else
            {
                sampleRateDropDown->clear();
                sampleRateDropDown->onChange = nullptr;
            }

            const auto getFrequencyString = [] (int rate) { return juce::String (rate) + " Hz"; };

            for (auto rate : currentDevice->getAvailableSampleRates())
            {
                const auto intRate = juce::roundToInt (rate);
                sampleRateDropDown->addItem (getFrequencyString (intRate), intRate);
            }

            const auto intRate = juce::roundToInt (currentDevice->getCurrentSampleRate());

            sampleRateDropDown->setText (getFrequencyString (intRate), juce::NotificationType::dontSendNotification);

            sampleRateDropDown->onChange = [this] { updateConfig (false, false, true, false); };
        }

        void updateBufferSizeComboBox (juce::AudioIODevice* currentDevice)
        {
            if (bufferSizeDropDown == nullptr)
            {
                bufferSizeDropDown = std::make_unique<juce::ComboBox>();
                addAndMakeVisible (bufferSizeDropDown.get());

                bufferSizeLabel = std::make_unique<juce::Label> (juce::String{}, TRANS ("Audio buffer size:"));
                bufferSizeLabel->attachToComponent (bufferSizeDropDown.get(), true);
            }
            else
            {
                bufferSizeDropDown->clear();
                bufferSizeDropDown->onChange = nullptr;
            }

            auto currentRate = currentDevice->getCurrentSampleRate();

            if (juce::exactlyEqual (currentRate, 0.0))
                currentRate = 48000.0;

            for (auto bs : currentDevice->getAvailableBufferSizes())
                bufferSizeDropDown->addItem (juce::String (bs) + " samples (" + juce::String (bs * 1000.0 / currentRate, 1) + " ms)", bs);

            bufferSizeDropDown->setSelectedId (currentDevice->getCurrentBufferSizeSamples(), juce::NotificationType::dontSendNotification);
            bufferSizeDropDown->onChange = [this] { updateConfig (false, false, false, true); };
        }

    public:
        //==============================================================================
        class ChannelSelectorListBox final : public juce::ListBox,
                                             private juce::ListBoxModel
        {
        public:
            enum BoxType
            {
                audioInputType,
                audioOutputType
            };

            //==============================================================================
            ChannelSelectorListBox (const AudioDeviceSetupDetails& setupDetails, BoxType boxType, const juce::String& noItemsText)
                    : juce::ListBox ({}, nullptr), setup (setupDetails), type (boxType), noItemsMessage (noItemsText)
            {
                refresh();
                setModel (this);
                setOutlineThickness (1);
            }

            void refresh()
            {
                items.clear();

                if (auto* currentDevice = setup.manager->getCurrentAudioDevice())
                {
                    if (type == audioInputType)
                        items = currentDevice->getInputChannelNames();
                    else if (type == audioOutputType)
                        items = currentDevice->getOutputChannelNames();

                    if (setup.useStereoPairs)
                    {
                        juce::StringArray pairs;

                        for (int i = 0; i < items.size(); i += 2)
                        {
                            auto& name = items[i];

                            if (i + 1 >= items.size())
                                pairs.add (name.trim());
                            else
                                pairs.add (getNameForChannelPair (name, items[i + 1]));
                        }

                        items = pairs;
                    }
                }

                updateContent();
                repaint();
            }

            int getNumRows() override
            {
                return items.size();
            }

            void paintListBoxItem (int row, juce::Graphics& g, int width, int height, bool) override
            {
                if (juce::isPositiveAndBelow (row, items.size()))
                {
                    g.fillAll (findColour (juce::ListBox::backgroundColourId));

                    auto item = items[row];
                    bool enabled = false;
                    auto config = setup.manager->getAudioDeviceSetup();

                    if (setup.useStereoPairs)
                    {
                        if (type == audioInputType)
                            enabled = config.inputChannels[row * 2] || config.inputChannels[row * 2 + 1];
                        else if (type == audioOutputType)
                            enabled = config.outputChannels[row * 2] || config.outputChannels[row * 2 + 1];
                    }
                    else
                    {
                        if (type == audioInputType)
                            enabled = config.inputChannels[row];
                        else if (type == audioOutputType)
                            enabled = config.outputChannels[row];
                    }

                    auto x = getTickX();
                    auto tickW = (float) height * 0.75f;

                    getLookAndFeel().drawTickBox (g, *this, (float) x - tickW, ((float) height - tickW) * 0.5f, tickW, tickW,
                                                  enabled, true, true, false);

                    drawTextLayout (g, *this, item, { x + 5, 0, width - x - 5, height }, enabled);
                }
            }

            void listBoxItemClicked (int row, const juce::MouseEvent& e) override
            {
                selectRow (row);

                if (e.x < getTickX())
                    flipEnablement (row);
            }

            void listBoxItemDoubleClicked (int row, const juce::MouseEvent&) override
            {
                flipEnablement (row);
            }

            void returnKeyPressed (int row) override
            {
                flipEnablement (row);
            }

            void paint (juce::Graphics& g) override
            {
                juce::ListBox::paint (g);

                if (items.isEmpty())
                {
                    g.setColour (juce::Colours::grey);
                    g.setFont (0.5f * (float) getRowHeight());
                    g.drawText (noItemsMessage,
                                0, 0, getWidth(), getHeight() / 2,
                                juce::Justification::centred, true);
                }
            }

            int getBestHeight (int maxHeight)
            {
                return getRowHeight() * juce::jlimit (2, juce::jmax (2, maxHeight / getRowHeight()),
                                                getNumRows())
                       + getOutlineThickness() * 2;
            }

        private:
            //==============================================================================
            const AudioDeviceSetupDetails setup;
            const BoxType type;
            const juce::String noItemsMessage;
            juce::StringArray items;

            static juce::String getNameForChannelPair (const juce::String& name1, const juce::String& name2)
            {
                juce::String commonBit;

                for (int j = 0; j < name1.length(); ++j)
                    if (name1.substring (0, j).equalsIgnoreCase (name2.substring (0, j)))
                        commonBit = name1.substring (0, j);

                // Make sure we only split the name at a space, because otherwise, things
                // like "input 11" + "input 12" would become "input 11 + 2"
                while (commonBit.isNotEmpty() && ! juce::CharacterFunctions::isWhitespace (commonBit.getLastCharacter()))
                    commonBit = commonBit.dropLastCharacters (1);

                return name1.trim() + " + " + name2.substring (commonBit.length()).trim();
            }

            void flipEnablement (int row)
            {
                jassert (type == audioInputType || type == audioOutputType);

                if (juce::isPositiveAndBelow (row, items.size()))
                {
                    auto config = setup.manager->getAudioDeviceSetup();

                    if (setup.useStereoPairs)
                    {
                        juce::BigInteger bits;
                        auto& original = (type == audioInputType ? config.inputChannels
                                                                 : config.outputChannels);

                        for (int i = 0; i < 256; i += 2)
                            bits.setBit (i / 2, original[i] || original[i + 1]);

                        if (type == audioInputType)
                        {
                            config.useDefaultInputChannels = false;
                            flipBit (bits, row, setup.minNumInputChannels / 2, setup.maxNumInputChannels / 2);
                        }
                        else
                        {
                            config.useDefaultOutputChannels = false;
                            flipBit (bits, row, setup.minNumOutputChannels / 2, setup.maxNumOutputChannels / 2);
                        }

                        for (int i = 0; i < 256; ++i)
                            original.setBit (i, bits[i / 2]);
                    }
                    else
                    {
                        if (type == audioInputType)
                        {
                            config.useDefaultInputChannels = false;
                            flipBit (config.inputChannels, row, setup.minNumInputChannels, setup.maxNumInputChannels);
                        }
                        else
                        {
                            config.useDefaultOutputChannels = false;
                            flipBit (config.outputChannels, row, setup.minNumOutputChannels, setup.maxNumOutputChannels);
                        }
                    }

                    setup.manager->setAudioDeviceSetup (config, true);
                }
            }

            static void flipBit (juce::BigInteger& chans, int index, int minNumber, int maxNumber)
            {
                auto numActive = chans.countNumberOfSetBits();

                if (chans[index])
                {
                    if (numActive > minNumber)
                        chans.setBit (index, false);
                }
                else
                {
                    if (numActive >= maxNumber)
                    {
                        auto firstActiveChan = chans.findNextSetBit (0);
                        chans.clearBit (index > firstActiveChan ? firstActiveChan : chans.getHighestBit());
                    }

                    chans.setBit (index, true);
                }
            }

            int getTickX() const
            {
                return getRowHeight();
            }

            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChannelSelectorListBox)
        };

    private:
        std::unique_ptr<ChannelSelectorListBox> inputChanList, outputChanList;
        juce::ScopedMessageBox messageBox;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioDeviceSettingsPanel)
    };


//==============================================================================
    AudioDeviceSelectorComponent::AudioDeviceSelectorComponent (juce::AudioDeviceManager& dm,
                                                                int minInputChannelsToUse,
                                                                int maxInputChannelsToUse,
                                                                int minOutputChannelsToUse,
                                                                int maxOutputChannelsToUse,
                                                                bool showMidiInputOptions,
                                                                bool showMidiOutputSelector,
                                                                bool showChannelsAsStereoPairsToUse,
                                                                bool hideAdvancedOptionsWithButtonToUse, juce::ValueTree tree)
            : deviceManager (dm),
              itemHeight (24),
              minOutputChannels (minOutputChannelsToUse),
              maxOutputChannels (maxOutputChannelsToUse),
              minInputChannels (minInputChannelsToUse),
              maxInputChannels (maxInputChannelsToUse),
              showChannelsAsStereoPairs (showChannelsAsStereoPairsToUse),
              hideAdvancedOptionsWithButton (hideAdvancedOptionsWithButtonToUse),
              state(tree)
    {
        jassert (minOutputChannels >= 0 && minOutputChannels <= maxOutputChannels);
        jassert (minInputChannels >= 0 && minInputChannels <= maxInputChannels);
        for (auto device : juce::MidiInput::getAvailableDevices())
        {
            if (auto _child = state.getChildWithName("midiPrefs").getChildWithProperty("name", device.name); _child.isValid())
            {
                auto enabled = _child.getProperty("active", 0);
                deviceManager.setMidiInputDeviceEnabled (device.identifier, enabled);
            }
            else
            {
                juce::ValueTree t(IDs::midiInput);
                t.setProperty(IDs::name, device.name, nullptr);
                t.setProperty(IDs::midiDeviceId, device.identifier, nullptr);
                t.setProperty(IDs::active, deviceManager.isMidiInputDeviceEnabled (device.identifier),nullptr);
                state.getChildWithName("midiPrefs").addChild(t, -1, nullptr);
                deviceManager.setMidiInputDeviceEnabled (device.identifier, 0);
            }
        }
        const juce::OwnedArray<juce::AudioIODeviceType>& types = deviceManager.getAvailableDeviceTypes();

        if (types.size() > 1)
        {
            deviceTypeDropDown = std::make_unique<juce::ComboBox>();

            for (int i = 0; i < types.size(); ++i)
                deviceTypeDropDown->addItem (types.getUnchecked (i)->getTypeName(), i + 1);

            addAndMakeVisible (deviceTypeDropDown.get());
            deviceTypeDropDown->onChange = [this] { updateDeviceType(); };

            deviceTypeDropDownLabel = std::make_unique<juce::Label> (juce::String{}, TRANS ("Audio device type:"));
            deviceTypeDropDownLabel->setJustificationType (juce::Justification::centredRight);
            deviceTypeDropDownLabel->attachToComponent (deviceTypeDropDown.get(), true);
        }

        if (showMidiInputOptions)
        {
            midiInputsList = std::make_unique <MidiInputSelectorComponentListBox> (deviceManager,
                                                                                   "(" + TRANS ("No MIDI inputs available") + ")");
            addAndMakeVisible (midiInputsList.get());

            midiInputsLabel = std::make_unique<juce::Label> (juce::String{}, TRANS ("Active MIDI inputs:"));
            midiInputsLabel->setJustificationType (juce::Justification::topRight);
            midiInputsLabel->attachToComponent (midiInputsList.get(), true);

            if (juce::BluetoothMidiDevicePairingDialogue::isAvailable())
            {
                bluetoothButton = std::make_unique<juce::TextButton> (TRANS ("Bluetooth MIDI"), TRANS ("Scan for bluetooth MIDI devices"));
                addAndMakeVisible (bluetoothButton.get());
                bluetoothButton->onClick = [this] { handleBluetoothButton(); };
            }
        }
        else
        {
            midiInputsList.reset();
            midiInputsLabel.reset();
            bluetoothButton.reset();
        }

        if (showMidiOutputSelector)
        {
            midiOutputSelector = std::make_unique<MidiOutputSelector> (deviceManager);
            addAndMakeVisible (midiOutputSelector.get());

            midiOutputLabel = std::make_unique<juce::Label> ("lm", TRANS ("MIDI Output:"));
            midiOutputLabel->attachToComponent (midiOutputSelector.get(), true);
        }
        else
        {
            midiOutputSelector.reset();
            midiOutputLabel.reset();
        }

        deviceManager.addChangeListener (this);
        updateAllControls();
    }

    AudioDeviceSelectorComponent::~AudioDeviceSelectorComponent()
    {
        deviceManager.removeChangeListener (this);
    }

    void AudioDeviceSelectorComponent::setItemHeight (int newItemHeight)
    {
        itemHeight = newItemHeight;
        resized();
    }

    void AudioDeviceSelectorComponent::resized()
    {
        juce::Rectangle<int> r (proportionOfWidth (0.35f), 15, proportionOfWidth (0.6f), 3000);
        auto space = itemHeight / 4;

        if (deviceTypeDropDown != nullptr)
        {
            deviceTypeDropDown->setBounds (r.removeFromTop (itemHeight));
            r.removeFromTop (space * 3);
        }

        if (audioDeviceSettingsComp != nullptr)
        {
            audioDeviceSettingsComp->resized();
            audioDeviceSettingsComp->setBounds (r.removeFromTop (audioDeviceSettingsComp->getHeight())
                                                        .withX (0).withWidth (getWidth()));
            r.removeFromTop (space);
        }

        if (midiInputsList != nullptr)
        {
            midiInputsList->setRowHeight (juce::jmin (22, itemHeight));
            midiInputsList->setBounds (r.removeFromTop (midiInputsList->getBestHeight (juce::jmin (itemHeight * 8,
                                                                                             getHeight() - r.getY() - space - itemHeight))));
            r.removeFromTop (space);
        }

        if (bluetoothButton != nullptr)
        {
            bluetoothButton->setBounds (r.removeFromTop (24));
            r.removeFromTop (space);
        }

        if (midiOutputSelector != nullptr)
            midiOutputSelector->setBounds (r.removeFromTop (itemHeight));

        r.removeFromTop (itemHeight);
        setSize (getWidth(), r.getY());
    }

    void AudioDeviceSelectorComponent::childBoundsChanged (juce::Component* child)
    {
        if (child == audioDeviceSettingsComp.get())
            resized();
    }

    void AudioDeviceSelectorComponent::updateDeviceType()
    {
        if (auto* type = deviceManager.getAvailableDeviceTypes() [deviceTypeDropDown->getSelectedId() - 1])
        {
            audioDeviceSettingsComp.reset();
            deviceManager.setCurrentAudioDeviceType (type->getTypeName(), true);
            updateAllControls(); // needed in case the type hasn't actually changed
        }
    }

    void AudioDeviceSelectorComponent::changeListenerCallback (juce::ChangeBroadcaster*)
    {
        updateAllControls();
    }

    void AudioDeviceSelectorComponent::updateAllControls()
    {
        if (deviceTypeDropDown != nullptr)
            deviceTypeDropDown->setText (deviceManager.getCurrentAudioDeviceType(), juce::NotificationType::dontSendNotification);

        if (audioDeviceSettingsComp == nullptr
            || audioDeviceSettingsCompType != deviceManager.getCurrentAudioDeviceType())
        {
            audioDeviceSettingsCompType = deviceManager.getCurrentAudioDeviceType();
            audioDeviceSettingsComp.reset();

            if (auto* type = deviceManager.getAvailableDeviceTypes() [deviceTypeDropDown == nullptr
                                                                      ? 0 : deviceTypeDropDown->getSelectedId() - 1])
            {
                AudioDeviceSetupDetails details;
                details.manager = &deviceManager;
                details.minNumInputChannels = minInputChannels;
                details.maxNumInputChannels = maxInputChannels;
                details.minNumOutputChannels = minOutputChannels;
                details.maxNumOutputChannels = maxOutputChannels;
                details.useStereoPairs = showChannelsAsStereoPairs;

                audioDeviceSettingsComp = std::make_unique<AudioDeviceSettingsPanel> (*type, details, hideAdvancedOptionsWithButton, *this);
                addAndMakeVisible (audioDeviceSettingsComp.get());
            }
        }

        if (midiInputsList != nullptr)
        {
            midiInputsList->updateDevices();
            midiInputsList->updateContent();
            midiInputsList->repaint();

            for (auto device : juce::MidiInput::getAvailableDevices())
            {
                if (auto _child = state.getChildWithName("midiPrefs").getChildWithProperty("midiDeviceId", device.identifier); _child.isValid())
                {
                   auto enabled =  deviceManager.isMidiInputDeviceEnabled (device.identifier);
                   _child.setProperty("active", enabled, nullptr);
                }
                else
                {
                    juce::ValueTree t(IDs::midiInput);
                    t.setProperty(IDs::name, device.name, nullptr);
                    t.setProperty(IDs::midiDeviceId, device.identifier, nullptr);
                    t.setProperty(IDs::active, deviceManager.isMidiInputDeviceEnabled (device.identifier),nullptr);
                    state.getChildWithName("midiPrefs").addChild(t, -1, nullptr);
                }
            }
        }

        resized();
    }

    void AudioDeviceSelectorComponent::handleBluetoothButton()
    {
        if (juce::RuntimePermissions::isGranted (juce::RuntimePermissions::bluetoothMidi))
        {
            juce::BluetoothMidiDevicePairingDialogue::open();
        }
        else
        {
            juce::RuntimePermissions::request (juce::RuntimePermissions::bluetoothMidi, [] (auto)
            {
                if (juce::RuntimePermissions::isGranted (juce::RuntimePermissions::bluetoothMidi))
                    juce::BluetoothMidiDevicePairingDialogue::open();
            });
        }
    }

    juce::ListBox* AudioDeviceSelectorComponent::getMidiInputSelectorListBox() const noexcept
    {
        return midiInputsList.get();
    }

} // namespace juce
