//
// Created by Dan Trueman on 11/5/24.
//

#include "DirectParametersView.h"

void DirectParametersView::resized()
{
    // width of the title at left, used in all preparations
    int title_width = getTitleWidth();

    // height for most of these components
    int knob_section_height = getKnobSectionHeight();

    // get the prep area, with left/right border for title
    //juce::Rectangle<int> bounds = getLocalBounds().withLeft(title_width);
    juce::Rectangle<int> bounds = getLocalBounds();
    bounds.reduce(title_width + 4, 0);

    // how much vertical space will we need for all the components?
    int verticalAreaNeeded = knob_section_height * 7;

    // how much vertical space is left, divided up so we have some buffer space between each component
    int bufferSpaceForEach = (bounds.getHeight() - verticalAreaNeeded) / 6;
    if (bufferSpaceForEach < 0 ) bufferSpaceForEach = 0;

    // start at the top, add the output knobs (main gain, hammers, resonance, etc..., and send)
    bounds.removeFromTop(bufferSpaceForEach);
    juce::Rectangle<int> outputKnobsArea = bounds.removeFromTop(knob_section_height);
    knobsBorder.setBounds(outputKnobsArea);
    placeKnobsInArea(outputKnobsArea, _sliders, true);

    // add the adsr below that
    bounds.removeFromTop(bufferSpaceForEach);
    juce::Rectangle<int> adsrArea = bounds.removeFromTop(knob_section_height * 4);
    envSection->setBounds(adsrArea);

    // add the transposition slider below that
    bounds.removeFromTop(bufferSpaceForEach);
    juce::Rectangle<int> transpositionSliderArea = bounds.removeFromTop(knob_section_height);
    transpositionSliderArea.reduce(transpositionSliderArea.getWidth() / 6, 0);
    transpositionSlider->setBounds(transpositionSliderArea);

    // add the velocity range slider below that
    bounds.removeFromTop(bufferSpaceForEach * 2);
    juce::Rectangle<int> velocitySliderArea = bounds.removeFromTop(knob_section_height);
    velocitySliderArea.reduce(velocitySliderArea.getWidth() / 4, 0); //narrow slightly; don't need the full width for this one!
    velocityMinMaxSlider->setBounds(velocitySliderArea);

    SynthSection::resized();
}