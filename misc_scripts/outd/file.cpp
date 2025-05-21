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

BKAbsoluteKeyboardSlider::BKAbsoluteKeyboardSlider(bool toggles, bool nos):
needsOctaveSlider(nos),
ratio(1.0)
{
    keyboardComponent = std::make_unique<juce::MidiKeyboardComponent>(keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard);

    addAndMakeVisible (*keyboardComponent);

    keyboard =  (juce::MidiKeyboardComponent*)keyboardComponent.get();

    // need slider or other interface for octave change
#if JUCE_IOS
    minKey = 48; // 21
    maxKey = 72; // 108

    if (needsOctaveSlider)
    {
        octaveSlider.setRange(0, 6, 1);
        octaveSlider.addListener(this);
        octaveSlider.setLookAndFeel(&laf);
        octaveSlider.setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
        octaveSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
        octaveSlider.setValue(3);

        addAndMakeVisible(octaveSlider);
    }
#else
    minKey = 21; // 21
    maxKey = 108; // 108
#endif

    // be default, assume cents, +/- 50
    midRange = 0.;
    minRange = -50.;
    maxRange = 50.;
    displayResolution = 1;

    keyboard->setRepaintsOnMouseActivity(false);
    keyboard->setScrollButtonsVisible(false);
    keyboard->setAvailableRange(minKey, maxKey);
    keyboard->setOctaveForMiddleC(4);
    // keyboard->setAllowDrag(false);
    // keyboard->doKeysToggle(toggles);
    keyboard->addMouseListener(this, true);
    keyboardState.addListener(this);
    lastKeyPressed = 0;

    showName.setText("unnamed keyboard slider", juce::dontSendNotification);
    showName.setJustificationType(juce::Justification::centredRight);
    showName.addMouseListener(this, true);
    addAndMakeVisible(showName);

    keyboardValueTF.setText(juce::String(midRange, displayResolution));
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

void BKAbsoluteKeyboardSlider::paint (juce::Graphics& g)
{

}

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

    juce::Rectangle<int> textSlab (keymapRow.removeFromBottom(2*heightUnit + gYSpacing));
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
    keyboardValueTF.setText(juce::String(keyboard->getLastNoteOverValue(), displayResolution), dontSendNotification);
}

void BKAbsoluteKeyboardSlider::mouseDrag(const juce::MouseEvent& e)
{
    if (disabledKeys.contains(lastKeyPressed))
    {
        DBG("key disabled");
        return;
    }

    if(e.y >= 0 && e.y <= keyboard->getHeight())
    {
        bool isBlackKey = juce::MidiMessage::isMidiNoteBlack (keyboard->getLastKeySelected());
        float dragPos = (float)e.y / keyboard->getHeight();
        if(isBlackKey) dragPos /= keyboard->getBlackNoteLengthProportion();

        dragPos = 1. - 2. * dragPos;
        if(dragPos > 0.) dragPos = dragPos * dragPos;
        else dragPos = -1.* dragPos * dragPos;
        //DBG("BKAbsoluteKeyboardSlider::mouseDrag dragPos = " + juce::String(dragPos));

        float outval;
        if (dragPos > 0) outval = midRange + dragPos * (maxRange - midRange);
        else outval = midRange + dragPos * (midRange - minRange);
        //DBG("BKAbsoluteKeyboardSlider::mouseDrag outval = " + juce::String(outval));

        //keyboardValueTF.setText(juce::String(dragPos * 50.0, 1), dontSendNotification);
        //keyboard->setKeyValue(lastKeyPressed, dragPos * 50.);
        keyboardValueTF.setText(juce::String(outval, displayResolution), dontSendNotification);
        keyboard->setKeyValue(lastKeyPressed, outval);

        //DBG("dragging last key, height " + juce::String(keyboard->getLastKeySelected()) + " " + juce::String(dragPos));
    }
}

void BKAbsoluteKeyboardSlider::mouseUp(const juce::MouseEvent& e)
{
    if(e.mouseWasClicked())
    {
        if(e.mods.isShiftDown())
        {
            keyboardValueTF.setText(juce::String(midRange, displayResolution), dontSendNotification);
            keyboard->setKeyValue(lastKeyPressed, 0.);
        }
    }

    listeners.call(&BKAbsoluteKeyboardSlider::Listener::keyboardSliderChanged,
                   getName(),
                   keyboard->getValues());

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

    lastKeyPressed = keyboard->getLastNoteOver();

}

void BKAbsoluteKeyboardSlider::bkTextFieldDidChange (juce::TextEditor& textEditor)
{

}


void BKAbsoluteKeyboardSlider::textEditorReturnKeyPressed(juce::TextEditor& textEditor)
{
    if(textEditor.getName() == keyboardValsTextField->getName())
    {
        keyboard->setValues(stringOrderedPairsToFloatArray(keyboardValsTextField->getText(), 128, midRange));

        listeners.call(&BKAbsoluteKeyboardSlider::Listener::keyboardSliderChanged,
                       getName(),
                       //keyboard->getValuesRotatedByFundamental());
                       keyboard->getValues());

        keyboardValsTextField->setAlpha(0);
        keyboardValsTextField->toBack();
        unfocusAllComponents();

    }
    else if(textEditor.getName() == keyboardValueTF.getName())
    {
        if (lastKeyPressed < 0) return;

        keyboard->setKeyValue(lastKeyPressed, keyboardValueTF.getText().getDoubleValue());

        listeners.call(&BKAbsoluteKeyboardSlider::Listener::keyboardSliderChanged,
                       getName(),
                       //keyboard->getValuesRotatedByFundamental());
                       keyboard->getValues());

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

void BKAbsoluteKeyboardSlider::handleKeymapNoteToggled (BKKeymapKeyboardState* source, int midiNoteNumber)
{

}

void BKAbsoluteKeyboardSlider::bkButtonClicked (juce::Button* b)
{
    if(b->getName() == keyboardValsTextFieldOpen.getName())
    {

        keyboardValsTextField->setText(offsetArrayToString3(keyboard->getValues(), midRange), dontSendNotification);

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
        keyboard->setKeyValue(i, newvals.getUnchecked(i));
    }

    keyboard->repaint();
}
