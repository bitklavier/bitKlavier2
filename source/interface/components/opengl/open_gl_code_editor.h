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
        s_liveEditors_.add (this);
    }

    ~OpenGlCodeEditor() override
    {
        stopTimer();
        s_liveEditors_.removeFirstMatchingValue (this);
    }

    // Overrides — see open_gl_code_editor.cpp for implementations.
    void handleReturnKey() override;
    void insertTextAtCaret (const juce::String& textToInsert) override;
    bool keyPressed (const juce::KeyPress& key) override;
    void paint (juce::Graphics& g) override;
    void caretPositionMoved() override;

    // Font-size zoom — Cmd+= / Cmd+- / Cmd+0. Affects all live editors (shared global).
    void setFontSize (float newSize);
    float getFontSize() const noexcept { return getFont().getHeight(); }
    // Mark this editor's current font size as the reset target for Cmd+0.
    void setDefaultFontSize (float size) noexcept { defaultFontSize_ = size; }

    // Called by setFontSize to propagate the change to every live OpenGlCodeEditor.
    static void broadcastFontSize (float newSize);
    static float getGlobalFontSize() noexcept { return s_globalFontSize_.load(); }

    // Find/search — wraps around silently. Returns true if a match was found.
    bool findNext (const juce::String& needle, bool caseSensitive = false);
    bool findPrev (const juce::String& needle, bool caseSensitive = false);

    // Set these to wire up keyboard shortcuts. Called from keyPressed when:
    //   onFindShortcut:     Cmd+F  (open find bar)
    //   onFindNextShortcut: Cmd+G  (next match)
    //   onFindPrevShortcut: Shift+Cmd+G  (previous match)
    // All three work in both editable and read-only modes.
    // Leaving a callback nullptr is safe — the key is consumed but nothing happens.
    std::function<void()> onFindShortcut;
    std::function<void()> onFindNextShortcut;
    std::function<void()> onFindPrevShortcut;

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

    // Font size at construction — Cmd+0 resets to this value.
    float defaultFontSize_ = 13.0f;

    // Positions of the matched brace pair; recomputed on every caret move.
    // Not position-maintained because updateBraceMatch() fires on every edit.
    juce::CodeDocument::Position braceA_, braceB_;
    bool braceMatchValid_ = false;

    // Global font size shared across all live editors. 0 = "use default".
    static std::atomic<float> s_globalFontSize_;

    // Registry of all live editors so broadcastFontSize can reach them.
    static juce::Array<OpenGlCodeEditor*, juce::CriticalSection> s_liveEditors_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OpenGlCodeEditor)
};
