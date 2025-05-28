//
// Created by Davis Polito on 5/27/25.
//

#include "BKOnOffKeyboardComponent.h"

#include "juce_core/unit_tests/juce_UnitTestCategories.h"

void BKOnOffKeyboardComponent::drawKeyboardBackground(juce::Graphics & g, juce::Rectangle<float> area) {
    g.fillAll (findColour (whiteNoteColourId));

    auto width = area.getWidth();
    auto height = area.getHeight();
    auto currentOrientation = getOrientation();
    juce::Point<float> shadowGradientStart, shadowGradientEnd;

    if (currentOrientation == verticalKeyboardFacingLeft)
    {
        shadowGradientStart.x = width - 1.0f;
        shadowGradientEnd.x   = width - 5.0f;
    }
    else if (currentOrientation == verticalKeyboardFacingRight)
    {
        shadowGradientEnd.x = 5.0f;
    }
    else
    {
        shadowGradientEnd.y = 5.0f;
    }

    auto keyboardWidth = getRectangleForKey (getRangeEnd()).getRight();
    auto shadowColour = findColour (shadowColourId);

    if (! shadowColour.isTransparent())
    {
        g.setGradientFill ({ shadowColour, shadowGradientStart,
                             shadowColour.withAlpha (0.0f), shadowGradientEnd,
                             false });

        switch (currentOrientation)
        {
            case horizontalKeyboard:            g.fillRect (0.0f, 0.0f, keyboardWidth, 5.0f); break;
            case verticalKeyboardFacingLeft:    g.fillRect (width - 5.0f, 0.0f, 5.0f, keyboardWidth); break;
            case verticalKeyboardFacingRight:   g.fillRect (0.0f, 0.0f, 5.0f, keyboardWidth); break;
            default: break;
        }
    }

    auto lineColour = findColour (keySeparatorLineColourId);

    if (! lineColour.isTransparent())
    {
        g.setColour (lineColour);

        switch (currentOrientation)
        {
            case horizontalKeyboard:            g.fillRect (0.0f, height - 1.0f, keyboardWidth, 1.0f); break;
            case verticalKeyboardFacingLeft:    g.fillRect (0.0f, 0.0f, 1.0f, keyboardWidth); break;
            case verticalKeyboardFacingRight:   g.fillRect (width - 1.0f, 0.0f, 1.0f, keyboardWidth); break;
            default: break;
        }
    }
}
void BKOnOffKeyboardComponent::drawBlackKey(int midiNoteNumber, juce::Graphics &g, juce::Rectangle<float> area) {

    juce::Colour c (juce::Colours::black);
    if (keys.test(midiNoteNumber)) c = findColour(keyDownOverlayColourId);
    if (mouseOverNote == midiNoteNumber) c = c.overlaidWith (findColour (mouseOverKeyOverlayColourId));

    g.setColour (c);
    g.fillRect (area);

    g.setColour (c.brighter());
    auto sideIndent = 1.0f / 8.0f;
    auto topIndent = 7.0f / 8.0f;
    auto w = area.getWidth();
    auto h = area.getHeight();

    switch (getOrientation())
    {
        case horizontalKeyboard:            g.fillRect (area.reduced (w * sideIndent, 0).removeFromTop   (h * topIndent)); break;
        case verticalKeyboardFacingLeft:    g.fillRect (area.reduced (0, h * sideIndent).removeFromRight (w * topIndent)); break;
        case verticalKeyboardFacingRight:   g.fillRect (area.reduced (0, h * sideIndent).removeFromLeft  (w * topIndent)); break;
        default: break;
    }


}
void BKOnOffKeyboardComponent::drawWhiteKey(int midiNoteNumber, juce::Graphics &g, juce::Rectangle<float> area) {

    auto c = juce::Colours::transparentWhite;
    if (keys.test(midiNoteNumber)) c = findColour(keyDownOverlayColourId).withAlpha(0.5f);
    if (mouseOverNote == midiNoteNumber) c = c.overlaidWith (findColour (mouseOverKeyOverlayColourId));
    int w = area.getWidth();
    int h = area.getHeight();
    int x = area.getX();
    int y = area.getY();
    g.setColour (c);
    g.fillRect (x, y, w, h);




    if (! juce::Colours::dimgrey.isTransparent())
    {
        g.setColour (juce::Colours::dimgrey);

        switch (getOrientation())
        {
            case horizontalKeyboard:            g.fillRect (x, y, 1, h); break;
            case verticalKeyboardFacingLeft:    g.fillRect (x, y, w, 1); break;
            case verticalKeyboardFacingRight:   g.fillRect (x, y + h - 1, w, 1); break;
            default: break;
        }

        if (midiNoteNumber == getRangeEnd())
        {
            switch (getOrientation())
            {
                case horizontalKeyboard:            g.fillRect (x + w, y, 1, h); break;
                case verticalKeyboardFacingLeft:    g.fillRect (x, y + h, w, 1); break;
                case verticalKeyboardFacingRight:   g.fillRect (x, y - 1, w, 1); break;
                default: break;
            }
        }
    }
}