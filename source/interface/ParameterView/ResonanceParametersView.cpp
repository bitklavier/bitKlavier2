//
// Created by Dan Trueman on 10/8/25.
//

#include "ResonanceParametersView.h"

void ResonanceParametersView::timerCallback(void)
{
    /*
     * probably a more direct way to do this without a timer...
     */
    heldKeysKeyboard->redoImage();
}

void ResonanceParametersView::resized()
{
    // width of the title at left, used in all preparations
    int title_width = getTitleWidth();

    // height for most of these components
    int knob_section_height = getKnobSectionHeight();
    int menu_section_height = findValue(Skin::kComboMenuHeight);

    int smallpadding = findValue(Skin::kPadding);
    int largepadding = findValue(Skin::kLargePadding);

    juce::Rectangle<int> titleArea = getLocalBounds().removeFromLeft(getTitleWidth());
    prepTitle->setBounds(titleArea);

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

    heldKeysKeyboard->setBounds(bounds.removeFromBottom(120));

    // overtone structure keyboards
    juce::Rectangle<int> keyboardsRect = bounds.removeFromRight(bounds.getWidth() * 0.5);
    keyboardsRect.reduce(largepadding, largepadding);
    juce::Rectangle<int> spectrumMenuArea = keyboardsRect.removeFromTop(menu_section_height);
    spectrumMenuArea.reduce(spectrumMenuArea.getWidth() * 0.25, 0);
    spectrum_combo_box->setBounds(spectrumMenuArea);

    int keyboardHeight = 100;
    int labelsectionheight = findValue(Skin::kLabelHeight);
    int kpadding = (keyboardsRect.getHeight() - 4. * (keyboardHeight + labelsectionheight)) / 3.;

    fundamentalKeyboard->setBounds(keyboardsRect.removeFromBottom(keyboardHeight));
    fundamentalKeyboard_label->setBounds(fundamentalKeyboard->getX(), fundamentalKeyboard->getY() + 4, fundamentalKeyboard->getWidth(), labelsectionheight);
    keyboardsRect.removeFromBottom(kpadding + labelsectionheight);

    closestKeyboard->setBounds(keyboardsRect.removeFromBottom(keyboardHeight));
    closestKeyboard_label->setBounds(closestKeyboard->getX(), closestKeyboard->getY() + 4, closestKeyboard->getWidth(), labelsectionheight);
    keyboardsRect.removeFromBottom(kpadding + labelsectionheight);

    offsetsKeyboard->setBounds(keyboardsRect.removeFromBottom(keyboardHeight));
    offsetsKeyboard_label->setBounds(offsetsKeyboard->getX(), offsetsKeyboard->getY() + 4, offsetsKeyboard->getWidth(), labelsectionheight);
    keyboardsRect.removeFromBottom(kpadding + labelsectionheight);

    gainsKeyboard->setBounds(keyboardsRect.removeFromBottom(keyboardHeight));
    gainsKeyboard_label->setBounds(gainsKeyboard->getX(), gainsKeyboard->getY() + 4, gainsKeyboard->getWidth(), labelsectionheight);

    // adsr
    bounds.reduce(largepadding, largepadding);
    juce::Rectangle<int> envRect = bounds.removeFromBottom(bounds.getHeight() * 0.5);
    envSection->setBounds(envRect);

    // knobs
    bounds.removeFromBottom(largepadding);
    juce::Rectangle<int> outputKnobsArea = bounds.removeFromBottom(knob_section_height);
    placeKnobsInArea(outputKnobsArea, _sliders, false);
    int sl_counter = 0;
    for (auto& slider : _sliders)
    {
        juce::Rectangle<int> sl_label_rect (slider->getX(), slider->getBottom() - 10, slider->getWidth(), labelsectionheight );
        slider_labels[sl_counter++]->setBounds(sl_label_rect);
    }

    SynthSection::resized();
}