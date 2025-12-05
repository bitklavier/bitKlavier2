
//
// Created by Dan Trueman on 11/5/24.
//

#include "DirectParametersView.h"

void DirectParametersView::resized()
{
    // width of the title at left, used in all preparations
    int title_width = getTitleWidth();
    int smallpadding = findValue(Skin::kPadding);
    int largepadding = findValue(Skin::kLargePadding);

    // height for most of these components
    int knob_section_height = getKnobSectionHeight();
    int labelsectionheight = findValue(Skin::kLabelHeight);
    auto knobLabelSize = findValue(Skin::kKnobLabelSizeSmall);

    juce::Rectangle<int> titleArea = getLocalBounds().removeFromLeft(title_width);
    prepTitle->setBounds(titleArea);
    prepTitle->setTextSize (findValue(Skin::kPrepTitleSize));

    // get the prep area, with left/right border for title
    juce::Rectangle<int> bounds = getLocalBounds();
    bounds.removeFromLeft(title_width);

    // bounds for level meter on right side
    juce::Rectangle<int> meterArea = bounds.removeFromRight(title_width);
    levelMeter->setBounds(meterArea);
    bounds.removeFromRight(smallpadding);
    sendLevelMeter->setBounds(bounds.removeFromRight(title_width));

    // how much vertical space will we need for all the components?
    int verticalAreaNeeded = knob_section_height * 7;

    // how much vertical space is left, divided up so we have some buffer space between each component
    int bufferSpaceForEach = (bounds.getHeight() - verticalAreaNeeded) / 5;
    if (bufferSpaceForEach < 0 ) bufferSpaceForEach = 0;

    // start at the top, add the output knobs (main gain, hammers, resonance, etc..., and send)
    bounds.removeFromTop(bufferSpaceForEach);
    juce::Rectangle<int> outputKnobsArea = bounds.removeFromTop(knob_section_height * 1.25);
    mixKnobsBorder->setBounds (outputKnobsArea);
    outputKnobsArea.reduce(0, largepadding);
    placeKnobsInArea(outputKnobsArea, _sliders, false);

    int sl_counter = 0;
    for (auto& slider : _sliders)
    {
        juce::Rectangle<int> sl_label_rect (slider->getX(), slider->getBottom() - 10, slider->getWidth(), labelsectionheight );
        slider_labels[sl_counter]->setTextSize (knobLabelSize);
        slider_labels[sl_counter++]->setBounds(sl_label_rect);
    }

    // add the adsr below that
    bounds.removeFromTop(bufferSpaceForEach);
    juce::Rectangle<int> adsrArea = bounds.removeFromTop(knob_section_height * 5);
    envSection->setBounds(adsrArea);

    // add the transposition and velocity range sliders below that
    bounds.removeFromTop(bufferSpaceForEach);
    juce::Rectangle<int> transpositionSliderArea = bounds.removeFromTop(knob_section_height);
    transpositionSlider->setBounds(transpositionSliderArea);

    SynthSection::resized();
}


