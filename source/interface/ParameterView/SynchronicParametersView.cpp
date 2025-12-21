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
    int labelsectionheight = findValue(Skin::kLabelHeight);
    auto knobLabelSize = findValue(Skin::kKnobLabelSizeSmall);
    int smallpadding = findValue(Skin::kPadding);
    int largepadding = findValue(Skin::kLargePadding);
    auto menuLabelSize = findValue(Skin::kButtonFontSize);

    DBG("largepadding = " << largepadding);
    DBG("smallpadding = " << smallpadding);

    pulseTriggeredBy_label->setTextSize (menuLabelSize);
    determinesCluster_label->setTextSize (menuLabelSize);

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

    //
    // *** done with meters placement section
    //

    bounds.reduce(largepadding, smallpadding);

    // make a left column for the menus, knobs, sliders, and ADSR
    juce::Rectangle<int> leftColumn = bounds.removeFromLeft(bounds.getWidth() / 3);

    //leftColumn.removeFromTop(smallpadding);
    int leftColumnWidth = leftColumn.getWidth();

    envSection->setBounds(leftColumn.removeFromBottom(knob_section_height * 3));
    leftColumn.removeFromBottom(smallpadding);
    clusterMinMaxSlider->setBounds(leftColumn.removeFromBottom(knob_section_height));
    leftColumn.removeFromBottom(smallpadding);
    holdTimeMinMaxSlider->setBounds(leftColumn.removeFromBottom(knob_section_height));
    leftColumn.removeFromBottom(largepadding);

    variousControlsBorder->setBounds(leftColumn);
    leftColumn.removeFromBottom(largepadding);
    leftColumn.removeFromTop(largepadding);
    leftColumn.reduce(0, largepadding);

    juce::Rectangle knobRow = leftColumn.removeFromBottom(knob_section_height);
    clusterThickness_knob->setBounds(knobRow.removeFromLeft(knobRow.getWidth() / 2).reduced(largepadding, 0));
    clusterThreshold_knob->setBounds(knobRow.reduced(largepadding, 0));
    juce::Rectangle<int> ctn_label_rect (clusterThickness_knob->getX(), clusterThickness_knob->getBottom() - largepadding, clusterThickness_knob->getWidth(), labelsectionheight );
    clusterThickness_knob_label->setBounds(ctn_label_rect);
    juce::Rectangle<int> cnl_label_rect (clusterThreshold_knob->getX(), clusterThreshold_knob->getBottom() - largepadding, clusterThreshold_knob->getWidth(), labelsectionheight );
    clusterThreshold_knob_label->setBounds(cnl_label_rect);

    knobRow = leftColumn.removeFromBottom(knob_section_height);

    numPulses_knob->setBounds(knobRow.removeFromLeft(knobRow.getWidth() / 2).reduced(largepadding, 0));
    numLayers_knob->setBounds(knobRow.reduced(largepadding, 0));

    juce::Rectangle<int> np_label_rect (numPulses_knob->getX(), numPulses_knob->getBottom() - largepadding, numPulses_knob->getWidth(), labelsectionheight );
    numPulses_knob_label->setBounds(np_label_rect);
    juce::Rectangle<int> nl_label_rect (numLayers_knob->getX(), numLayers_knob->getBottom() - largepadding, numLayers_knob->getWidth(), labelsectionheight );
    numLayers_knob_label->setBounds(nl_label_rect);

    clusterThreshold_knob_label->setTextSize (knobLabelSize);
    clusterThickness_knob_label->setTextSize (knobLabelSize);
    numPulses_knob_label->setTextSize (knobLabelSize);
    numLayers_knob_label->setTextSize (knobLabelSize);

    leftColumn.removeFromBottom(largepadding);
    leftColumn.reduce (largepadding * 2, 0);
    int remainingSpace = leftColumn.getHeight() - (menu_section_height * 2 + smallpadding * 2);
    leftColumn.removeFromBottom(remainingSpace / 3);
    juce::Rectangle comboBoxRow = leftColumn.removeFromBottom(menu_section_height);
    juce::Rectangle<int> skipFirstRect = comboBoxRow.removeFromRight(leftColumnWidth/4);
    skipFirst->setBounds(skipFirstRect.removeFromBottom(menu_section_height));

    pulseTriggeredBy_combo_box->setBounds(comboBoxRow.removeFromRight(leftColumnWidth / 2).reduced(largepadding, 0));
    comboBoxRow.removeFromRight(smallpadding);
    pulseTriggeredBy_label->setBounds(comboBoxRow);

    leftColumn.removeFromBottom(smallpadding);

    comboBoxRow = leftColumn.removeFromBottom(menu_section_height);
    comboBoxRow.removeFromRight(leftColumnWidth/4);
    determinesCluster_combo_box->setBounds(comboBoxRow.removeFromRight(leftColumnWidth / 2).reduced(largepadding, 0));
    comboBoxRow.removeFromRight(smallpadding);
    determinesCluster_label->setBounds(comboBoxRow);

    //
    // *** now on to the right section for the multisliders
    //

    bounds.removeFromLeft(largepadding);

    // how much vertical space will we need for all the components?
    float sliderSectionHeight = bounds.getHeight() / 5. ;

    transpositionsSlider->setBounds(bounds.removeFromTop(sliderSectionHeight - smallpadding));
    bounds.removeFromTop(smallpadding);
    accentsSlider->setBounds(bounds.removeFromTop(sliderSectionHeight - smallpadding));
    bounds.removeFromTop(smallpadding);
    sustainLengthMultipliersSlider->setBounds(bounds.removeFromTop(sliderSectionHeight - smallpadding));
    bounds.removeFromTop(smallpadding);
    beatLengthMultipliersSlider->setBounds(bounds.removeFromTop(sliderSectionHeight - smallpadding));
    bounds.removeFromTop(smallpadding);
    envSequenceSection->setBounds(bounds.removeFromTop(sliderSectionHeight));

    int useTuningWidth = 100;
    int useTuningHeight = 20;
    useTuning->setBounds(transpositionsSlider->getRight() - useTuningWidth - smallpadding, transpositionsSlider->getBottom() - useTuningHeight - smallpadding, useTuningWidth, useTuningHeight);
    useTuning->toFront(false);

    SynthSection::resized();
}