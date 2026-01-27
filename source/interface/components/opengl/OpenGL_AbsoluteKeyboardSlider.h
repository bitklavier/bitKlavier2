//
// Created by Davis Polito on 5/19/25.
//

#ifndef OPENGL_ABSOLUTEKEYBOARDSLIDER_H
#define OPENGL_ABSOLUTEKEYBOARDSLIDER_H
#include "../BKComponents/BKTuningKeyboardSlider.h"
#include "open_gl_image_component.h"

class OpenGLAbsoluteKeyboardSlider : public OpenGlAutoImageComponent<BKTuningKeyboardSlider> {
public:
    OpenGLAbsoluteKeyboardSlider(TuningState& keystate, bool helperButtons = true, bool showBorder_ = false, juce::String borderLabel = "")
        : OpenGlAutoImageComponent<BKTuningKeyboardSlider> (&keystate, false, false, false, keystate.stateChanges.defaultState)
    {
        image_component_ = std::make_shared<OpenGlImageComponent>();
        setLookAndFeel(DefaultLookAndFeel::instance());
        image_component_->setComponent(this);
        setComponentID(IDs::absoluteTuning.toString());
        useHelperButtons = helperButtons;

        isModulated_ = true;
        isModulation_ = false;

        showBorder = showBorder_;
        if (showBorder)
        {
            addAndMakeVisible(sliderBorder);
            sliderBorder.setText (borderLabel);
            sliderBorder.setTextLabelPosition (juce::Justification::centred);
        }
    }

   OpenGLAbsoluteKeyboardSlider() :OpenGLAbsoluteKeyboardSlider(mod_key_state)
    {
        isModulated_ = false;
        isModulation_ = true;

        setName("");

        sliderBorder.setColour(juce::GroupComponent::outlineColourId, findColour (Skin::kRotaryArc));
        sliderBorder.setColour(juce::GroupComponent::textColourId, findColour (Skin::kRotaryArc));
        sliderBorder.setText ("MODIFIED");
        sliderBorder.setTextLabelPosition (juce::Justification::centred);

        addAndMakeVisible(sliderBorder);
    }

    ~OpenGLAbsoluteKeyboardSlider(){}

    virtual void resized() override {
        if (isModulation_ || showBorder)
        {
            sliderBorder.setBounds(getLocalBounds());
        }

        OpenGlAutoImageComponent::resized();
        redoImage();
    }

    /*
     * this is for making the modulation UI view opaque
     */
    void paint(juce::Graphics& g) override {
        if (isModulation_)
        {
            g.fillAll(juce::Colours::black); // choose your opaque BG
            BKTuningKeyboardSlider::paint(g);
        }
    }

    /*
     * called whenever the user drags on a key in the absoluteKeyboard and sets its offset
     * we create a string representation of the new offsets and save it to the appropriate
     * valueTree: modulationState if the user is editing the modulation, or defaultState
     * is editing the actual absoluteKeyboard tuning slider
     */
    void mouseDrag(const juce::MouseEvent &e) override {
        OpenGlAutoImageComponent::mouseDrag(e);
        redoImage();

        juce::String s = "";
        int key = 0;
        for (auto& offset : keyboardState->absoluteTuningOffset)
        {
            if (offset != 0.f)  s += juce::String(key) + ":" + juce::String((offset)) + " ";
            ++key;
        }

        if (isModulation_)
        {
            modulationState.setProperty(IDs::absoluteTuning, s, nullptr);
            DBG("UI modulationState: " + modulationState.toXmlString());
        }
        else if (isModulated_)
        {
            defaultState.setProperty(IDs::absoluteTuning, s, nullptr);
            DBG("UI defaultState: " + defaultState.toXmlString());
        }
    }

    void mouseMove(const juce::MouseEvent &e) override {
        OpenGlAutoImageComponent::mouseMove(e);
        redoImage();
    }

    void buttonClicked(juce::Button *b) override {
        OpenGlAutoImageComponent::buttonClicked(b);
        redoImage();
    }

    void textEditorReturnKeyPressed(juce::TextEditor &textEditor) override {
        OpenGlAutoImageComponent::textEditorReturnKeyPressed(textEditor);
        redoImage();
        if (isModulation_) {
            modulationState.setProperty(IDs::absoluteTuning, keyboardValsTextField->getText(), nullptr);
        }
        else if (isModulated_) {
            defaultState.setProperty(IDs::absoluteTuning, keyboardValsTextField->getText(), nullptr);
        }
    }

    void textEditorFocusLost(juce::TextEditor &textEditor) override {
        OpenGlAutoImageComponent::textEditorFocusLost(textEditor);
        redoImage();
    }

    void textEditorEscapeKeyPressed(juce::TextEditor &textEditor) override {
        OpenGlAutoImageComponent::textEditorEscapeKeyPressed(textEditor);
        redoImage();
    }

    void textEditorTextChanged(juce::TextEditor &textEditor) override {
        OpenGlAutoImageComponent::textEditorTextChanged(textEditor);
        redoImage();
    }


    OpenGLAbsoluteKeyboardSlider *clone() override {
        return new OpenGLAbsoluteKeyboardSlider();
    }

    /**
     * syncToValueTree() is called in ModulationManager::modulationClicked and
     * is used to set the mod view of the parameter to the current values in the main view of the parameter
     * see the comparable one in OpenGL_TranspositionSlider.h
     */
    void syncToValueTree() override {
        updateValuesFromString (modulationState.getProperty(IDs::absoluteTuning).toString(), false);
    }

    inline static TuningState mod_key_state;
    bool useHelperButtons = true;
    bool showBorder = false;
    //juce::GroupComponent sliderBorder;
};

class OpenGLCircularKeyboardSlider : public OpenGlAutoImageComponent<BKTuningKeyboardSlider> {
public:

    OpenGLCircularKeyboardSlider(TuningState& keystate, bool showBorder_ = false, juce::String borderLabel = "")
        : OpenGlAutoImageComponent<BKTuningKeyboardSlider> (&keystate,false,false, true, keystate.stateChanges.defaultState) {
        image_component_ = std::make_shared<OpenGlImageComponent>();
        setLookAndFeel(DefaultLookAndFeel::instance());
        image_component_->setComponent(this);
        setComponentID(IDs::circularTuning.toString());

        isModulated_ = true;
        isModulation_ = false;

        showBorder = showBorder_;
        if (showBorder)
        {
            addAndMakeVisible(sliderBorder);
            sliderBorder.setText (borderLabel);
            sliderBorder.setTextLabelPosition (juce::Justification::centred);
        }
    }

    // clone constructor for modulators
    OpenGLCircularKeyboardSlider() : OpenGLCircularKeyboardSlider(mod_key_state)
    {
        isModulated_ = false;
        isModulation_ = true;

        setName("");

        sliderBorder.setColour(juce::GroupComponent::outlineColourId, findColour (Skin::kRotaryArc));
        sliderBorder.setColour(juce::GroupComponent::textColourId, findColour (Skin::kRotaryArc));
        sliderBorder.setText ("MODIFIED");
        sliderBorder.setTextLabelPosition (juce::Justification::centred);

        addAndMakeVisible(sliderBorder);
    }

    ~OpenGLCircularKeyboardSlider() {}

    virtual void resized() override {
        if (isModulation_ || showBorder)
        {
            sliderBorder.setBounds(getLocalBounds());
        }

        OpenGlAutoImageComponent::resized();
        redoImage();
    }

    /*
     * this is for making the modulation UI view opaque
     */
    void paint(juce::Graphics& g) override {
        if (isModulation_)
        {
            g.fillAll(juce::Colours::black); // choose your opaque BG
            BKTuningKeyboardSlider::paint(g);
        }
    }

    /**
     * see mouseDrag for absoluteKeyboardSlider above
     */
    void mouseDrag(const juce::MouseEvent &e) override {
        OpenGlAutoImageComponent::mouseDrag(e);
        redoImage();

        juce::String s = "";
        for (auto& offset : keyboardState->circularTuningOffset) {
            s += juce::String((offset)) + " ";
        }

        if (isModulation_)
        {
            modulationState.setProperty(IDs::circularTuning, s, nullptr);
            DBG("UI modulationState: " + modulationState.toXmlString());
            // issue: this should set the tuningType to custom, and then modify whatever is in custom
        }
        else if (isModulated_)
        {
            defaultState.setProperty(IDs::circularTuning, s, nullptr);
            DBG("UI defaultState: " + defaultState.toXmlString());
        }
    }

    void mouseMove(const juce::MouseEvent &e) override {
        OpenGlAutoImageComponent::mouseMove(e);
        redoImage();
    }

    void buttonClicked(juce::Button *b) override {
        OpenGlAutoImageComponent::buttonClicked(b);
        redoImage();
    }

    void textEditorReturnKeyPressed(juce::TextEditor &textEditor) override {
        OpenGlAutoImageComponent::textEditorReturnKeyPressed(textEditor);
        redoImage();
        if (isModulation_) {
            modulationState.setProperty(IDs::circularTuning, keyboardValsTextField->getText(), nullptr);
        }
        else if (isModulated_) {
            defaultState.setProperty(IDs::circularTuning, keyboardValsTextField->getText(), nullptr);
        }
    }

    void textEditorFocusLost(juce::TextEditor &textEditor) override {
        OpenGlAutoImageComponent::textEditorFocusLost(textEditor);
        redoImage();
    }

    void textEditorEscapeKeyPressed(juce::TextEditor &textEditor) override {
        OpenGlAutoImageComponent::textEditorEscapeKeyPressed(textEditor);
        redoImage();
    }

    void textEditorTextChanged(juce::TextEditor &textEditor) override {
        OpenGlAutoImageComponent::textEditorTextChanged(textEditor);
        redoImage();
    }

    OpenGLCircularKeyboardSlider *clone() override {
        return new OpenGLCircularKeyboardSlider();
    }

    /**
     * syncToValueTree() is called in ModulationManager::modulationClicked and
     * is used to set the mod view of the parameter to the current values in the main view of the parameter
     * see the comparable one in OpenGL_TranspositionSlider.h
     */
    void syncToValueTree() override
    {
        updateValuesFromString (modulationState.getProperty(IDs::circularTuning).toString(), true);
    }

    inline static TuningState mod_key_state;
    bool showBorder = false;
    //juce::GroupComponent sliderBorder;
};
#endif //OPENGL_ABSOLUTEKEYBOARDSLIDER_H
