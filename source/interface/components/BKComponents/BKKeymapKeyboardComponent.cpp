//
// Created by Davis Polito on 5/27/25.
//

#include "BKKeymapKeyboardComponent.h"
#include "array_to_string.h"
void BKKeymapKeyboardComponent::resized() {
    float heightUnit = getHeight() * 0.1;
    float widthUnit = getWidth() * 0.1;

    juce::Rectangle<int> area (getLocalBounds());
    float keyboardHeight = 8 * heightUnit;
    juce::Rectangle<int> keymapRow = area.removeFromBottom(10 * heightUnit);

    //absolute keymap
    float keyWidth = keymapRow.getWidth() / round((maxKey - minKey) * 7./12 + 1); //num white keys
    keyboard_.setKeyWidth(keyWidth);
    keyboard_.setBlackNoteLengthProportion(0.65);

    juce::Rectangle<int> keyboardRect = keymapRow.removeFromBottom(keyboardHeight);


    keyboard_.setBounds(keyboardRect);

    juce::Rectangle<int> textSlab (keymapRow.removeFromBottom(2*heightUnit + 4));
    keyboardValsTextFieldOpen.setBounds(textSlab.removeFromLeft(widthUnit*1.5));

    keyboardValsTextField->setBounds(keyboard_.getBounds());

}

void BKKeymapKeyboardComponent::mouseUp(const juce::MouseEvent& e) {
    keyboard_.repaint();
}
void BKKeymapKeyboardComponent::mouseDown(const juce::MouseEvent& e) {
    if (keyboardValsTextField->hasKeyboardFocus(false)) {
        keyboardValsTextField->mouseDown(e);
    }
    else if(e.y >= 0 && e.y <= keyboard_.getHeight()) {
        lastKeyPressed =  keyboard_.getNoteAndVelocityAtPosition(e.position).note;
        if (lastKeyPressed != -1)
            keyboard_state_.keyStates.flip(lastKeyPressed);
    }


}
void BKKeymapKeyboardComponent::mouseDrag(const juce::MouseEvent& e) {
    if (keyboardValsTextField->hasKeyboardFocus(false)) {
        keyboardValsTextField->mouseDrag(e);
        return;
    }
    if(e.y >= 0 && e.y <= keyboard_.getHeight()) {
       auto key = keyboard_.getNoteAndVelocityAtPosition(e.position).note;
        if (key != lastKeyPressed)
        {


            lastKeyPressed = key;
            if (lastKeyPressed != -1)
                keyboard_state_.keyStates.flip(lastKeyPressed);
        }
    }
    keyboard_.repaint();

}


void BKKeymapKeyboardComponent::buttonClicked(juce::Button* button) {
    if (button == &keyboardValsTextFieldOpen) {
       auto onKeys = getOnKeyString(keyboard_state_.keyStates);
       keyboardValsTextField->setText(onKeys, juce::dontSendNotification);
       keyboardValsTextField->setAlpha(1);
       keyboardValsTextField->toFront(true);

       keyboardValsTextField->selectAll();
       keyboardValsTextField->setCaretPosition(0);
       keyboardValsTextField->setCaretVisible(true);
       keyboardValsTextField->setColour(juce::TextEditor::outlineColourId, juce::Colours::black);
    }

}

void BKKeymapKeyboardComponent::textEditorReturnKeyPressed(juce::TextEditor &textEditor) {
    if(textEditor.getName() == keyboardValsTextField->getName())
    {
        auto toString  = textEditor.getText().toStdString();
        //also used in keymapprocesor deserialzie TODO - make a function
        std::bitset<128> bits;
        std::istringstream iss(toString);
        int key;

        while (iss >> key) {
            if (key >= 0 && key < 128) {
                bits.set(key);
            }
        }
        keyboard_state_.keyStates = bits;
        keyboardValsTextField->setAlpha(0);
        keyboardValsTextField->toBack();
        unfocusAllComponents();

    }
}
void BKKeymapKeyboardComponent::textEditorFocusLost(juce::TextEditor &textEditor) {

        textEditorReturnKeyPressed(textEditor);
}
void BKKeymapKeyboardComponent::textEditorEscapeKeyPressed(juce::TextEditor &) {
    keyboardValsTextField->setAlpha(0);
    keyboardValsTextField->toBack();
    unfocusAllComponents();
}
