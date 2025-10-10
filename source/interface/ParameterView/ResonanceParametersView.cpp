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

    juce::Rectangle<int> keyboardsRect = bounds.removeFromRight(bounds.getWidth() * 0.75);
    keyboardsRect.reduce(largepadding, largepadding);
    int keyboardHeight = keyboardsRect.getHeight() / 4. - largepadding * 3;
    gainsKeyboard->setBounds(keyboardsRect.removeFromTop(keyboardHeight));
    keyboardsRect.removeFromTop(largepadding);
    offsetsKeyboard->setBounds(keyboardsRect.removeFromTop(keyboardHeight));
    keyboardsRect.removeFromTop(largepadding);
    closestKeyboard->setBounds(keyboardsRect.removeFromTop(keyboardHeight));
    keyboardsRect.removeFromTop(largepadding);
    fundamentalKeyboard->setBounds(keyboardsRect.removeFromTop(keyboardHeight));

    SynthSection::resized();
}