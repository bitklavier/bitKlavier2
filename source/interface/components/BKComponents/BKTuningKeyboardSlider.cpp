//
// Created by Davis Polito on 5/19/25.
//
/*
  ==============================================================================

    BKTuningKeyboardSlider.cpp
    Created: 29 Jul 2018 4:40:55pm
    Author:  Daniel Trueman

  ==============================================================================
*/

#include "BKTuningKeyboardSlider.h"

#include "chowdsp_core/third_party/span-lite/test/lest/lest_cpp03.hpp"
#include "juce_core/juce_core.h"
void KeyboardOffsetComponent::drawBlackKey(int midiNoteNumber, juce::Graphics &g, juce::Rectangle<float> area) {

    juce::Colour c (juce::Colours::black);

    g.setColour (c);
    g.fillRect (area);

    float keyVal = isCircular ? state.circularTuningOffset[midiNoteNumber] : state.absoluteTuningOffset[midiNoteNumber];
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


}

void KeyboardOffsetComponent::drawWhiteKey(int midiNoteNumber, juce::Graphics &g, juce::Rectangle<float> area) {

    auto c = juce::Colours::transparentWhite;
    float keyVal = isCircular ? state.circularTuningOffset[midiNoteNumber] : state.absoluteTuningOffset[midiNoteNumber];


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
BKTuningKeyboardSlider::BKTuningKeyboardSlider(TuningKeyboardState* state,bool toggles, bool nos,bool isCircular):
needsOctaveSlider(nos),
ratio(1.0),
keyboardState(state), isCircular(isCircular)
{
    keyboard = std::make_unique<KeyboardOffsetComponent>(*keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard,isCircular);

    addAndMakeVisible (*keyboard);


    // need slider or other interface for octave change

    minKey = 21; // 21
    maxKey = 108; // 108
    displayResolution = 1;
    if (isCircular) {
        minKey = 0;
        maxKey = 11;
    }

    keyboard->setRepaintsOnMouseActivity(false);
    keyboard->setScrollButtonsVisible(false);
    keyboard->setAvailableRange(minKey, maxKey);
    keyboard->setOctaveForMiddleC(4);

    // keyboard->setAllowDrag(false);
    // keyboard->doKeysToggle(toggles);
    keyboard->addMouseListener(this, true);
   // keyboardState->keyboardState.addListener(this);
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
    keyboardValsTextField->setInterceptsMouseClicks(false, false);
    setInterceptsMouseClicks(true, true);
    addAndMakeVisible(keyboardValsTextFieldOpen);
    keyboardValueTF.addMouseListener(this, true);
}




void BKTuningKeyboardSlider::resized()
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


void BKTuningKeyboardSlider::setAvailableRange(int min, int max)
{
    minKey = min;
    maxKey = max;
    keyboardSize = max - min; //

    //all of the above unnecessary?
    keyboard->setAvailableRange(minKey, maxKey);
}

void BKTuningKeyboardSlider::mouseMove(const juce::MouseEvent& e)
{
   // keyboardValueTF.setText(juce::String(keyboard->getLastNoteOverValue(), displayResolution), dontSendNotification);
}

void BKTuningKeyboardSlider::mouseDrag(const juce::MouseEvent& e)
{
    if (e.originalComponent == &keyboardValueTF) {
        return;
    }
    if (keyboardValsTextField->hasKeyboardFocus(false)) {
        keyboardValsTextField->mouseDrag(e);
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
        DBG("BKTuningKeyboardSlider::mouseDrag dragPos = " + juce::String(dragPos));

        float outval;
        if (dragPos > 0) outval = keyboard->midRange + dragPos * (keyboard->maxRange - keyboard->midRange);
        else outval = keyboard->midRange + dragPos * (keyboard->midRange - keyboard->minRange);
        DBG("BKTuningKeyboardSlider::mouseDrag outval = " + juce::String(outval));
        auto val  = std::clamp(outval, keyboard->minRange, keyboard->maxRange);
       // keyboardState->setKeyOffset(myNote, dragPos * 50.);
        keyboardValueTF.setText(juce::String(val, displayResolution), juce::dontSendNotification);
        keyboardState->setKeyOffset(myNote, val,isCircular);

        //DBG("dragging last key, height " + juce::String(keyboard->getLastKeySelected()) + " " + juce::String(dragPos));
    }
}

void BKTuningKeyboardSlider::mouseUp(const juce::MouseEvent& e)
{
    if(e.mouseWasClicked())
    {
        if(e.mods.isShiftDown())
        {
            keyboardValueTF.setText(juce::String(keyboard->midRange, displayResolution), juce::dontSendNotification);
            keyboardState->setKeyOffset(lastKeyPressed, 0.,isCircular);
        }
    }

    // listeners.call(&BKTuningKeyboardSlider::Listener::keyboardSliderChanged,
    //                getName(),
    //                keyboard->getValues());
    keyboard->repaint();
}

void BKTuningKeyboardSlider::mouseDoubleClick(const juce::MouseEvent& e)
{
}

void BKTuningKeyboardSlider::mouseDown(const juce::MouseEvent& e)
{
    if (e.originalComponent == &keyboardValueTF) {
        return;
    }
    if(e.y >= 0 && e.y <= keyboard->getHeight())
        lastKeyPressed =  keyboard->getNoteAndVelocityAtPosition(e.position).note;
    else
        lastKeyPressed = -1;
    if (keyboardValsTextField->hasKeyboardFocus(false)) {
        keyboardValsTextField->mouseDown(e);
    }
    // if (keyboardValueTF.hasKeyboardFocus(false)) {
    //     keyboardValueTF.mouseDown(e);
    // }

}



juce::Array<float> stringToFloatArray(juce::String s)
{
    juce::Array<float> arr = juce::Array<float>();

    juce::String temp = "";
    bool inNumber = false;

    juce::String::CharPointerType c = s.getCharPointer();

    juce::juce_wchar prd = '.';
    juce::juce_wchar dash = '-';
    juce::juce_wchar slash = '/'; // blank: put a zero in

    int prdCnt = 0;

    // DEBUG
    for (int i = 0; i < (s.length()+1); i++)
    {
        juce::juce_wchar c1 = c.getAndAdvance();

        bool isPrd = !juce::CharacterFunctions::compare(c1, prd);
        bool isDash = !juce::CharacterFunctions::compare(c1, dash);
        bool isSlash = !juce::CharacterFunctions::compare(c1, slash);

        if (isPrd) prdCnt += 1;

        bool isNumChar = juce::CharacterFunctions::isDigit(c1) || isPrd || isDash;

        if (!isNumChar)
        {
            if (inNumber)
            {
                arr.add(temp.getFloatValue());
                temp = "";
            }

            // slash indicates a zero slot
            if (isSlash) {
                arr.add(0.);
                temp = "";
            }

            inNumber = false;
            continue;
        }
        else
        {
            inNumber = true;

            temp += c1;
        }
    }

    return arr;
}
void BKTuningKeyboardSlider::textEditorReturnKeyPressed(juce::TextEditor& textEditor)
{
    if(textEditor.getName() == keyboardValsTextField->getName())
    {
        if (isCircular) {
             auto   tempVals = parseFloatStringToArrayCircular<12>(keyboardValsTextField->getText().toStdString());
            if (tempVals.size() == maxKey - 1) {
                int offset;
                if(keyboardState->fundamental <= 0) offset = 0;
                else offset = keyboardState->fundamental;
                auto rangeAll  =  (keyboard->getRangeEnd() - keyboard->getRangeStart()) + 1;
                for(int i=keyboard->getRangeStart(); i<=keyboard->getRangeEnd(); i++)
                {
                    int index = ((i - offset) + rangeAll) % rangeAll;
                    keyboardState->setKeyOffset(index, tempVals[index],isCircular);
                }
            }

        }else {
            auto array = parseIndexValueStringToArrayAbsolute<128>(keyboardValsTextField->getText().toStdString());
            for(int i=0; i<array.size(); i++)
            {
                keyboardState->setKeyOffset(i, array[i],isCircular);
            }

            //
            // listeners.call(&BKTuningKeyboardSlider::Listener::keyboardSliderChanged,
            //                getName(),
            //                //keyboard->getValuesRotatedByFundamental());
            //                keyboard->getValues());
        }
        keyboardValsTextField->setAlpha(0);
        keyboardValsTextField->toBack();
        unfocusAllComponents();

    }
    else if(textEditor.getName() == keyboardValueTF.getName())
    {
        if (lastKeyPressed < 0) return;

        keyboardState->setKeyOffset(lastKeyPressed, keyboardValueTF.getText().getDoubleValue(),isCircular);

        // listeners.call(&BKTuningKeyboardSlider::Listener::keyboardSliderChanged,
        //                getName(),
        //                //keyboard->getValuesRotatedByFundamental());
        //                keyboard->getValues());

        keyboard->repaint();
    }

}

void BKTuningKeyboardSlider::textEditorEscapeKeyPressed (juce::TextEditor& textEditor)
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

void BKTuningKeyboardSlider::textEditorFocusLost(juce::TextEditor& textEditor)
{
    if(!focusLostByEscapeKey)
    {
        textEditorReturnKeyPressed(textEditor);
    }

}

void BKTuningKeyboardSlider::textEditorTextChanged(juce::TextEditor& tf)
{
}


void BKTuningKeyboardSlider::buttonClicked (juce::Button* b)
{
    if(b->getName() == keyboardValsTextFieldOpen.getName())
    {

        if (isCircular) {
            //        keyboardValsTextField->setText(offsetArrayToString3(keyboard->getValues(), midRange), dontSendNotification);
            juce::String s = "";

            for (auto offset : keyboardState->circularTuningOffset) {
                s += juce::String((offset)) + " ";
            }
            keyboardValsTextField->setText(s,juce::sendNotificationSync);
        }
        else {
            //        keyboardValsTextField->setText(offsetArrayToString3(keyboard->getValues(), midRange), dontSendNotification);
            juce::String s = "";
            int key = 0;
            for (auto offset : keyboardState->absoluteTuningOffset)
            {
                //if (offset != 0.0)  s += String(key) + ":" + String((int)(offset*100.0f)) + " ";
                //DBG("offsetArrayToString3 val = " + juce::String(offset));
                if (offset != 0.f)  s += juce::String(key) + ":" + juce::String((offset)) + " ";

                ++key;
            }

            keyboardValsTextField->setText(s,juce::sendNotificationSync);
        }
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

void BKTuningKeyboardSlider::setValues(juce::Array<float> newvals)
{
    for(int i=0; i<newvals.size(); i++)
    {
        //keyboard->setKeyValue(i, newvals.getUnchecked(i));
    }

    keyboard->repaint();
}
