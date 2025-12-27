//
// Created by Myra Norton on 11/14/25.
//

#include "EQParameterView.h"

void EQParameterView::resized()
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

    auto activeBounds = juce::Rectangle<int>(bounds.getWidth()-40, 0, 65, 20);
    auto resetBounds = juce::Rectangle<int>(bounds.getWidth()-40, 30, 65, 20);
    activeEq_toggle->setBounds (activeBounds);
    reset_button->setBounds(resetBounds);
    bounds.removeFromTop (60);

    inLevelMeter->setBounds(bounds.removeFromLeft(title_width));

    bounds.removeFromRight(title_width*.85);
    levelMeter->setBounds(bounds.removeFromRight(title_width));

    // bounds.removeFromRight(smallpadding);
    // sendLevelMeter->setBounds(bounds.removeFromRight(title_width));

    bounds.reduce(largepadding, largepadding);

    bounds.expand (0,50);
    bounds.removeFromBottom (50);
    bounds.removeFromLeft (title_width);
    bounds.removeFromRight (title_width);
    auto topHalf = bounds.removeFromTop (bounds.getHeight()/2 - 20);
    equalizerGraph->setBounds(topHalf);
    bounds.removeFromTop (20);
    // bounds.removeFromBottom (20);

    // Padding between columns
    constexpr int colPadding = 30;

    // Total width available
    const int totalWidth = bounds.getWidth();
    const int colWidth = (totalWidth - 4 * colPadding) / 5;

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