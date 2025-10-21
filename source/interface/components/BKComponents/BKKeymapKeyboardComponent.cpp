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

    float keyWidth = keymapRow.getWidth() / round((maxKey - minKey) * 7./12 + 1); //num white keys
    keyboard_.setKeyWidth(keyWidth);
    keyboard_.setBlackNoteLengthProportion(0.65);

    juce::Rectangle<int> keyboardRect = keymapRow.removeFromBottom(keyboardHeight);

    keyboard_.setBounds(keyboardRect);

    if(useHelperButtons)
    {
        keymapRow.removeFromBottom(2);
        juce::Rectangle<int> textSlab (keymapRow.removeFromBottom(2 * heightUnit + 4));
        keyboardValsTextFieldOpen.setBounds(textSlab.removeFromRight(widthUnit));

        allOnButton.setBounds(textSlab.removeFromLeft(widthUnit));
        textSlab.removeFromLeft(4);
        clearButton.setBounds(textSlab.removeFromLeft(widthUnit));

        textSlab.removeFromLeft(20);
        keysCB.setBounds(textSlab.removeFromLeft(widthUnit));

        textSlab.removeFromLeft(4);
        keysButton.setBounds(textSlab.removeFromLeft(widthUnit));

        keyboardValsTextField->setBounds(keyboard_.getBounds());
    }
}

void BKKeymapKeyboardComponent::mouseUp(const juce::MouseEvent& e) {
    keyboard_.repaint();
}

void BKKeymapKeyboardComponent::mouseDown(const juce::MouseEvent& e) {
    if (keyboardValsTextField->hasKeyboardFocus(false)) {
        keyboardValsTextField->mouseDown(e);
    }
    else if(e.y >= 0 && e.y <= keyboard_.getHeight()) {
        lastKeyPressed = keyboard_.getNoteAndVelocityAtPosition(e.position).note;
        if (lastKeyPressed != -1)
        {
            if (isMonophonic)
            {
                keyboard_state_.keyStates.reset();
                keyboard_state_.keyStates.set(lastKeyPressed);
            }
            else
                keyboard_state_.keyStates.flip(lastKeyPressed);

            listeners.call(&BKKeymapKeyboardComponent::Listener::BKKeymapKeyboardChanged,
                getName(),
                keyboard_state_.keyStates,
                lastKeyPressed);
        }
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
            {
                if (isMonophonic)
                {
                    keyboard_state_.keyStates.reset();
                    keyboard_state_.keyStates.set(lastKeyPressed);
                }
                else
                    keyboard_state_.keyStates.flip(lastKeyPressed);

                listeners.call(&BKKeymapKeyboardComponent::Listener::BKKeymapKeyboardChanged,
                    getName(),
                    keyboard_state_.keyStates,
                    lastKeyPressed);
            }
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
    else if (button == &clearButton) {
        keyboard_state_.keyStates.reset();
    }
    else if (button == &allOnButton) {
        keyboard_state_.keyStates.set();
    }
    else if (button == &keysButton)
    {
        getKeysMenu().showMenuAsync(
        juce::PopupMenu::Options().withTargetComponent(&keysButton),

        // FIX: Use juce::ModalCallbackFunction::create with a lambda that captures 'this'.
        juce::ModalCallbackFunction::create(
            // The lambda captures 'this' to access the non-static member function
            [this](int result)
            {
                // Now, you can safely call your original member function,
                // passing 'this' for the 'vc' argument if it expects the component pointer.
                if (result != 0)
                {
                    this->keysMenuCallback(result, this);
                    this->resized(); // triggers update of keyStates
                }
            })
        );
    }
}

void BKKeymapKeyboardComponent::keysMenuCallback(int result, BKKeymapKeyboardComponent* vc)
{
    int set = result / 12;
    int pc = result % 12;
    DBG("set: " + juce::String(set) + " pc: " + juce::String(pc));

    switch (set) {
        case KeySetBlack :
            setBlack(deselectKey);
            break;

        case KeySetWhite :
            setWhite(deselectKey);
            break;

        case KeySetAllPC :
            setChord(static_cast<KeySet>(set), getPitchClassFromInt(pc));
            break;

        case KeySetWholetoneOne :
            setWholetone(WT1);
            break;

        case KeySetWholetoneTwo :
            setWholetone(WT2);
            break;

        case KeySetOctatonicOne :
            setOctatonic(Oct1);
            break;

        case KeySetOctatonicTwo :
            setOctatonic(Oct2);
            break;

        case KeySetOctatonicThree :
            setOctatonic(Oct3);
            break;

        default :
            setChord(static_cast<KeySet>(set), getPitchClassFromInt(pc));
            break;
    }
}

void BKKeymapKeyboardComponent::comboBoxChanged (juce::ComboBox* comboBoxThatHasChanged)
{
    if(comboBoxThatHasChanged == &keysCB)
    {
        deselectKey = keysCB.getSelectedItemIndex();
    }
}

/*
 * todo: add whole tone scales
 */
juce::PopupMenu BKKeymapKeyboardComponent::getKeysMenu(void)
{
    juce::PopupMenu menu;

   // menu.addItem(ID(KeySetAll), "All");
    menu.addSubMenu("Pitches:",  getPitchClassMenu((KeySet) ID(KeySetAllPC)));

    menu.addItem(ID(KeySetBlack), "Black");
    menu.addItem(ID(KeySetWhite), "White");

    menu.addItem(ID(KeySetWholetoneOne), "Whole Tone 1");
    menu.addItem(ID(KeySetWholetoneTwo), "Whole Tone 2");

    menu.addItem(ID(KeySetOctatonicOne), "Octatonic 1");
    menu.addItem(ID(KeySetOctatonicTwo), "Octatonic 2");
    menu.addItem(ID(KeySetOctatonicThree), "Octatonic 3");

    menu.addSubMenu("Major Triad",  getPitchClassMenu((KeySet) ID(KeySetMajorTriad)));
    menu.addSubMenu("Minor Triad",  getPitchClassMenu((KeySet) ID(KeySetMinorTriad)));
    menu.addSubMenu("Major Seven",  getPitchClassMenu((KeySet) ID(KeySetMajorSeven)));
    menu.addSubMenu("Dom Seven",    getPitchClassMenu((KeySet) ID(KeySetDomSeven)));
    menu.addSubMenu("Minor Seven",  getPitchClassMenu((KeySet) ID(KeySetMinorSeven)));

    menu.addSubMenu("Major", getPitchClassMenu((KeySet) ID(KeySetMajor)));
    menu.addSubMenu("Natural Minor", getPitchClassMenu((KeySet) ID(KeySetNaturalMinor)));
    menu.addSubMenu("Harmonic Minor", getPitchClassMenu((KeySet) ID(KeySetHarmonicMinor)));

    return std::move(menu);
}

void BKKeymapKeyboardComponent::setWhite(bool action)
{
    int pc;
    for (int note = 0; note < 128; note++)
    {
        pc = note % 12;

        if (white.contains(pc))
        {
            keyboard_state_.keyStates[note] = !deselectKey;
        }
    }

}

void BKKeymapKeyboardComponent::setBlack(bool action)
{
    int pc;
    for (int note = 0; note < 128; note++)
    {
        pc = note % 12;

        if (black.contains(pc))
        {
            keyboard_state_.keyStates[note] = !deselectKey;
        }
    }
}

void BKKeymapKeyboardComponent::setChord(KeySet set, PitchClass root)
{
    int pc;
    juce::Array<int> chord;
    if      (set == KeySetMajorTriad)          chord = majortriad;
    else if (set == KeySetMinorTriad)          chord = minortriad;
    else if (set == KeySetMajorSeven)          chord = majorseven;
    else if (set == KeySetDomSeven)            chord = domseven;
    else if (set == KeySetMinorSeven)          chord = minorseven;
    else if (set == KeySetAllPC)               chord = allpc;
    else if (set == KeySetMajor)               chord = major;
    else if (set == KeySetNaturalMinor)        chord = naturalminor;
    else if (set == KeySetHarmonicMinor)       chord = harmonicminor;
    else                                       return;

    for (int note = 0; note < 128; note++)
    {
        pc = ((note - intFromPitchClass(root)) % 12);

        if (chord.contains(pc))
        {
            //keymap.set(note, action);
            keyboard_state_.keyStates[note] = !deselectKey;
        }
    }
}

void BKKeymapKeyboardComponent::setOctatonic(OctType type)
{
    int pc;
    juce::Array<int> octatonic;
    if      (type == Oct1)  octatonic = octatonic1;
    else if (type == Oct2)  octatonic = octatonic2;
    else if (type == Oct3)  octatonic = octatonic3;
    else return;

    for (int note = 0; note < 128; note++)
    {
        pc = note % 12;

        if (octatonic.contains(pc))
        {
            keyboard_state_.keyStates[note] = !deselectKey;
        }
    }
}

void BKKeymapKeyboardComponent::setWholetone(WholetoneType type)
{
    int pc;
    juce::Array<int> wholet;
    if      (type == WT1)  wholet = wholetone1;
    else if (type == WT2)  wholet = wholetone2;
    else return;

    for (int note = 0; note < 128; note++)
    {
        pc = note % 12;

        if (wholet.contains(pc))
        {
            keyboard_state_.keyStates[note] = !deselectKey;
        }
    }
}

juce::PopupMenu BKKeymapKeyboardComponent::getPitchClassMenu(int offset)
{
    int Id;

    juce::PopupMenu menu;

    for (int i = 0; i < 12; i++)
    {
        Id = offset + i;
        DBG("ID: " + juce::String(Id));
        menu.addItem(Id, pcs[i]);
    }

    return std::move(menu);
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
