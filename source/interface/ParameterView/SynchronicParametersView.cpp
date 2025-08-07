//
// Created by Dan Trueman on 8/5/25.
//

#include "SynchronicParametersView.h"
void SynchronicParametersView::timerCallback()
{
    /*
     * for updating the currently active sliders in the UI
     */
    transpositionsSlider->setCurrentSlider(sparams_.transpositions_current.load());
    accentsSlider->setCurrentSlider(sparams_.accents_current.load());
    sustainLengthMultipliersSlider->setCurrentSlider(sparams_.sustainLengthMultipliers_current.load());
    beatLengthMultipliersSlider->setCurrentSlider(sparams_.beatLengthMultipliers_current.load());

    transpositionsSlider->redoImage();
    accentsSlider->redoImage();
    sustainLengthMultipliersSlider->redoImage();
    beatLengthMultipliersSlider->redoImage();
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

    juce::Rectangle comboBoxRow = leftColumn.removeFromTop(menu_section_height);
    pulseTriggeredBy_combo_box->setBounds(comboBoxRow.removeFromRight(comboBoxRow.getWidth() / 2));
    comboBoxRow.removeFromRight(smallpadding);
    pulseTriggeredBy_label->setBounds(comboBoxRow);

    leftColumn.removeFromTop(smallpadding);

    comboBoxRow = leftColumn.removeFromTop(menu_section_height);
    determinesCluster_combo_box->setBounds(comboBoxRow.removeFromRight(comboBoxRow.getWidth() / 2));
    comboBoxRow.removeFromRight(smallpadding);
    determinesCluster_label->setBounds(comboBoxRow);

    // knobs
    juce::Rectangle knobRow = leftColumn.removeFromTop(knob_section_height);
    numPulses_knob->setBounds(knobRow.removeFromLeft(knobRow.getWidth() / 2));
    numLayers_knob->setBounds(knobRow);
    leftColumn.removeFromTop(smallpadding);

    knobRow = leftColumn.removeFromTop(knob_section_height);
    clusterThickness_knob->setBounds(knobRow.removeFromLeft(knobRow.getWidth() / 2));
    clusterThreshold_knob->setBounds(knobRow);
    leftColumn.removeFromTop(smallpadding);

    envSection->setBounds(leftColumn.removeFromBottom(knob_section_height * 2));
    leftColumn.removeFromBottom(smallpadding);
    clusterMinMaxSlider->setBounds(leftColumn.removeFromBottom(knob_section_height));
    holdTimeMinMaxSlider->setBounds(leftColumn.removeFromBottom(knob_section_height));

    // *** now on to the right section for the multisliders

    bounds.removeFromLeft(largepadding);
    bounds.removeFromTop(largepadding);

    // how much vertical space will we need for all the components?
    int verticalAreaNeeded = knob_section_height * 1.5 * 4;

    /*
     * todo: better spacing for these...
     */
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

    SynthSection::resized();
}