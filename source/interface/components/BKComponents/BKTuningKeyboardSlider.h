/*
  ==============================================================================

    BKTuningKeyboardSlider.h
    Created: 29 Jul 2018 4:40:55pm
    Author:  Daniel Trueman

  ==============================================================================
*/

#pragma once

#include "../StateModulatedComponent.h"
#include "BKSliders.h"
#include "TuningProcessor.h"
typedef enum KSliderTextFieldType
{
    KSliderAllValues,
    KSliderThisValue,
    KSliderTextFieldTypeNil,
} KSliderTextFieldType;

class KeyboardOffsetComponent : public juce::KeyboardComponentBase

{
public:
    enum ColourIds
    {
        whiteNoteColourId               = 0x1005000,
        blackNoteColourId               = 0x1005001,
        keySeparatorLineColourId        = 0x1005002,
        mouseOverKeyOverlayColourId     = 0x1005003,  /**< This colour will be overlaid on the normal note colour. */
        keyDownOverlayColourId          = 0x1005004,  /**< This colour will be overlaid on the normal note colour. */
        textLabelColourId               = 0x1005005,
        upDownButtonBackgroundColourId  = 0x1005006,
        upDownButtonArrowColourId       = 0x1005007,
        shadowColourId                  = 0x1005008
    };
    KeyboardOffsetComponent(TuningKeyboardState &state, Orientation o,bool isCircular= false) : juce::KeyboardComponentBase(o),state(state),isCircular(isCircular) {
           setColour(whiteNoteColourId, juce::Colours::white);
            setColour(blackNoteColourId, juce::Colours::black);
        setColour(keySeparatorLineColourId, juce::Colours::grey);
        setColour(mouseOverKeyOverlayColourId, juce::Colours::white.withAlpha(0.5f));
        setColour(keyDownOverlayColourId, juce::Colours::white.withAlpha(0.5f));
        setColour(textLabelColourId, juce::Colours::black);
        setColour(upDownButtonBackgroundColourId, juce::Colours::grey);
        setColour(upDownButtonArrowColourId, juce::Colours::black);
        setColour(shadowColourId, juce::Colours::black.withAlpha(0.5f));
    }
    ~KeyboardOffsetComponent() {
    }

    void drawWhiteKey(int midiNoteNumber, juce::Graphics &g, juce::Rectangle<float> area) override;
    void drawBlackKey(int midiNoteNumber, juce::Graphics &g, juce::Rectangle<float> area) override;
    void drawKeyboardBackground(juce::Graphics &g, juce::Rectangle<float> area) override;
    float midRange = 0.f;
    float minRange = -50.f;
    float maxRange = 50.f;
    TuningKeyboardState &state;
    bool isCircular;
};


class BKTuningKeyboardSlider :
public StateModulatedComponent,
public juce::MidiKeyboardState::Listener,
public juce::TextEditor::Listener,
public juce::Button::Listener
{

public:

    BKTuningKeyboardSlider(TuningKeyboardState* state, bool toggles, bool needsOctaveSlider = false, bool isCircular = false);
    ~BKTuningKeyboardSlider()
    {
        keyboard = nullptr;
        setLookAndFeel(nullptr);
    };

    void resized() override;

    class Listener
    {

    public:
        virtual ~Listener() {};

        virtual void keyboardSliderChanged(juce::String name)= 0;

    };

    juce::ListenerList<Listener> listeners;
    void addMyListener(Listener* listener)     { listeners.add(listener); }
    void removeMyListener(Listener* listener)  { listeners.remove(listener); }

    inline void setText(juce::String text)
    {
        keyboardValueTF.setText(text, false);
    }

    inline juce::TextEditor* getTextEditor(KSliderTextFieldType which)
    {
        if (which == KSliderAllValues) return keyboardValsTextField.get();
        if (which == KSliderThisValue) return &keyboardValueTF;

        return nullptr;
    }

    inline void dismissTextEditor(juce::TextEditor* which, bool setValue = false)
    {
        if (setValue)
        {
            textEditorReturnKeyPressed(*which);
        }
        else
        {
            textEditorEscapeKeyPressed(*which);
        }
    }

    void setName(juce::String newName) { sliderName = newName; showName.setText(sliderName, juce::dontSendNotification); }
    juce::String getName() { return sliderName; }

    void setAvailableRange(int min, int max);
    void setValues(juce::Array<float> newvals);

    void setMinMidMaxValues(float min, float mid, float max, int resolution)
    {
        if (min > mid || min > max || mid > max) {
            DBG("min must be < mid must be < max");
            return;
        }

        keyboard->minRange = min;
        keyboard->midRange = mid;
        keyboard->maxRange = max;
        displayResolution = resolution;

        //keyboard->setMinMidMaxValues(min, mid, max);
    }





    void setOctaveForMiddleC(int octave) { keyboard->setOctaveForMiddleC(octave);};

    inline void setDimensionRatio(float r) { ratio = r; }

    inline juce::Rectangle<float> getEditAllBounds(void) { return keyboardValsTextFieldOpen.getBounds().toFloat();}

    void setDim(float newalpha)
    {
        showName.setAlpha(newalpha);
        keyboard->setAlpha(newalpha);
        keyboardValsTextFieldOpen.setAlpha(newalpha);
        keyboardValueTF.setAlpha(newalpha);
    }

    void setBright() { setDim(1.); }

    BKTuningKeyboardSlider* clone() {
        return new BKTuningKeyboardSlider(new TuningKeyboardState(),false,false);
    }
    void syncToValueTree() override {

    }

    void handleNoteOn(juce::MidiKeyboardState *source, int midiChannel, int midiNoteNumber, float velocity) override{}
    void handleNoteOff(juce::MidiKeyboardState *source, int midiChannel, int midiNoteNumber, float velocity) override{}
    // void bkMessageReceived (const juce::String& message) override {};
    // void bkComboBoxDidChange (juce::ComboBox*) override {};
    void textEditorReturnKeyPressed(juce::TextEditor& textEditor) override;
    void textEditorFocusLost(juce::TextEditor& textEditor) override;
    void textEditorEscapeKeyPressed (juce::TextEditor& textEditor) override;
    void textEditorTextChanged(juce::TextEditor& textEditor) override;
    // void bkTextFieldDidChange (juce::TextEditor& txt) override;
    void buttonClicked (juce::Button* b) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    std::unique_ptr<juce::TextEditor> keyboardValsTextField;
    TuningKeyboardState* keyboardState;
private:

    juce::String sliderName;
    juce::Label showName;
    bool isCircular;

    bool needsOctaveSlider;
#if JUCE_IOS
    Slider octaveSlider;
    void sliderValueChanged     (Slider* slider)                override;
    BKButtonAndMenuLAF laf;
#endif

    float ratio;

    juce::TextEditor keyboardValueTF;
    std::unique_ptr<KeyboardOffsetComponent> keyboard;

    juce::TextButton keyboardValsTextFieldOpen;

    int keyboardSize, minKey, maxKey;
    int lastKeyPressed;


    int displayResolution; // how many decimal points


    void setActiveValsFromString(juce::String s);
    void setActiveValsFromStringWithFundamentalOffset(juce::String s);




    bool focusLostByEscapeKey;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BKTuningKeyboardSlider)
};



