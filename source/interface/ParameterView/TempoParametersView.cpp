//
// Created by Myra Norton on 8/20/25.
//

#include "TempoParametersView.h"

void TempoParametersView::resized()
{
    // width of the title at left, used in all preparations
    int title_width = getTitleWidth();

    // height for most of these components
    int knob_section_height = getKnobSectionHeight();

    // get the prep area, with left/right border for title
    juce::Rectangle<int> bounds = getLocalBounds();
    bounds.removeFromLeft(title_width);

    // how much vertical space will we need for all the components?
    int verticalAreaNeeded = knob_section_height * 2;

    // how much vertical space is left, divided up so we have some buffer space between each component
    int bufferSpaceForEach = (bounds.getHeight() - verticalAreaNeeded) / 5;
    if (bufferSpaceForEach < 0 ) bufferSpaceForEach = 0;

    // start at the top, add the output knobs (main gain, hammers, resonance, etc..., and send)
    bounds.removeFromTop(bufferSpaceForEach);
    juce::Rectangle<int> slidersArea = bounds.removeFromTop(knob_section_height * 2);
    int sliderHeight = knob_section_height; // or tweak this
    slidersArea.setWidth (slidersArea.getWidth()*0.25);
    // slidersArea.setX((bounds.getWidth() - slidersArea.getWidth()) * 0.5);

    for (auto& slider : _sliders)
    {
        slider->setBounds (slidersArea.removeFromTop(sliderHeight));
    }
    // DBG(" output knob area" + juce::String(outputKnobsArea.getWidth()) + " " + juce::String(outputKnobsArea.getHeight()));


    SynthSection::resized();
}