//
// Created by Dan Trueman on 8/5/25.
//

#include "SynchronicParametersView.h"
void SynchronicParametersView::timerCallback()
{
    /*
     * for updating the currently active sliders in the UI
     */
    if(!transpositionsSlider->hovering_)
    {
        transpositionsSlider->setCurrentSlider (sparams_.transpositions_current.load());
        transpositionsSlider->redoImage();
    }
    if(!accentsSlider->hovering_)
    {
        accentsSlider->setCurrentSlider (sparams_.accents_current.load());
        accentsSlider->redoImage();
    }
    if(!sustainLengthMultipliersSlider->hovering_)
    {
        sustainLengthMultipliersSlider->setCurrentSlider (sparams_.sustainLengthMultipliers_current.load());
        sustainLengthMultipliersSlider->redoImage();
    }
    if(!beatLengthMultipliersSlider->hovering_)
    {
        beatLengthMultipliersSlider->setCurrentSlider (sparams_.beatLengthMultipliers_current.load());
        beatLengthMultipliersSlider->redoImage();
    }

    envSequenceSection->setCurrentlyPlayingEnvelope(sparams_.envelopes_current.load());
}

void SynchronicParametersView::resized()
{
    // width of the title at left, used in all preparations
    int title_width = getTitleWidth();

    // height for most of these components
    int knob_section_height = getKnobSectionHeight();
    int menu_section_height = findValue(Skin::kComboMenuHeight);

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

    // *** done with meters placement section

    bounds.reduce(largepadding, largepadding);

    // make a left column for the menus, knobs, sliders, and ADSR
    juce::Rectangle<int> leftColumn = bounds.removeFromLeft(bounds.getWidth() / 3);

    leftColumn.removeFromTop(largepadding);

    juce::Rectangle comboBoxRow = leftColumn.removeFromTop(menu_section_height);
    pulseTriggeredBy_combo_box->setBounds(comboBoxRow.removeFromRight(comboBoxRow.getWidth() / 2).reduced(largepadding, 0));
    comboBoxRow.removeFromRight(smallpadding);
    pulseTriggeredBy_label->setBounds(comboBoxRow);

    leftColumn.removeFromTop(smallpadding);

    comboBoxRow = leftColumn.removeFromTop(menu_section_height);
    determinesCluster_combo_box->setBounds(comboBoxRow.removeFromRight(comboBoxRow.getWidth() / 2).reduced(largepadding, 0));
    comboBoxRow.removeFromRight(smallpadding);
    determinesCluster_label->setBounds(comboBoxRow);

    // knobs
    leftColumn.removeFromTop(largepadding);

    juce::Rectangle knobRow = leftColumn.removeFromTop(knob_section_height);
    numPulses_knob->setBounds(knobRow.removeFromLeft(knobRow.getWidth() / 2).reduced(largepadding, 0));
    numLayers_knob->setBounds(knobRow.reduced(largepadding, 0));

    leftColumn.removeFromTop(smallpadding);

    knobRow = leftColumn.removeFromTop(knob_section_height);
    clusterThickness_knob->setBounds(knobRow.removeFromLeft(knobRow.getWidth() / 2).reduced(largepadding, 0));
    clusterThreshold_knob->setBounds(knobRow.reduced(largepadding, 0));

    leftColumn.removeFromTop(largepadding);

    clusterMinMaxSlider->setBounds(leftColumn.removeFromTop(knob_section_height));
    leftColumn.removeFromTop(smallpadding);
    holdTimeMinMaxSlider->setBounds(leftColumn.removeFromTop(knob_section_height));

    leftColumn.removeFromTop(largepadding);

    envSection->setBounds(leftColumn);


    // *** now on to the right section for the multisliders

    bounds.removeFromLeft(largepadding);
    bounds.removeFromTop(largepadding);

    // how much vertical space will we need for all the components?
    int verticalAreaNeeded = knob_section_height * 1.5 * 5;

    // how much vertical space is left, divided up so we have some buffer space between each component
    int bufferSpaceForEach = (bounds.getHeight() - verticalAreaNeeded) / 4;
    if (bufferSpaceForEach < 0 ) bufferSpaceForEach = 0;

    transpositionsSlider->setBounds(bounds.removeFromTop(knob_section_height * 1.5));
    bounds.removeFromTop(bufferSpaceForEach);
    accentsSlider->setBounds(bounds.removeFromTop(knob_section_height * 1.5));
    bounds.removeFromTop(bufferSpaceForEach);
    sustainLengthMultipliersSlider->setBounds(bounds.removeFromTop(knob_section_height * 1.5));
    bounds.removeFromTop(bufferSpaceForEach);
    beatLengthMultipliersSlider->setBounds(bounds.removeFromTop(knob_section_height * 1.5));
    bounds.removeFromTop(bufferSpaceForEach);
    envSequenceSection->setBounds(bounds.removeFromTop(knob_section_height * 1.5));

    SynthSection::resized();
}