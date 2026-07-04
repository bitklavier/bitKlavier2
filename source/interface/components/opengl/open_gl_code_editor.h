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
// Editing behavior (script editor only; all paths gate on isReadOnly()):
//  - Auto-indent on Return: copies current line's leading whitespace and adds
//    one extra indent when the line ends with { ( [
//  - Auto-dedent on }: when } is the first non-whitespace on the line, snaps
//    the indent to the matching {'s indent via a tokeniser-driven balanced scan.
//  - Brace-match highlight: outlines the adjacent { } ( ) [ ] and its partner.
//  - Smart backspace: inside pure leading whitespace, deletes to the previous
//    tab stop rather than one character.
//
// Console panel usage (read-only, nullptr tokeniser): call setReadOnly(true).
// All new behavior short-circuits on isReadOnly() or tokeniser_ == nullptr.
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
        , tokeniser_ (tok)
        , braceA_ (doc, 0)
        , braceB_ (doc, 0)
    {
        image_component_ = std::make_shared<OpenGlImageComponent>();
        image_component_->setComponent (this);
        // paintEntireComponent captures child ScrollBars into the GL texture.
        image_component_->paintEntireComponent (true);
        startTimer (33); // ~30 Hz — drives caret blink + edits
    }

    ~OpenGlCodeEditor() override { stopTimer(); }

    // Overrides — see open_gl_code_editor.cpp for implementations.
    void handleReturnKey() override;
    void insertTextAtCaret (const juce::String& textToInsert) override;
    bool keyPressed (const juce::KeyPress& key) override;
    void paint (juce::Graphics& g) override;
    void caretPositionMoved() override;

private:
    void timerCallback() override { redoImage(); }

    // Brace-match helpers (impl in .cpp)
    void updateBraceMatch();
    int  findMatchingBrace (int anchorIdx, juce::juce_wchar anchorChar) const;
    void drawBraceMatchOverlay (juce::Graphics& g);

    // Smart-backspace helper
    bool trySmartBackspace();

    // Stashed from ctor; nullptr for the read-only console panel.
    juce::CodeTokeniser* tokeniser_ = nullptr;

    // Positions of the matched brace pair; recomputed on every caret move.
    // Not position-maintained because updateBraceMatch() fires on every edit.
    juce::CodeDocument::Position braceA_, braceB_;
    bool braceMatchValid_ = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OpenGlCodeEditor)
};
