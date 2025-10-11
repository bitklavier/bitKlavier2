//
// Created by Davis Polito on 5/27/25.
//

#ifndef BKKEYMAPKEYBOARDCOMPONENT_H
#define BKKEYMAPKEYBOARDCOMPONENT_H

#include "../StateModulatedComponent.h"
#include "BKOnOffKeyboardComponent.h"
#include "KeymapProcessor.h"
#include "juce_audio_utils/juce_audio_utils.h"
#include "TuningUtils.h"

#define SELECT_ID  7
#define DESELECT_ID 8
#define ID(IN) (IN*12)

typedef enum KeySet
{
    KeySetAll = 1,
    KeySetAllPC,
    KeySetBlack,
    KeySetWhite,
    KeySetWholetoneOne,
    KeySetWholetoneTwo,
    KeySetOctatonicOne,
    KeySetOctatonicTwo,
    KeySetOctatonicThree,
    KeySetMajorTriad,
    KeySetMinorTriad,
    KeySetMajorSeven,
    KeySetDomSeven,
    KeySetMinorSeven,
    KeySetMajor,
    KeySetNaturalMinor,
    KeySetHarmonicMinor,
    KeySetNil
} KeySet;

typedef enum OctType
{
    Oct1 = 0,
    Oct2,
    Oct3,
    OctNil
} OctType;

typedef enum WholetoneType
{
    WT1 = 0,
    WT2
} WholetoneType;

class BKKeymapKeyboardComponent : public StateModulatedComponent,
                                  public juce::TextEditor::Listener,
                                  public juce::Button::Listener,
                                  public juce::ComboBox::Listener {
public:
    BKKeymapKeyboardComponent(KeymapKeyboardState* keyboard_state, bool helperButtons = true) :
                  StateModulatedComponent(juce::ValueTree{}),
                  keyboard_state_(*keyboard_state),
                  keyboard_(BKOnOffKeyboardComponent::horizontalKeyboard, keyboard_state->keyStates){

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


        clearButton.setName("KSLIDERCLEAR");
        clearButton.addListener(this);
        clearButton.setButtonText("all off");
        clearButton.setTooltip("clear all selected keys");


        allOnButton.setName("KSLIDERALLON");
        allOnButton.addListener(this);
        allOnButton.setButtonText("all on");
        allOnButton.setTooltip("select all");


        keysCB.setName("keysCB");
        keysCB.addListener(this);
        keysCB.setTooltip("Choose between 'select' or 'deselect' for batch operations (under Keys)");
        keysCB.addItem("Select", SELECT_ID);
        keysCB.addItem("Deselect", DESELECT_ID);
        keysCB.setSelectedId(SELECT_ID);


        keysButton.setName("KSLIDERKEYSBUTTON");
        keysButton.addListener(this);
        keysButton.setButtonText("keys");
        keysButton.setTooltip("macros for selecting particular keys");


        keyboardValsTextField->setInterceptsMouseClicks(false, false);
        setInterceptsMouseClicks(true, true);

        if (helperButtons)
        {
            addAndMakeVisible(keyboardValsTextFieldOpen);
            addAndMakeVisible(clearButton);
            addAndMakeVisible(allOnButton);
            addAndMakeVisible(keysCB);
            addAndMakeVisible(keysButton);
        }

    }

    ~BKKeymapKeyboardComponent() {
        keyboardValsTextField->setLookAndFeel(nullptr);
        keyboardValsTextField->removeListener(this);
        keyboardValsTextField.reset();
        keyboard_.setLookAndFeel(nullptr);
    }

    void syncToValueTree() override {}

    BKKeymapKeyboardComponent* clone () {
        return nullptr;
    }

    void mouseUp(const juce::MouseEvent &event) override;
    void mouseDown(const juce::MouseEvent &event) override;
    void mouseDrag(const juce::MouseEvent &event) override;
    void resized() override;
    void buttonClicked(juce::Button *) override;
    void comboBoxChanged (juce::ComboBox* comboBoxThatHasChanged) override;
    void textEditorReturnKeyPressed(juce::TextEditor &) override;
    void textEditorFocusLost(juce::TextEditor &) override;
    void textEditorEscapeKeyPressed(juce::TextEditor &) override;
    juce::PopupMenu getKeysMenu(void);
    juce::PopupMenu getPitchClassMenu(int offset);
    void keysMenuCallback(int result, BKKeymapKeyboardComponent* vc);

    void setWhite(bool action);
    void setBlack(bool action);
    void setChord(KeySet set, PitchClass root);
    void setOctatonic(OctType type);
    void setWholetone(WholetoneType type);

    KeymapKeyboardState& keyboard_state_;
    BKOnOffKeyboardComponent keyboard_;
    juce::TextButton keyboardValsTextFieldOpen;
    std::unique_ptr<juce::TextEditor> keyboardValsTextField;

    juce::TextButton clearButton;
    juce::TextButton allOnButton;
    juce::TextButton keysButton;
    juce::ComboBox keysCB;

    /*
     * todo: need
     *  - Select/Deselect menu
     *  - Keys menu
     *  - Clear button
     *  - Midi Edit (E) toggle
     *
     *  and then velocity curves
     *
     *  keymap should default to having all keys on
     */


    int keyboardSize, minKey, maxKey;
    int lastKeyPressed = -1;

    // bool to track the state of the "select/deselect" button
    // select = 0/false, deselect = 1/true
    bool deselectKey = false;

    juce::String pcs[12] = {"C","C#/Db","D","D#/Eb","E","F","F#/Gb","G","G#/Ab","A","A#/Bb","B",};

    juce::Array<int> white = {0,2,4,5,7,9,11};
    juce::Array<int> black = {1,3,6,8,10};

    juce::Array<int> wholetone1 = {0,2,4,6,8,10};
    juce::Array<int> wholetone2 = {1,3,5,7,9,11};

    juce::Array<int> octatonic1 = {0,1,3,4,6,7,9,10};
    juce::Array<int> octatonic2 = {1,2,4,5,7,8,10,11};
    juce::Array<int> octatonic3 = {0,2,3,5,6,8,9,11};

    juce::Array<int> majortriad = {0,4,7};
    juce::Array<int> minortriad = {0,3,7};
    juce::Array<int> majorseven = {0,4,7,11};
    juce::Array<int> domseven = {0,4,7,10};
    juce::Array<int> minorseven = {0,3,7,10};

    juce::Array<int> major = {0,2,4,5,7,9,11};
    juce::Array<int> naturalminor = {0,2,3,5,7,8,10};
    juce::Array<int> harmonicminor = {0,2,3,5,7,8,11};

    juce::Array<int> allpc = {0};
};
#endif //BKKEYMAPKEYBOARDCOMPONENT_H
