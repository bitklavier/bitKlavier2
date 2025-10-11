//
// Created by Dan Trueman on 10/9/25.
//

#ifndef BITKLAVIER0_OPENGL_KEYMAPKEYBOARD_H
#define BITKLAVIER0_OPENGL_KEYMAPKEYBOARD_H
#pragma once

#include "../components/BKComponents/BKKeymapKeyboardComponent.h"

class OpenGLKeymapKeyboardComponent: public OpenGlAutoImageComponent<BKKeymapKeyboardComponent> {
public:
    OpenGLKeymapKeyboardComponent(KeymapKeyboardState & params, bool helperButtons = true, bool isMono = false) :
        OpenGlAutoImageComponent (&params, helperButtons, isMono) {
        image_component_ = std::make_shared<OpenGlImageComponent>();
        setLookAndFeel(DefaultLookAndFeel::instance());
        image_component_->setComponent(this);
    }

    ~OpenGLKeymapKeyboardComponent(){}

    virtual void resized() override {
        OpenGlAutoImageComponent::resized();
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

#endif //BITKLAVIER0_OPENGL_KEYMAPKEYBOARD_H
