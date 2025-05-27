//
// Created by Davis Polito on 5/27/25.
//

#include "BKKeymapKeyboardComponent.h"

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
    if(e.y >= 0 && e.y <= keyboard_.getHeight()) {
        lastKeyPressed =  keyboard_.getNoteAndVelocityAtPosition(e.position).note;
        if (lastKeyPressed != -1)
            keyboard_state_.keyStates.flip(lastKeyPressed);
    }

}
void BKKeymapKeyboardComponent::mouseDrag(const juce::MouseEvent& e) {
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

std::string getOnKeyString(const std::bitset<128>& bits) {
    std::ostringstream oss;
    bool first = true;

    for (size_t i = 0; i < bits.size(); ++i) {
        if (bits.test(i)) {
            if (!first) oss << ' ';
            oss << i;
            first = false;
        }
    }

    return oss.str();
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
void setKeysFromString(std::bitset<128>& bits, const std::string& input) {
    std::istringstream iss(input);
    int key;

    while (iss >> key) {
        if (key >= 0 && key < 128) {
            bits.set(key);
        }
    }
}
void BKKeymapKeyboardComponent::textEditorReturnKeyPressed(juce::TextEditor &) {

}
