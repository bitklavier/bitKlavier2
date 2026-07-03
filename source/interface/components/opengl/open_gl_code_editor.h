// Copyright (C) 2022-2025 Dan Trueman
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "open_gl_image_component.h"
#include <juce_gui_extra/juce_gui_extra.h>

// OpenGL-rendered wrapper around juce::CodeEditorComponent.
//
// Follows the same pattern as OpenGlTextEditor (open_gl_image_component.h:114):
//  - Inherits OpenGlAutoImageComponent<CodeEditorComponent> so the component
//    renders into an OpenGlImageComponent texture.
//  - Inherits juce::Timer at 30 Hz to trigger redraws every frame, capturing
//    caret blink and selection animation.
//
// Usage:
//   auto editor = std::make_unique<OpenGlCodeEditor>(doc, tokeniser);
//   addAndMakeVisible(editor.get());
//   addOpenGlComponent(editor->getImageComponent());
//   // ... configure font, colours, line numbers AFTER addAndMakeVisible
//   editor->stopTimer();  // call from stopAllTimers() in the owning SynthSection
//
class OpenGlCodeEditor : public OpenGlAutoImageComponent<juce::CodeEditorComponent>,
                         public juce::Timer
{
public:
    OpenGlCodeEditor (juce::CodeDocument& doc, juce::CodeTokeniser* tok)
        : OpenGlAutoImageComponent<juce::CodeEditorComponent> (doc, tok)
    {
        image_component_ = std::make_shared<OpenGlImageComponent>();
        image_component_->setComponent (this);
        // paintEntireComponent captures child ScrollBars into the GL texture.
        image_component_->paintEntireComponent (true);
        startTimer (33); // ~30 Hz — drives caret blink + edits
    }

    ~OpenGlCodeEditor() override { stopTimer(); }

private:
    void timerCallback() override { redoImage(); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OpenGlCodeEditor)
};
