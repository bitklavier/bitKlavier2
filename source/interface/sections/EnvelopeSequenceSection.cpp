// Copyright (C) 2022-2025 Dan Trueman
// SPDX-License-Identifier: GPL-3.0-or-later

//
// Created by Dan Trueman on 8/8/25.
//

#include "EnvelopeSequenceSection.h"

EnvelopeSequenceSection::EnvelopeSequenceSection (
    juce::String name,
    EnvelopeSequenceParams &params,
    chowdsp::ParameterListeners &listeners,
    SynthSection &parent) : SynthSection(name), _params(params)
{
    for ( auto &param_ : *params.getBoolParams())
    {
        if(param_->paramID.startsWith("envelope")){
            auto button = std::make_unique<SynthButton>(param_->paramID);
            auto button_ToggleAttachment = std::make_unique<chowdsp::ButtonAttachment>(param_, listeners, *button, nullptr);
            button->setComponentID(param_->paramID);
            addSynthButton(button.get(), true);
            button->setButtonText(juce::String(getEnvelopeIndex(param_->paramID) + 1)); // +1 for more human-centered labeling!
            button->setColour(juce::TextButton::buttonOnColourId, juce::Colours::goldenrod);
            button->setHelpText("activate this envelope");

            _envActiveButtons_toggleAttachments.emplace_back(std::move(button_ToggleAttachment));
            _envActiveButtons.emplace_back(std::move(button));

            auto editbutton = std::make_unique<SynthButton>("edit_" + param_->paramID);
            editbutton->setPowerButton();
            editbutton->setColour(juce::TextButton::buttonOnColourId, juce::Colours::goldenrod);
            addSynthButton(editbutton.get(), true);
            _envEditButtons.emplace_back(std::move(editbutton));

            auto playingbutton = std::make_unique<SynthButton>("playing_" + param_->paramID);
            playingbutton->setPowerButton();
            playingbutton->setInterceptsMouseClicks(false, false); // user should not be able to interact with these
            addSynthButton(playingbutton.get(), true);
            _envPlayingButtons.emplace_back(std::move(playingbutton));
        }
    }

    // turn on the edit button for the most recently edited envelope (one edit button should be on at all times)
    auto currentEnvelopeBeingEdited = static_cast<int>(_params.currentlyEditing->getCurrentValue());
    _envEditButtons[currentEnvelopeBeingEdited]->setToggleState(true, juce::NotificationType::dontSendNotification);

    envelopeBorder = std::make_shared<OpenGL_LabeledBorder>("envelope border", "Envelope Sequence");
    addBorder(envelopeBorder.get());

    _currentLabel = std::make_shared<PlainTextComponent>("env_current_label", "current");
    _currentLabel->setJustification(juce::Justification::centredRight);
    addOpenGlComponent(_currentLabel);

    _activeLabel = std::make_shared<PlainTextComponent>("env_active_label", "active");
    _activeLabel->setJustification(juce::Justification::centredRight);
    addOpenGlComponent(_activeLabel);

    _editingLabel = std::make_shared<PlainTextComponent>("env_editing_label", "editing");
    _editingLabel->setJustification(juce::Justification::centredRight);
    addOpenGlComponent(_editingLabel);
}

int EnvelopeSequenceSection::getEnvelopeIndex(const juce::String& s)
{
    return s.fromLastOccurrenceOf("e", false, false).getIntValue();
    //return s.fromLastOccurrenceOf("_", false, false).getIntValue();
}

void EnvelopeSequenceSection::buttonClicked (juce::Button* clicked_button)
{
    if(clicked_button->getName().startsWith("edit_"))
    {
        for (auto & _b : _envEditButtons)
        {
            if(clicked_button->getName() != _b->getName())
            {
                //turn off other buttons: can only edit one button at a time
                _b->setToggleState(false, juce::NotificationType::dontSendNotification);
            }
            else
            {
                //can't turn off the current one; will only be turned off if another is selected
                if(!clicked_button->getToggleState())
                {
                    clicked_button->setToggleState(true, juce::NotificationType::dontSendNotification);
                }

                _params.currentlyEditing->setParameterValue(getEnvelopeIndex(_b->getName()));
            }
        }
        return; // skip the rest
    }

    bool noneActive = true;
    for (auto& button : _envActiveButtons)
    {
        if (button->getToggleState())
            noneActive = false;
    }

    if (noneActive)
    {
        // need to make sure at least one is active!
        DBG("no buttons active, keeping this one on!");
        clicked_button->setToggleState(true, juce::NotificationType::sendNotification);
    }
}

void EnvelopeSequenceSection::setCurrentlyPlayingEnvelope(int which)
{
    _envPlayingButtons[which]->setToggleState(true, juce::NotificationType::dontSendNotification);
    for (auto & _b : _envPlayingButtons)
    {
        if(getEnvelopeIndex(_b->getName()) != which)
        {
            _b->setToggleState(false, juce::NotificationType::dontSendNotification);
        }
        else
        {
            _b->setToggleState(true, juce::NotificationType::dontSendNotification);
        }
    }
}

void EnvelopeSequenceSection::paintBackground(juce::Graphics& g) {

    setLabelFont(g);
    paintKnobShadows(g);
    paintChildrenBackgrounds(g);
}

void EnvelopeSequenceSection::resized() {

    juce::Rectangle<int> area (getLocalBounds());

    int smallpadding = findValue(Skin::kPadding);

    envelopeBorder->setBounds(area);

    area.removeFromTop(17);
    area.reduce(10, 0);

    constexpr int labelWidth = 80;
    auto labelColumn = area.removeFromLeft(labelWidth);

    float labelTextSize = findValue(Skin::kButtonFontSize);
    _currentLabel->setTextSize(labelTextSize);
    _activeLabel->setTextSize(labelTextSize);
    _editingLabel->setTextSize(labelTextSize);

    int heightSave = area.getHeight();
    int envButtonWidth = area.getWidth() / (int)_envActiveButtons.size();

    juce::Rectangle displayButtonsRow = area.removeFromTop(heightSave / 4);
    _currentLabel->setBounds(labelColumn.removeFromTop(displayButtonsRow.getHeight()));
    for (auto& _b : _envPlayingButtons)
    {
        _b->setBounds(displayButtonsRow.removeFromLeft(envButtonWidth));
    }

    juce::Rectangle activeButtonRow = area.removeFromTop(heightSave / 2);
    _activeLabel->setBounds(labelColumn.removeFromTop(activeButtonRow.getHeight()));
    for (auto& _b : _envActiveButtons)
    {
        _b->setBounds(activeButtonRow.removeFromLeft(envButtonWidth).reduced(smallpadding));
    }

    _editingLabel->setBounds(labelColumn);
    for (auto& _b : _envEditButtons)
    {
        _b->setBounds(area.removeFromLeft(envButtonWidth));
    }

    SynthSection::resized();
}
