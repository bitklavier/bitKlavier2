//
// Created by Davis Polito on 5/27/25.
//

#ifndef BKKEYMAPKEYBOARDCOMPONENT_H
#define BKKEYMAPKEYBOARDCOMPONENT_H

#include "../StateModulatedComponent.h"
#include "BKOnOffKeyboardComponent.h"
#include "KeymapProcessor.h"
#include "juce_audio_utils/juce_audio_utils.h"

class BKKeymapKeyboardComponent : public StateModulatedComponent,
                                  public juce::TextEditor::Listener,
                                  public juce::Button::Listener {
public:
    BKKeymapKeyboardComponent(KeymapKeyboardState* keyboard_state) : StateModulatedComponent(juce::ValueTree{}),
    keyboard_state_(*keyboard_state), keyboard_(BKOnOffKeyboardComponent::horizontalKeyboard,keyboard_state->keyStates){

        minKey = 21; // 21

        maxKey = 108; // 108
        keyboard_.setAvailableRange(minKey, maxKey);
        keyboard_.setRepaintsOnMouseActivity(false);
        keyboard_.setScrollButtonsVisible(false);
        keyboard_.setAvailableRange(minKey, maxKey);
        keyboard_.setOctaveForMiddleC(4);

        // keyboard->setAllowDrag(false);
        // keyboard->doKeysToggle(toggles);
        keyboard_.addMouseListener(this, true);
        addAndMakeVisible(keyboard_);
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
        keyboardValsTextField->setInterceptsMouseClicks(false, false);
        setInterceptsMouseClicks(true, true);
    }

    ~BKKeymapKeyboardComponent() {
        keyboardValsTextField->setLookAndFeel(nullptr);
        keyboardValsTextField->removeListener(this);
        keyboardValsTextField.reset();
        keyboard_.setLookAndFeel(nullptr);
    }

    void syncToValueTree() override
    {}
    BKKeymapKeyboardComponent* clone () {
        return nullptr;
    }

    // to assist in placing other components on top of keyboard and to the right of the edit button
    int getKeyboardTop() { return keyboard_.getY(); }
    int getEditAllTextButtonRight() { return keyboardValsTextFieldOpen.getRight(); }

    void mouseUp(const juce::MouseEvent &event) override;
    void mouseDown(const juce::MouseEvent &event) override;
    void mouseDrag(const juce::MouseEvent &event) override;
    void resized() override;
    void buttonClicked(juce::Button *) override;
    void textEditorReturnKeyPressed(juce::TextEditor &) override;
    void textEditorFocusLost(juce::TextEditor &) override;
    void textEditorEscapeKeyPressed(juce::TextEditor &) override;

    KeymapKeyboardState& keyboard_state_;
    BKOnOffKeyboardComponent keyboard_;
    juce::TextButton keyboardValsTextFieldOpen;
    std::unique_ptr<juce::TextEditor> keyboardValsTextField;
    int keyboardSize, minKey, maxKey;
    int lastKeyPressed = -1;
};
#endif //BKKEYMAPKEYBOARDCOMPONENT_H
