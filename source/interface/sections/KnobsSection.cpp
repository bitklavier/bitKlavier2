//
// Created by Dan Trueman on 6/26/25.
//

#include "KnobsSection.h"

void KnobsSection::resized()
{
    juce::Rectangle<int> bounds = getLocalBounds();
    placeKnobsInArea(bounds, _sliders, true);
    knobsBorder.setBounds(bounds);

    SynthSection::resized();
}