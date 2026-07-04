// Copyright (C) 2022-2025 Dan Trueman
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "ChucKCodeTokeniser.h"
#include "open_gl_code_editor.h"
#include <juce_gui_extra/juce_gui_extra.h>

class ChucKlavierProcessor;

// ─── content component ───────────────────────────────────────────────────────

// The scrollable editing area inside ChucKlavierFloatingEditor.
// Uses normal JUCE software rendering (no separate OpenGLContext needed —
// the OpenGlCodeEditor renders via its paint(g) + caret-blink paths without GL).
class ChucKlavierFloatingEditorContent : public juce::Component
{
public:
    explicit ChucKlavierFloatingEditorContent (ChucKlavierProcessor& proc);
    ~ChucKlavierFloatingEditorContent() override;

    void resized() override;

    // Expose the editor for error-highlight routing.
    OpenGlCodeEditor* getEditor() noexcept { return editor_.get(); }

    void compile();

    void showFindBar();
    void hideFindBar();
    void runFind (bool forward);
    void flashFindNoMatch();

private:
    ChucKlavierProcessor&                proc_;
    juce::CodeDocument&                  doc_;
    std::unique_ptr<ChucKCodeTokeniser>  tokeniser_;
    std::unique_ptr<OpenGlCodeEditor>    editor_;
    std::unique_ptr<juce::TextButton>    compileButton_;
    std::unique_ptr<juce::Label>         statusLabel_;
    std::unique_ptr<OpenGlTextEditor>    findField_;
    bool                                 findBarVisible_ = false;
    juce::String                         lastFindNeedle_;

    void setStatus (const juce::String& text, juce::Colour colour);

    // Polls vmParked_ at 5ms until the audio thread parks, then calls doHotSwap.
    struct CompileTimer : public juce::Timer
    {
        explicit CompileTimer (ChucKlavierFloatingEditorContent& owner) : owner_ (owner) {}
        void start (const juce::String& script);
        void timerCallback() override;
        ChucKlavierFloatingEditorContent& owner_;
        juce::String                      script_;
        int                               tickCount_ = 0;
    };
    std::unique_ptr<CompileTimer>        compileTimer_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChucKlavierFloatingEditorContent)
};

// ─── floating window ─────────────────────────────────────────────────────────

// Persistent, resizable script editor window for one ChucKlavier preparation.
// Allocation: heap-managed (new / delete).  Owned by ChucKlavierProcessor via
// a raw pointer; closed either when the prep is deleted (dtor) or when the
// plugin editor closes (ChucKlavierFloatingEditor::closeAll()).
class ChucKlavierFloatingEditor : public juce::DocumentWindow
{
public:
    explicit ChucKlavierFloatingEditor (ChucKlavierProcessor& proc);
    ~ChucKlavierFloatingEditor() override;

    // Route a compile-result error highlight to this editor.
    void applyErrorHighlight (int line1Based);
    void clearErrorHighlight();

    // Delete every live floating editor (call from message thread before GL teardown).
    static void closeAll();

    // Hiding instead of deleting keeps the window alive for quick re-open.
    void closeButtonPressed() override { setVisible (false); }

private:
    ChucKlavierProcessor&                             proc_;
    std::unique_ptr<ChucKlavierFloatingEditorContent> content_;

    static juce::Array<ChucKlavierFloatingEditor*, juce::CriticalSection> s_liveWindows_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChucKlavierFloatingEditor)
};
