//
// Created by Davis Polito on 5/19/25.
//
/*
  ==============================================================================

    BKAbsoluteKeyboardSlider.cpp
    Created: 29 Jul 2018 4:40:55pm
    Author:  Daniel Trueman

  ==============================================================================
*/

#include "BKAbsoluteKeyboardSlider.h"

#include "chowdsp_core/third_party/span-lite/test/lest/lest_cpp03.hpp"
#include "juce_core/juce_core.h"
void KeyboardOffsetComponent::drawBlackKey(int midiNoteNumber, juce::Graphics &g, juce::Rectangle<float> area) {

    juce::Colour c (juce::Colours::black);

    g.setColour (c);
    g.fillRect (area);

    float keyVal = state.tuningOffset[midiNoteNumber];
    //if(keyVal != 0.)
    if(keyVal != midRange)
    {
        if(keyVal > midRange) c = c.overlaidWith (juce::Colours::red.withSaturation ( sqrt((keyVal - midRange) / (maxRange - midRange))) );
        else c = c.overlaidWith (juce::Colours::blue.withSaturation ( sqrt((midRange - keyVal) / (midRange - minRange))));

        g.setColour(c);
    }
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

    //     else g.setColour (c.brighter());
    //     int w = area.getWidth();
    // int h = area.getHeight();
    // int x = area.getX();
    // int y = area.getY();
    //     const int xIndent = juce::jmax (1, juce::jmin (w, h) / 8);
    //
    //     switch (getOrientation())
    //     {
    //         case horizontalKeyboard:            g.fillRect (x + xIndent, y, w - xIndent * 2, 7 * h / 8); break;
    //         case verticalKeyboardFacingLeft:    g.fillRect (x + w / 8, y + xIndent, w - w / 8, h - xIndent * 2); break;
    //         case verticalKeyboardFacingRight:   g.fillRect (x, y + xIndent, 7 * w / 8, h - xIndent * 2); break;
    //         default: break;
    //     }


}

void KeyboardOffsetComponent::drawWhiteKey(int midiNoteNumber, juce::Graphics &g, juce::Rectangle<float> area) {

    auto c = juce::Colours::transparentWhite;
    float keyVal = state.tuningOffset[midiNoteNumber];


    if(keyVal != midRange)
    {

        if(keyVal > midRange) c = c.overlaidWith (juce::Colours::red.withSaturation ( sqrt((keyVal - midRange) / (maxRange - midRange))) );
        else c = c.overlaidWith (juce::Colours::blue.withSaturation ( sqrt((midRange - keyVal) / (midRange - minRange))));

    }

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
void KeyboardOffsetComponent::drawKeyboardBackground(juce::Graphics & g, juce::Rectangle<float> area) {
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
BKAbsoluteKeyboardSlider::BKAbsoluteKeyboardSlider(TuningKeyboardState* state,bool toggles, bool nos):
needsOctaveSlider(nos),
ratio(1.0),
keyboardState(state)
{
    keyboard = std::make_unique<KeyboardOffsetComponent>(*keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard);

    addAndMakeVisible (*keyboard);


    // need slider or other interface for octave change

    minKey = 21; // 21
    maxKey = 108; // 108
    displayResolution = 1;

    keyboard->setRepaintsOnMouseActivity(false);
    keyboard->setScrollButtonsVisible(false);
    keyboard->setAvailableRange(minKey, maxKey);
    keyboard->setOctaveForMiddleC(4);
    // keyboard->setAllowDrag(false);
    // keyboard->doKeysToggle(toggles);
    keyboard->addMouseListener(this, true);
    keyboardState->keyboardState.addListener(this);
    lastKeyPressed = 0;

    showName.setText("unnamed keyboard slider", juce::dontSendNotification);
    showName.setJustificationType(juce::Justification::centredRight);
    showName.addMouseListener(this, true);
    addAndMakeVisible(showName);

    keyboardValueTF.setText(juce::String(keyboard->midRange, displayResolution));
    keyboardValueTF.setName("KSLIDERTXT");
    keyboardValueTF.addListener(this);
#if JUCE_IOS
    keyboardValueTF.setReadOnly(true);
#endif
    addAndMakeVisible(keyboardValueTF);

    keyboardValsTextField = std::make_unique<juce::TextEditor>();
    keyboardValsTextField->setMultiLine(true);
    keyboardValsTextField->setName("KSLIDERTXTEDITALL");
    keyboardValsTextField->addListener(this);
    addAndMakeVisible(keyboardValsTextField.get());
    keyboardValsTextField->setAlpha(0);
    keyboardValsTextField->toBack();

    keyboardValsTextFieldOpen.setName("KSLIDERTXTEDITALLBUTTON");
    keyboardValsTextFieldOpen.addListener(this);
    keyboardValsTextFieldOpen.setButtonText("edit all");
    keyboardValsTextFieldOpen.setTooltip("click drag on keys to set values by key, or press 'edit all' to edit as text");
    addAndMakeVisible(keyboardValsTextFieldOpen);
}

#if JUCE_IOS
void BKAbsoluteKeyboardSlider::sliderValueChanged     (juce::Slider* slider)
{
    if (slider == &octaveSlider)
    {
        int octave = (int) octaveSlider.getValue();

        if (octave == 0)    keyboard->setAvailableRange(21, 45);
        else                keyboard->setAvailableRange(12+octave*12, 36+octave*12);
    }
}
#endif



void BKAbsoluteKeyboardSlider::resized()
{
    float heightUnit = getHeight() * 0.1;
    float widthUnit = getWidth() * 0.1;

    juce::Rectangle<int> area (getLocalBounds());
    float keyboardHeight = 8 * heightUnit;
    juce::Rectangle<int> keymapRow = area.removeFromBottom(10 * heightUnit);

    //absolute keymap
    float keyWidth = keymapRow.getWidth() / round((maxKey - minKey) * 7./12 + 1); //num white keys
    keyboard->setKeyWidth(keyWidth);
    keyboard->setBlackNoteLengthProportion(0.65);

    juce::Rectangle<int> keyboardRect = keymapRow.removeFromBottom(keyboardHeight);

#if JUCE_IOS
    if (needsOctaveSlider)
    {
        float sliderHeight = 15;
        juce::Rectangle<int> sliderArea = keyboardRect.removeFromTop(sliderHeight);
        octaveSlider.setBounds(sliderArea);
    }
#endif
    keyboard->setBounds(keyboardRect);

    juce::Rectangle<int> textSlab (keymapRow.removeFromBottom(2*heightUnit + 4));
    keyboardValueTF.setBounds(textSlab.removeFromRight(ratio * widthUnit));
    showName.setBounds(textSlab.removeFromRight(2*ratio*widthUnit));
    keyboardValsTextFieldOpen.setBounds(textSlab.removeFromLeft(ratio*widthUnit*1.5));

#if JUCE_IOS
    keyboardValsTextField->setBounds(keyboard->getBounds());
    keyboardValsTextField->setSize(keyboard->getWidth() * 0.5f, keyboard->getHeight());
#else
    keyboardValsTextField->setBounds(keyboard->getBounds());
#endif
}


void BKAbsoluteKeyboardSlider::setAvailableRange(int min, int max)
{
    minKey = min;
    maxKey = max;
    keyboardSize = max - min; //

    //all of the above unnecessary?
    keyboard->setAvailableRange(minKey, maxKey);
}

void BKAbsoluteKeyboardSlider::mouseMove(const juce::MouseEvent& e)
{
   // keyboardValueTF.setText(juce::String(keyboard->getLastNoteOverValue(), displayResolution), dontSendNotification);
}

void BKAbsoluteKeyboardSlider::mouseDrag(const juce::MouseEvent& e)
{
    if (disabledKeys.contains(lastKeyPressed))
    {
        DBG("key disabled");
        return;
    }

    if(lastKeyPressed != -1)
    {
        auto myNote  = lastKeyPressed;
        bool isBlackKey = juce::MidiMessage::isMidiNoteBlack (myNote);
        DBG(juce::String((int)isBlackKey) + " is black key");
        float dragPos = (float)e.y / keyboard->getHeight();
        if(isBlackKey) dragPos /= keyboard->getBlackNoteLengthProportion();

        dragPos = 1. - 2. * dragPos;
        if(dragPos > 0.) dragPos = dragPos * dragPos;
        else dragPos = -1.* dragPos * dragPos;
        DBG("BKAbsoluteKeyboardSlider::mouseDrag dragPos = " + juce::String(dragPos));

        float outval;
        if (dragPos > 0) outval = keyboard->midRange + dragPos * (keyboard->maxRange - keyboard->midRange);
        else outval = keyboard->midRange + dragPos * (keyboard->midRange - keyboard->minRange);
        DBG("BKAbsoluteKeyboardSlider::mouseDrag outval = " + juce::String(outval));
        auto val  = std::clamp(outval, keyboard->minRange, keyboard->maxRange);
       // keyboardState->setKeyOffset(myNote, dragPos * 50.);
        keyboardValueTF.setText(juce::String(val, displayResolution), juce::dontSendNotification);
        keyboardState->setKeyOffset(myNote, val);

        //DBG("dragging last key, height " + juce::String(keyboard->getLastKeySelected()) + " " + juce::String(dragPos));
    }
}

void BKAbsoluteKeyboardSlider::mouseUp(const juce::MouseEvent& e)
{
    if(e.mouseWasClicked())
    {
        if(e.mods.isShiftDown())
        {
            keyboardValueTF.setText(juce::String(keyboard->midRange, displayResolution), juce::dontSendNotification);
            keyboardState->setKeyOffset(lastKeyPressed, 0.);
        }
    }

    // listeners.call(&BKAbsoluteKeyboardSlider::Listener::keyboardSliderChanged,
    //                getName(),
    //                keyboard->getValues());
    lastKeyPressed = -1;
    keyboard->repaint();
}

void BKAbsoluteKeyboardSlider::mouseDoubleClick(const juce::MouseEvent& e)
{
#if JUCE_IOS
    lastKeyPressed = -1;
    lastKeyPressed = keyboard->getLastNoteOver();

    if (e.eventComponent == keyboard)
    {
        if (lastKeyPressed >= 0)
        {
            hasBigOne = true;
            WantsBigOne::listeners.call(&WantsBigOne::Listener::iWantTheBigOne, &keyboardValueTF,
                                        "value for note " + midiToPitchClass(lastKeyPressed));
        }
    }
#endif
}

void BKAbsoluteKeyboardSlider::mouseDown(const juce::MouseEvent& e)
{
    if(e.y >= 0 && e.y <= keyboard->getHeight())
        lastKeyPressed =  keyboard->getNoteAndVelocityAtPosition(e.position).note;

}


//takes string of ordered pairs in the form x1:y1 x2:y2
//and converts into Array of floats of y, with indices x
juce::Array<float> stringOrderedPairsToFloatArray(juce::String s, int size, float init)
{

    juce::String tempInt = "";
    juce::String tempFloat = "";
    juce::String::CharPointerType c = s.getCharPointer();

    juce::juce_wchar colon = ':';
    juce::juce_wchar dash = '-';
    juce::juce_wchar prd = '.';

    bool isNumber = false;
    bool isColon  = false;
    bool isSpace = false;
    bool isDash = false;
    bool isPeriod = false;

    bool inInt = false;
    bool inFloat = false;

    bool previousColon = false;
    bool previousSpace = true;

    bool isEndOfString = false;

    int newindex = 0;
    float newval = 0.;

    juce::Array<float> newarray;
    newarray.ensureStorageAllocated(size);
    // for(int i=0;i<size;i++) newarray.set(i, 0.);
    for(int i=0;i<size;i++) newarray.set(i, init);

    for (int i = 0; i < (s.length()+1); i++)
    {
        juce::juce_wchar c1 = c.getAndAdvance();

        isColon     = !juce::CharacterFunctions::compare(c1, colon);
        isNumber    = juce::CharacterFunctions::isDigit(c1);
        isSpace     = juce::CharacterFunctions::isWhitespace(c1);
        isDash      = !juce::CharacterFunctions::compare(c1, dash);
        isPeriod    = !juce::CharacterFunctions::compare(c1, prd);
        if (i==s.length()) isEndOfString = true;

        //numbers
        if(isNumber && previousSpace) //beginning index read
        {
            inInt = true;
            tempInt += c1;
        }
        else if(isNumber && inInt) //still reading index
        {
            tempInt += c1;
        }
        else if( (isNumber && previousColon) || isDash || isPeriod ) //beginning val read
        {
            inFloat = true;
            tempFloat += c1;
        }
        else if(isNumber && inFloat) //still reading float val
        {
            tempFloat += c1;
        }


        //colons and spaces
        if(isColon)
        {
            previousColon = true;
            inInt = false;
            inFloat = false;

            newindex = tempInt.getIntValue();
            tempInt = "";
        }
        else previousColon = false;

        if(isSpace && previousSpace) //skip repeated spaces
        {
            previousSpace = true;
            inInt = false;
            inFloat = false;
        }
        else if(previousSpace && isEndOfString)
        {
            //do nothing; previousSpace already finalized array
        }
        else if(isSpace || isEndOfString)
        {
            previousSpace = true;
            inInt = false;
            inFloat = false;

            newval = tempFloat.getFloatValue();
            tempFloat = "";

            newarray.set(newindex, newval);
        }
        else previousSpace = false;

    }

    return newarray;
}
void BKAbsoluteKeyboardSlider::textEditorReturnKeyPressed(juce::TextEditor& textEditor)
{
    if(textEditor.getName() == keyboardValsTextField->getName())
    {
        auto array = stringOrderedPairsToFloatArray(keyboardValsTextField->getText(), 128,keyboard->midRange);
        for(int i=0; i<array.size(); i++)
        {
            keyboardState->setKeyOffset(i, array.getUnchecked(i));
        }
        //
        // listeners.call(&BKAbsoluteKeyboardSlider::Listener::keyboardSliderChanged,
        //                getName(),
        //                //keyboard->getValuesRotatedByFundamental());
        //                keyboard->getValues());

        keyboardValsTextField->setAlpha(0);
        keyboardValsTextField->toBack();
        unfocusAllComponents();

    }
    else if(textEditor.getName() == keyboardValueTF.getName())
    {
        if (lastKeyPressed < 0) return;

        //keyboard->setKeyValue(lastKeyPressed, keyboardValueTF.getText().getDoubleValue());

        // listeners.call(&BKAbsoluteKeyboardSlider::Listener::keyboardSliderChanged,
        //                getName(),
        //                //keyboard->getValuesRotatedByFundamental());
        //                keyboard->getValues());

        keyboard->repaint();
    }

}

void BKAbsoluteKeyboardSlider::textEditorEscapeKeyPressed (juce::TextEditor& textEditor)
{
    focusLostByEscapeKey = true;
    if(textEditor.getName() == keyboardValsTextField->getName())
    {
        keyboardValsTextField->setAlpha(0);
        keyboardValsTextField->toBack();
        unfocusAllComponents();
    }
    else if(textEditor.getName() == keyboardValueTF.getName())
    {
        unfocusAllComponents();
    }
}

void BKAbsoluteKeyboardSlider::textEditorFocusLost(juce::TextEditor& textEditor)
{
#if !JUCE_IOS
    if(!focusLostByEscapeKey)
    {
        textEditorReturnKeyPressed(textEditor);
    }
#endif
}

void BKAbsoluteKeyboardSlider::textEditorTextChanged(juce::TextEditor& tf)
{
#if JUCE_IOS
    if (hasBigOne)
    {
        hasBigOne = false;
        textEditorReturnKeyPressed(tf);
    }
#endif
}


void BKAbsoluteKeyboardSlider::buttonClicked (juce::Button* b)
{
    if(b->getName() == keyboardValsTextFieldOpen.getName())
    {

//        keyboardValsTextField->setText(offsetArrayToString3(keyboard->getValues(), midRange), dontSendNotification);
        juce::String s = "";
        int key = 0;
        for (auto offset : keyboardState->tuningOffset)
        {
            //if (offset != 0.0)  s += String(key) + ":" + String((int)(offset*100.0f)) + " ";
            //DBG("offsetArrayToString3 val = " + juce::String(offset));
            if (offset != 0.f)  s += juce::String(key) + ":" + juce::String((offset)) + " ";

            ++key;
        }
        keyboardValsTextField->setText(s,juce::sendNotificationSync);

#if JUCE_IOS
        hasBigOne = true;
        WantsBigOne::listeners.call(&WantsBigOne::Listener::iWantTheBigOne, keyboardValsTextField.get(),
                                    needsOctaveSlider ? "absolute offsets" : "scale offsets");
#else

        focusLostByEscapeKey = false;

        keyboardValsTextField->setAlpha(1);

        keyboardValsTextField->toFront(true);
#endif

    }


}

void BKAbsoluteKeyboardSlider::setValues(juce::Array<float> newvals)
{
    for(int i=0; i<newvals.size(); i++)
    {
        //keyboard->setKeyValue(i, newvals.getUnchecked(i));
    }

    keyboard->repaint();
}
