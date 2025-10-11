//
// Created by Dan Trueman on 10/8/25.
//

#include "ResonanceParametersView.h"
void ResonanceParametersView::resized()
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

    //
    // *** done with meters placement section
    //

    juce::Rectangle<int> keyboardsRect = bounds.removeFromRight(bounds.getWidth() * 0.5);
    keyboardsRect.reduce(largepadding, largepadding);
    int keyboardHeight = 120;
    int labelsectionheight = findValue(Skin::kLabelHeight);
    int kpadding = (keyboardsRect.getHeight() - 4. * (keyboardHeight + labelsectionheight)) / 3.;

    fundamentalKeyboard->setBounds(keyboardsRect.removeFromBottom(keyboardHeight));
    fundamentalKeyboard_label->setBounds(keyboardsRect.removeFromBottom(labelsectionheight));
    keyboardsRect.removeFromBottom(kpadding);

    closestKeyboard->setBounds(keyboardsRect.removeFromBottom(keyboardHeight));
    closestKeyboard_label->setBounds(keyboardsRect.removeFromBottom(labelsectionheight));
    keyboardsRect.removeFromBottom(kpadding);

    offsetsKeyboard->setBounds(keyboardsRect.removeFromBottom(keyboardHeight));
    offsetsKeyboard_label->setBounds(keyboardsRect.removeFromBottom(labelsectionheight));
    keyboardsRect.removeFromBottom(kpadding);

    gainsKeyboard->setBounds(keyboardsRect.removeFromBottom(keyboardHeight));
    gainsKeyboard_label->setBounds(keyboardsRect.removeFromBottom(labelsectionheight));

    bounds.reduce(largepadding, largepadding);
    juce::Rectangle<int> envRect = bounds.removeFromBottom(bounds.getHeight() * 0.5);
    envSection->setBounds(envRect);

    SynthSection::resized();
}