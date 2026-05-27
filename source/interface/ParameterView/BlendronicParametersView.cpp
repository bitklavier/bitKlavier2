// Copyright (C) 2022-2025 Dan Trueman
// SPDX-License-Identifier: GPL-3.0-or-later

//
// Created by Dan Trueman on 7/26/25.
//

#include "BlendronicParametersView.h"

void BlendronicParametersView::timerCallback()
{
    bool soloed    = bparams_.soloed_.load(std::memory_order_relaxed);
    bool userMuted = bparams_.userMuted_.load(std::memory_order_relaxed);
    bool soloMuted = bparams_.soloMuted_.load(std::memory_order_relaxed);
    soloButton_->setToggleState(soloed, juce::dontSendNotification);
    if (soloMuted && !userMuted) {
        bool blinkPhase = (juce::Time::getMillisecondCounter() / 300) % 2;
        muteButton_->setToggleState(blinkPhase, juce::dontSendNotification);
    } else {
        muteButton_->setToggleState(userMuted, juce::dontSendNotification);
    }

    /*
     * for updating the currently active sliders in the UI
     */

    // feedback is the only param where we don't want a default of 1, so need to update here and in BlendronicProcessor constructor
    if (feedbackCoeffsSlider->getAllActiveValues()[0][0] != bparams_.feedbackCoeffs.sliderVals[0].load())
    {
        DBG("updating slider value to match internal val");
        feedbackCoeffsSlider->updateSliderVal (0, 0, bparams_.feedbackCoeffs.sliderVals[0].load());
        bparams_.feedbackCoeffs_current = bparams_.feedbackCoeffs.sliderVals[0].load();
        feedbackCoeffsSlider->drawSliders(juce::dontSendNotification);
        feedbackCoeffsSlider->redoImage();
    }

    if(!beatLengthsSlider->hovering_)
    {
        beatLengthsSlider->setCurrentSlider(bparams_.beatLengths_current.load());
        beatLengthsSlider->redoImage();
    }
    if(!delayLengthsSlider->hovering_)
    {
        delayLengthsSlider->setCurrentSlider (bparams_.delayLengths_current.load());
        delayLengthsSlider->redoImage();
    }
    if(!smoothingTimesSlider->hovering_)
    {
        smoothingTimesSlider->setCurrentSlider (bparams_.smoothingTimes_current.load());
        smoothingTimesSlider->redoImage();
    }
    if(!feedbackCoeffsSlider->hovering_)
    {
        feedbackCoeffsSlider->setCurrentSlider (bparams_.feedbackCoeffs_current.load());
        feedbackCoeffsSlider->redoImage();
    }
}

void BlendronicParametersView::resized()
{
    // width of the title at left, used in all preparations
    int title_width = getTitleWidth();

    // height for most of these components
    int knob_section_height = getKnobSectionHeight();

    int smallpadding = findValue(Skin::kPadding);
    int largepadding = findValue(Skin::kLargePadding);

    juce::Rectangle<int> titleArea = getLocalBounds().removeFromLeft(title_width);
    prepTitle->setBounds(titleArea);
    prepTitle->setTextSize (findValue(Skin::kPrepTitleSize));

    // get the prep area, with left/right border for title
    juce::Rectangle<int> bounds = getLocalBounds();
    bounds.removeFromLeft(title_width);
    bounds.removeFromLeft(smallpadding);
    bounds.removeFromTop(8);      // push meters down a few pixels so label tops aren't clipped
    bounds.removeFromBottom(20);  // shorten all meters (and multisliders) by 20px

    // left group: Internal then External meters
    externalLevelMeter->setBounds(bounds.removeFromLeft(title_width));
    bounds.removeFromLeft(smallpadding);
    inLevelMeter->setBounds(bounds.removeFromLeft(title_width));

    // right group: Main then Send meters
    juce::Rectangle<int> meterArea = bounds.removeFromRight(title_width);
    levelMeter->setBounds(meterArea);

    bounds.removeFromRight(smallpadding);
    sendLevelMeter->setBounds(bounds.removeFromRight(title_width));

    // M and S buttons below Send and Main meters, spanning their combined width
    {
        int muteLeft  = sendLevelMeter->getX();
        int muteRight = levelMeter->getRight();
        int muteTop   = levelMeter->getBottom() + smallpadding;
        int muteH     = getLocalBounds().getBottom() - muteTop - smallpadding;
        if (muteH > 0) {
            constexpr int kGap = 2;
            int totalW = muteRight - muteLeft;
            int halfW  = (totalW - kGap) / 2;
            muteButton_->setBounds(muteLeft, muteTop, halfW, muteH);
            soloButton_->setBounds(muteLeft + halfW + kGap, muteTop, totalW - halfW - kGap, muteH);
        }
    }

    bounds.reduce(largepadding, largepadding);

    // how much vertical space will we need for all the components?
    float sliderHeightScale = 2.;
    int verticalAreaNeeded = knob_section_height * 4 * sliderHeightScale;

    /*
     * todo: better spacing for these...
     */
    // how much vertical space is left, divided up so we have some buffer space between each component
    int bufferSpaceForEach = (bounds.getHeight() - verticalAreaNeeded) / 5;
    if (bufferSpaceForEach < 0 ) bufferSpaceForEach = 0;

    bounds.removeFromTop(bufferSpaceForEach);

    beatLengthsSlider->setBounds(bounds.removeFromTop(knob_section_height * sliderHeightScale));
    bounds.removeFromTop(bufferSpaceForEach);
    delayLengthsSlider->setBounds(bounds.removeFromTop(knob_section_height * sliderHeightScale));
    bounds.removeFromTop(bufferSpaceForEach);
    smoothingTimesSlider->setBounds(bounds.removeFromTop(knob_section_height * sliderHeightScale));
    bounds.removeFromTop(bufferSpaceForEach);
    feedbackCoeffsSlider->setBounds(bounds.removeFromTop(knob_section_height * sliderHeightScale));

    SynthSection::resized();
}