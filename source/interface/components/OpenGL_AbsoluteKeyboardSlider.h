//
// Created by Davis Polito on 5/19/25.
//

#ifndef OPENGL_ABSOLUTEKEYBOARDSLIDER_H
#define OPENGL_ABSOLUTEKEYBOARDSLIDER_H
#include "BKAbsoluteKeyboardSlider.h"
#include "open_gl_image_component.h"
class OpenGLAbsoluteKeyboardSlider : public OpenGlAutoImageComponent<BKAbsoluteKeyboardSlider> {
public:
    OpenGLAbsoluteKeyboardSlider(TuningParams & params)
        : OpenGlAutoImageComponent<BKAbsoluteKeyboardSlider> (&params.keyboardState,false,false) {
        image_component_ = std::make_shared<OpenGlImageComponent>();
        setLookAndFeel(DefaultLookAndFeel::instance());
        image_component_->setComponent(this);
    }

    ~OpenGLAbsoluteKeyboardSlider()
    {}
    virtual void resized() override {
        OpenGlAutoImageComponent::resized();
        // if (isShowing())
        redoImage();
    }
    void mouseDrag(const juce::MouseEvent &e) override {
        OpenGlAutoImageComponent::mouseDrag(e);
        redoImage();
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

};
#endif //OPENGL_ABSOLUTEKEYBOARDSLIDER_H
