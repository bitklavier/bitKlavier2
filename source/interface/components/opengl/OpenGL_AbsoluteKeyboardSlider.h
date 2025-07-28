//
// Created by Davis Polito on 5/19/25.
//

#ifndef OPENGL_ABSOLUTEKEYBOARDSLIDER_H
#define OPENGL_ABSOLUTEKEYBOARDSLIDER_H
#include "../BKComponents/BKTuningKeyboardSlider.h"
#include "open_gl_image_component.h"
class OpenGLAbsoluteKeyboardSlider : public OpenGlAutoImageComponent<BKTuningKeyboardSlider> {
public:
    OpenGLAbsoluteKeyboardSlider(TuningState& keystate)
        : OpenGlAutoImageComponent<BKTuningKeyboardSlider> (&keystate,false,false, false) {
        image_component_ = std::make_shared<OpenGlImageComponent>();
        setLookAndFeel(DefaultLookAndFeel::instance());
        image_component_->setComponent(this);
        setComponentID(IDs::absoluteTuning.toString());
    }
    /**
     * todo: update all this isModulation/isModulated_ stuff to match what is in OpenGL_TranspositionSlider and elsewhere
     */

    OpenGLAbsoluteKeyboardSlider() :OpenGLAbsoluteKeyboardSlider(mod_key_state){

        isModulation_ = true;
    }

    ~OpenGLAbsoluteKeyboardSlider(){}

    virtual void resized() override {
        OpenGlAutoImageComponent::resized();
        // if (isShowing())
        redoImage();
    }

    void mouseDrag(const juce::MouseEvent &e) override {
        OpenGlAutoImageComponent::mouseDrag(e);
        redoImage();
        if (isModulation_) {
            //        keyboardValsTextField->setText(offsetArrayToString3(keyboard->getValues(), midRange), dontSendNotification);
            juce::String s = "";
            int key = 0;
            for (auto offset : keyboardState->absoluteTuningOffset)
            {

                if (offset != 0.f)  s += juce::String(key) + ":" + juce::String((offset)) + " ";

                ++key;
            }

            modulationState.setProperty(IDs::absoluteTuning, s, nullptr);
        }
        else if (isModulated_)
        {

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
//        modulationState = juce::ValueTree(IDs::absoluteTuning);
    }

    TuningState mod_key_state;
};

class OpenGLCircularKeyboardSlider : public OpenGlAutoImageComponent<BKTuningKeyboardSlider> {
public:

    OpenGLCircularKeyboardSlider(TuningState& keystate)
        : OpenGlAutoImageComponent<BKTuningKeyboardSlider> (&keystate,false,false, true) {
        image_component_ = std::make_shared<OpenGlImageComponent>();
        setLookAndFeel(DefaultLookAndFeel::instance());
        image_component_->setComponent(this);
        setComponentID(IDs::circularTuning.toString());
    }

    OpenGLCircularKeyboardSlider() : OpenGLCircularKeyboardSlider(mod_key_state)
    {
        isModulation_ = true;
    }

    ~OpenGLCircularKeyboardSlider() {}

    virtual void resized() override {
        OpenGlAutoImageComponent::resized();
        // if (isShowing())
        redoImage();
    }

    void mouseDrag(const juce::MouseEvent &e) override {
        OpenGlAutoImageComponent::mouseDrag(e);
        redoImage();
        if (isModulation_) {
            juce::String s = "";

            for (auto offset : keyboardState->circularTuningOffset) {
                s += juce::String((offset)) + " ";
            }
            modulationState.setProperty(IDs::circularTuning, s, nullptr);
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

    void syncToValueTree() override {}

    TuningState mod_key_state;

};
#endif //OPENGL_ABSOLUTEKEYBOARDSLIDER_H
