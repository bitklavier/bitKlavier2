//
// Created by Myra Norton on 8/20/25.
//

#include "NostalgicParametersView.h"

void NostalgicParametersView::timerCallback()
{
    waveSlider->updateSliderPositionsGL (nparams_.waveDistUndertowParams.displaySliderPositions);
}

void NostalgicParametersView::resized()
{
    // width of the title at left, used in all preparations
    int title_width = getTitleWidth();
    int smallpadding = findValue(Skin::kPadding);
    int largepadding = findValue(Skin::kLargePadding);
    int labelsectionheight = findValue(Skin::kLabelHeight);

    juce::Rectangle<int> titleArea = getLocalBounds().removeFromLeft(title_width);
    prepTitle->setBounds(titleArea);

    // get the prep area, with left/right border for title
    juce::Rectangle<int> bounds = getLocalBounds();
    bounds.removeFromLeft(title_width);

    // bounds for level meter on right side
    juce::Rectangle<int> meterArea = bounds.removeFromRight(title_width);
    levelMeter->setBounds(meterArea);
    bounds.removeFromRight(smallpadding);
    sendLevelMeter->setBounds(bounds.removeFromRight(title_width));
    bounds.removeFromLeft(largepadding);
    bounds.removeFromRight(largepadding);

    // wave section goes on the bottom
    waveSlider->setBounds(bounds.removeFromBottom (Skin::kKnobSectionHeight*8));

    // left column for transposition slider, hold time slider, and reverse adsr
    juce::Rectangle<int> leftColumn = bounds.removeFromLeft(bounds.getWidth() / 2);
    leftColumn.removeFromRight (smallpadding);
    reverseEnvSection->setBounds(leftColumn.removeFromBottom (Skin::kKnobSectionHeight*8));
    leftColumn.removeFromBottom (largepadding);
    holdTimeMinMaxSlider->setBounds(leftColumn.removeFromBottom (Skin::kKnobSectionHeight*3));

    // knobs and knob labels
    leftColumn.removeFromBottom (largepadding);
    leftColumn.removeFromTop (Skin::kKnobSectionHeight);
    placeKnobsInArea(leftColumn, { clusterMin_knob.get(), clusterThreshold_knob.get(), noteLengthMult_knob.get() }, false);
    beatsToSkip_knob->setBounds(noteLengthMult_knob->getBounds());
    juce::Rectangle<int> cm_label_rect (clusterMin_knob->getX(), clusterMin_knob->getBottom() - 10, clusterMin_knob->getWidth(), labelsectionheight );
    clusterMin_knob_label->setBounds(cm_label_rect);
    juce::Rectangle<int> cts_label_rect (clusterThreshold_knob->getX(), clusterThreshold_knob->getBottom() - 10, clusterThreshold_knob->getWidth(), labelsectionheight );
    clusterThreshold_knob_label->setBounds(cts_label_rect);
    juce::Rectangle<int> nlm_label_rect (noteLengthMult_knob->getX(), noteLengthMult_knob->getBottom() - 10, noteLengthMult_knob->getWidth(), labelsectionheight );
    noteLengthMult_knob_label->setBounds(nlm_label_rect);
    juce::Rectangle<int> bts_label_rect (beatsToSkip_knob->getX(), beatsToSkip_knob->getBottom() - 10, beatsToSkip_knob->getWidth(), labelsectionheight );
    beatsToSkip_knob_label->setBounds(bts_label_rect);

    // right column for knobs and undertow adsr
    bounds.removeFromLeft (smallpadding);
    undertowEnvSection->setBounds(bounds.removeFromBottom (Skin::kKnobSectionHeight*8));
    bounds.removeFromBottom (largepadding);
    transpositionSlider->setBounds(bounds.removeFromBottom(Skin::kKnobSectionHeight*3));
    bounds.removeFromBottom (largepadding);
    nostalgicTriggeredBy_combo_box->setBounds(bounds.removeFromRight(bounds.getWidth() / 3).removeFromBottom (Skin::kComboMenuHeight));
    keyOnReset->setBounds(bounds.removeFromRight(bounds.getWidth() / 3).removeFromBottom (Skin::kComboMenuHeight));

    SynthSection::resized();
}