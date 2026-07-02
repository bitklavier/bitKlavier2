// Copyright (C) 2022-2025 Dan Trueman
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ChucKlavierParametersView.h"

void ChucKlavierParametersView::resized()
{
    const int titleWidth    = getTitleWidth();
    const int smallPadding  = findValue (Skin::kPadding);
    const int largePadding  = findValue (Skin::kLargePadding);
    const int buttonHeight  = 22;

    juce::Rectangle<int> titleArea = getLocalBounds().removeFromLeft (titleWidth);
    prepTitle->setBounds (titleArea);
    prepTitle->setTextSize (findValue (Skin::kPrepTitleSize));

    juce::Rectangle<int> bounds = getLocalBounds();
    bounds.removeFromLeft (titleWidth);
    bounds.removeFromLeft (smallPadding);
    bounds.removeFromTop (8);
    bounds.removeFromBottom (20);

    // Left group: External then Internal meters
    externalLevelMeter->setBounds (bounds.removeFromLeft (titleWidth));
    bounds.removeFromLeft (smallPadding);
    inLevelMeter->setBounds (bounds.removeFromLeft (titleWidth));

    // Right group: Main then Send meters
    juce::Rectangle<int> meterArea = bounds.removeFromRight (titleWidth);
    levelMeter->setBounds (meterArea);
    bounds.removeFromRight (smallPadding);
    sendLevelMeter->setBounds (bounds.removeFromRight (titleWidth));

    // M/S buttons below right meters
    {
        int muteLeft  = sendLevelMeter->getX();
        int muteRight = levelMeter->getRight();
        int muteTop   = levelMeter->getBottom() + smallPadding;
        int muteH     = getLocalBounds().getBottom() - muteTop - smallPadding;
        if (muteH > 0)
        {
            constexpr int kGap = 2;
            int totalW = muteRight - muteLeft;
            int halfW  = (totalW - kGap) / 2;
            muteButton_->setBounds (muteLeft, muteTop, halfW, muteH);
            soloButton_->setBounds (muteLeft + halfW + kGap, muteTop, totalW - halfW - kGap, muteH);
        }
    }

    // Remaining area: script editor + Send button
    bounds.reduce (largePadding, largePadding);

    // Send to VM button at the bottom
    auto buttonArea = bounds.removeFromBottom (buttonHeight);
    sendScriptButton->setBounds (buttonArea);
    bounds.removeFromBottom (smallPadding);

    // Script editor fills the rest
    if (bounds.getHeight() > 0)
        scriptEditor->setBounds (bounds);

    SynthSection::resized();
}
