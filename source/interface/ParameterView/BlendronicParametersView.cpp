//
// Created by Dan Trueman on 7/26/25.
//

#include "BlendronicParametersView.h"

void BlendronicParametersView::resized()
{
    // width of the title at left, used in all preparations
    int title_width = getTitleWidth();

    // height for most of these components
    int knob_section_height = getKnobSectionHeight();

    // get the prep area, with left/right border for title
    juce::Rectangle<int> bounds = getLocalBounds();
    bounds.removeFromLeft(title_width);

    // bounds for level meter on right side
    juce::Rectangle<int> meterArea = bounds.removeFromRight(title_width);
    levelMeter->setBounds(meterArea);

    // how much vertical space will we need for all the components?
    int verticalAreaNeeded = knob_section_height * 7;

    // how much vertical space is left, divided up so we have some buffer space between each component
    int bufferSpaceForEach = (bounds.getHeight() - verticalAreaNeeded) / 5;
    if (bufferSpaceForEach < 0 ) bufferSpaceForEach = 0;

    // start at the top, add the output knobs
    bounds.removeFromTop(bufferSpaceForEach);
    juce::Rectangle<int> outputKnobsArea = bounds.removeFromTop(knob_section_height);
    placeKnobsInArea(outputKnobsArea, _sliders, true);
    DBG(" output knob area" + juce::String(outputKnobsArea.getWidth()) + " " + juce::String(outputKnobsArea.getHeight()));

    // add the adsr below that
    bounds.removeFromTop(bufferSpaceForEach);
    juce::Rectangle<int> beatlengthsArea = bounds.removeFromTop(knob_section_height * 5);
    beatLengthsSlider->setBounds(beatlengthsArea);
//    envSection->setBounds(adsrArea);
//
//    // add the transposition and velocity range sliders below that
//    bounds.removeFromTop(bufferSpaceForEach);
//    juce::Rectangle<int> transpositionSliderArea = bounds.removeFromTop(knob_section_height);
//    juce::Rectangle<int> velocitySliderArea = transpositionSliderArea.removeFromLeft(transpositionSliderArea.getWidth() * 0.5);
//    transpositionSlider->setBounds(transpositionSliderArea);
//    velocityMinMaxSlider->setBounds(velocitySliderArea);

    SynthSection::resized();
}