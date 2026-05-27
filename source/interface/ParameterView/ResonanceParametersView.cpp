// Copyright (C) 2022-2025 Dan Trueman
// SPDX-License-Identifier: GPL-3.0-or-later

//
// Created by Dan Trueman on 10/8/25.
//

#include "ResonanceParametersView.h"

void ResonanceParametersView::timerCallback(void)
{
    bool soloed    = sparams_.soloed_.load(std::memory_order_relaxed);
    bool userMuted = sparams_.userMuted_.load(std::memory_order_relaxed);
    bool soloMuted = sparams_.soloMuted_.load(std::memory_order_relaxed);
    soloButton_->setToggleState(soloed, juce::dontSendNotification);
    if (soloMuted && !userMuted) {
        bool blinkPhase = (juce::Time::getMillisecondCounter() / 300) % 2;
        muteButton_->setToggleState(blinkPhase, juce::dontSendNotification);
    } else {
        muteButton_->setToggleState(userMuted, juce::dontSendNotification);
    }

    /*
     * probably a more direct way to do this without a timer...
     */
    allNotesOffButton->setToggleState(false, juce::NotificationType::dontSendNotification);
    heldKeysKeyboard->redoImage();
    closestKeyboard->redoImage();
    fundamentalKeyboard->redoImage();
    offsetsKeyboard->redoImage();
    gainsKeyboard->redoImage();
}

void ResonanceParametersView::resized()
{
    // width of the title at left, used in all preparations
    int title_width = getTitleWidth();

    // scaled sizes
    auto knob_section_height = getKnobSectionHeight();
    auto menu_section_height = findValue(Skin::kComboMenuHeight);
    auto smallpadding = findValue(Skin::kPadding);
    auto largepadding = findValue(Skin::kLargePadding);
    auto knobLabelSize = findValue(Skin::kKnobLabelSizeMedium);

    juce::Rectangle<int> titleArea = getLocalBounds().removeFromLeft(getTitleWidth());
    prepTitle->setBounds(titleArea);
    prepTitle->setTextSize (findValue(Skin::kPrepTitleSize));

    // get the prep area, with left/right border for title
    juce::Rectangle<int> bounds = getLocalBounds();
    bounds.removeFromLeft(title_width);

    // reserve bottom strip for mute button before extracting meter areas
    const int muteButtonH = 18;
    auto muteRow = bounds.removeFromBottom(muteButtonH + smallpadding);

    // bounds for level meter on right side
    juce::Rectangle<int> meterArea = bounds.removeFromRight(title_width);
    meterArea.removeFromTop(8);      // push meters down a few pixels so label tops aren't clipped
    levelMeter->setBounds(meterArea);

    bounds.removeFromRight(smallpadding);
    juce::Rectangle<int> sendmeterArea = bounds.removeFromRight(title_width);
    sendmeterArea.removeFromTop(8);      // push meters down a few pixels so label tops aren't clipped
    sendLevelMeter->setBounds(sendmeterArea);

    // place M and S buttons spanning send and main meter columns
    {
        int muteLeft  = sendLevelMeter->getX();
        int muteRight = levelMeter->getRight();
        muteRow.setLeft(muteLeft);
        muteRow.setRight(muteRight);
        muteRow.removeFromTop(smallpadding);
        constexpr int kGap = 2;
        int halfW = (muteRow.getWidth() - kGap) / 2;
        muteButton_->setBounds(muteRow.removeFromLeft(halfW));
        muteRow.removeFromLeft(kGap);
        soloButton_->setBounds(muteRow);
    }

    //
    // *** done with meters placement section
    //

    fundamentalKeyboard_label->setTextSize (knobLabelSize);
    offsetsKeyboard_label->setTextSize (knobLabelSize);
    gainsKeyboard_label->setTextSize (knobLabelSize);
    closestKeyboard_label->setTextSize (knobLabelSize);

    heldKeysKeyboard->setBounds(bounds.removeFromBottom(120));

    // overtone structure keyboards
    juce::Rectangle<int> keyboardsRect = bounds.removeFromRight(bounds.getWidth() * 0.5);
    keyboardsRect.reduce(largepadding, largepadding);
    juce::Rectangle<int> spectrumMenuArea = keyboardsRect.removeFromTop(menu_section_height);
    //spectrumMenuArea.reduce(spectrumMenuArea.getWidth() * 0.25, 0);
    // spectrum_combo_box->setBounds(spectrumMenuArea);
    spectrum_combo_box->setBounds(spectrumMenuArea.removeFromLeft(spectrumMenuArea.getWidth() * 0.3));
    allNotesOffButton->setBounds(spectrumMenuArea.removeFromRight(spectrumMenuArea.getWidth() * 0.3));

    /*
     * todo: scale keyboard height; add to Skin
     */
    int keyboardHeight = 100;
    int labelsectionheight = findValue(Skin::kLabelHeight);
    int kpadding = (keyboardsRect.getHeight() - 4. * (keyboardHeight + labelsectionheight)) / 3.;

    fundamentalKeyboard->setBounds(keyboardsRect.removeFromBottom(keyboardHeight));
    fundamentalKeyboard->showLiveState = false;
    fundamentalKeyboard_label->setBounds(fundamentalKeyboard->getX(), fundamentalKeyboard->getY() + 4, fundamentalKeyboard->getWidth(), labelsectionheight);
    keyboardsRect.removeFromBottom(kpadding + labelsectionheight);

    closestKeyboard->setBounds(keyboardsRect.removeFromBottom(keyboardHeight));
    closestKeyboard->showLiveState = false;
    closestKeyboard_label->setBounds(closestKeyboard->getX(), closestKeyboard->getY() + 4, closestKeyboard->getWidth(), labelsectionheight);
    keyboardsRect.removeFromBottom(kpadding + labelsectionheight);

    offsetsKeyboard->setBounds(keyboardsRect.removeFromBottom(keyboardHeight));
    offsetsKeyboard_label->setBounds(offsetsKeyboard->getX(), offsetsKeyboard->getY() + 4, offsetsKeyboard->getWidth(), labelsectionheight);
    keyboardsRect.removeFromBottom(kpadding + labelsectionheight);

    gainsKeyboard->setBounds(keyboardsRect.removeFromBottom(keyboardHeight));
    gainsKeyboard_label->setBounds(gainsKeyboard->getX(), gainsKeyboard->getY() + 4, gainsKeyboard->getWidth(), labelsectionheight);

    // adsr
    bounds.reduce(largepadding, largepadding);
    juce::Rectangle<int> envRect = bounds.removeFromBottom(bounds.getHeight() * 0.5);
    envSection->setBounds(envRect);

    // knobs
    bounds.removeFromBottom(largepadding);
    variousControlsBorder->setBounds(bounds);
    auto reduceBy = bounds.getHeight() - (knob_section_height * 2.0 + largepadding);
    bounds.reduce(0, reduceBy * 0.5);

    //juce::Rectangle<int> outputKnobsArea = bounds.removeFromBottom(knob_section_height);
    placeKnobsInArea(bounds.removeFromBottom(knob_section_height), _sliders_row1, false);
    bounds.removeFromBottom(largepadding);
    placeKnobsInArea(bounds.removeFromBottom(knob_section_height), _sliders_row2, false);

    int sl_counter = 0;
    for (auto& slider : _sliders_row1)
    {
        juce::Rectangle<int> sl_label_rect (slider->getX(), slider->getBottom() - 10, slider->getWidth(), labelsectionheight );
        slider_labels[sl_counter++]->setBounds(sl_label_rect);
    }
    for (auto& slider : _sliders_row2)
    {
        juce::Rectangle<int> sl_label_rect (slider->getX(), slider->getBottom() - 10, slider->getWidth(), labelsectionheight );
        slider_labels[sl_counter++]->setBounds(sl_label_rect);
    }

    SynthSection::resized();
}