//
// Created by Myra Norton on 11/14/25.
//

#include "CompressorParameterView.h"

void CompressorParameterView::resized()
{
    // width of the title at left, used in all preparations
    int title_width = getTitleWidth();

    // get the prep area, with left/right border for title
    juce::Rectangle<int> bounds = getLocalBounds();
    bounds.removeFromLeft(title_width);

    SynthSection::resized();
}