//
// Created by Myra Norton on 11/14/25.
//

#include "EQParameterView.h"

void EQParameterView::resized()
{
    // width of the title at left, used in all preparations
    int title_width = getTitleWidth();

    // get the prep area, with left/right border for title
    juce::Rectangle<int> bounds = getLocalBounds();
    bounds.removeFromLeft(title_width);

    auto activeBounds = juce::Rectangle<int>(bounds.getWidth()-53, 0, 80, 80);
    auto resetBounds = juce::Rectangle<int>(bounds.getWidth()-45, 70, 65, 20);
    // Remove left/right padding
    constexpr int sidePadding = 100;
    bounds.removeFromLeft(sidePadding);
    bounds.removeFromRight(sidePadding);

    auto topHalf = bounds.removeFromTop (bounds.getHeight()/2);
    // equalizerGraph->setBounds(topHalf);
    activeEq_toggle->setBounds (activeBounds);
    reset_button->setBounds(resetBounds);

    bounds.removeFromBottom (20);

    // Padding between columns
    constexpr int colPadding = 50;

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