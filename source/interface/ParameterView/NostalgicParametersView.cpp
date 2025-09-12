//
// Created by Myra Norton on 8/20/25.
//

#include "NostalgicParametersView.h"

void NostalgicParametersView::resized()
{
    // width of the title at left, used in all preparations
    int title_width = getTitleWidth();
    int smallpadding = findValue(Skin::kPadding);
    int largepadding = findValue(Skin::kLargePadding);

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
    // juce::Rectangle<int> waveArea = bounds.removeFromBottom(Skin::kWavetableWaveHeight);
    // waveSection->setBounds(waveArea);
    bounds.removeFromBottom(Skin::kKnobSectionHeight*8);

    // left column for transposition slider, hold time slider, and reverse adsr
    juce::Rectangle<int> leftColumn = bounds.removeFromLeft(bounds.getWidth() / 2);
    leftColumn.removeFromRight (smallpadding);
    reverseEnvSection->setBounds(leftColumn.removeFromBottom (Skin::kKnobSectionHeight*8));
    leftColumn.removeFromBottom (largepadding);
    holdTimeMinMaxSlider->setBounds(leftColumn.removeFromBottom (Skin::kKnobSectionHeight*3));
    leftColumn.removeFromBottom (largepadding);
    leftColumn.removeFromTop (Skin::kKnobSectionHeight);
    clusterMin_knob->setBounds(leftColumn.removeFromRight(leftColumn.getWidth() / 3).reduced(largepadding, 0));
    clusterThreshold_knob->setBounds(leftColumn.removeFromRight(leftColumn.getWidth() / 2).reduced(largepadding, 0));
    noteLengthMult_knob->setBounds(leftColumn.reduced(largepadding, 0));

    // right column for knobs and udnertow adsr
    bounds.removeFromLeft (smallpadding);
    undertowEnvSection->setBounds(bounds.removeFromBottom (Skin::kKnobSectionHeight*8));
    bounds.removeFromBottom (largepadding);
    transpositionSlider->setBounds(bounds.removeFromBottom(Skin::kKnobSectionHeight*3));



    SynthSection::resized();
}