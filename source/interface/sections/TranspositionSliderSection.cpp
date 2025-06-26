//
// Created by Dan Trueman on 6/19/25.
//

#include "TranspositionSliderSection.h"

void TranspositionSliderSection::resized()
{
    juce::Rectangle<int> bounds = getLocalBounds();
    slider->setBounds(bounds);
    slider->redoImage();

    int onWidth = 100;
    int onHeight = 20;
    on->setBounds(getWidth() - onWidth - 4, getHeight() - onHeight - 4, onWidth, onHeight);
    on->toFront(false);

    SynthSection::resized();
}