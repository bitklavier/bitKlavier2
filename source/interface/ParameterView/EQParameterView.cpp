// Copyright (C) 2022-2025 Dan Trueman
// SPDX-License-Identifier: GPL-3.0-or-later

//
// Created by Myra Norton on 11/14/25.
//

#include "EQParameterView.h"
#include "FullInterface.h"

void EQParameterView::resized()
{
    if (auto* parent = findParentComponentOfClass<FullInterface>())
        parent->hideSoundsetSelector();

    // width of the title at left, used in all preparations
    int title_width = getTitleWidth();

    // height for most of these components
    int knobsectionheight = findValue(Skin::kKnobSectionHeight);

    int smallpadding = findValue(Skin::kPadding);
    int largepadding = findValue(Skin::kLargePadding);

    juce::Rectangle<int> titleArea = getLocalBounds().removeFromLeft(title_width);
    prepTitle->setBounds(titleArea);
    prepTitle->setTextSize (findValue(Skin::kPrepTitleSize));

    // get the prep area, with left/right border for title
    juce::Rectangle<int> bounds = getLocalBounds();
    bounds.removeFromLeft(title_width);
    bounds.removeFromLeft(smallpadding);

    // auto activeBounds = juce::Rectangle<int>(bounds.getWidth()-40, 0, 65, 20);
    // auto resetBounds = juce::Rectangle<int>(bounds.getWidth()-40, 30, 65, 20);
    // activeEq_toggle->setBounds (activeBounds);
    // reset_button->setBounds(resetBounds);
    //bounds.removeFromTop (60);

    const int muteButtonH = 18;
    juce::Rectangle<int> muteRow;
    if (isPrepVersion_)
        muteRow = bounds.removeFromBottom(muteButtonH + smallpadding);

    if (isPrepVersion_ && externalLevelMeter)
        externalLevelMeter->setBounds(bounds.removeFromLeft(title_width));
    inLevelMeter->setBounds(bounds.removeFromLeft(title_width));
    bounds.removeFromRight(int(title_width * 0.85));
    levelMeter->setBounds(bounds.removeFromRight(title_width));
    if (isPrepVersion_ && sendLevelMeter)
        sendLevelMeter->setBounds(bounds.removeFromRight(title_width));

    if (isPrepVersion_ && muteButton_ && soloButton_ && sendLevelMeter)
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

    bounds.reduce(largepadding, largepadding);

    auto presets_rect = bounds.removeFromTop (knobsectionheight * 1.);
    presets_rect.reduce(presets_rect.getWidth() / 5., 0);
    presetsBorder->setBounds (presets_rect);
    presets_rect.reduce(largepadding, largepadding * 1.5);
    presets_rect.removeFromTop (largepadding);

    const int btnGap = 4;
    const int btnW   = (presets_rect.getWidth() - btnGap) / 2;
    activeEq_toggle->setBounds (presets_rect.removeFromLeft (btnW));
    presets_rect.removeFromLeft (btnGap);
    presetsButton->setBounds (presets_rect);

    bounds.removeFromTop (largepadding);
    //bounds.removeFromBottom (largepadding);
    bounds.removeFromLeft (title_width);
    bounds.removeFromRight (title_width);
    equalizerGraph->setBounds(bounds.removeFromTop (bounds.getHeight() / 2.5));
    bounds.removeFromTop (largepadding);

    // Padding between columns
    int colPadding = largepadding * 3;

    // Total width available
    int totalWidth = bounds.getWidth();
    int colWidth = (totalWidth - 4 * colPadding) / 5;

    loCutSection->setBounds(bounds.removeFromLeft(colWidth));
    bounds.removeFromLeft(colPadding);
    peak1Section->setBounds(bounds.removeFromLeft(colWidth));
    bounds.removeFromLeft(colPadding);
    peak2Section->setBounds(bounds.removeFromLeft(colWidth));
    bounds.removeFromLeft(colPadding);
    peak3Section->setBounds(bounds.removeFromLeft(colWidth));
    bounds.removeFromLeft(colPadding);
    hiCutSection->setBounds(bounds.removeFromLeft(colWidth));

    SynthSection::resized();
}