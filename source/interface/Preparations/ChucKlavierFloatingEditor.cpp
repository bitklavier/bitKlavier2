// Copyright (C) 2022-2025 Dan Trueman
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ChucKlavierFloatingEditor.h"
#include "ChucKlavierProcessor.h"
#include "default_look_and_feel.h"

// ─── static members ──────────────────────────────────────────────────────────

juce::Array<ChucKlavierFloatingEditor*, juce::CriticalSection>
    ChucKlavierFloatingEditor::s_liveWindows_;

// ─── CompileTimer ────────────────────────────────────────────────────────────

void ChucKlavierFloatingEditorContent::CompileTimer::start (const juce::String& script)
{
    script_    = script;
    tickCount_ = 0;
    startTimer (5);
}

static int parseErrorLine (const juce::String& msg)
{
    auto idx = msg.indexOf ("<compiled.code>:");
    if (idx >= 0) { int n = msg.substring (idx + 16).getIntValue(); if (n > 0) return n; }
    idx = msg.indexOf ("[chuck:");
    if (idx >= 0) { int n = msg.substring (idx + 7).getIntValue(); if (n > 0) return n; }
    const int colon = msg.indexOfChar (':');
    if (colon >= 0) { int n = msg.substring (colon + 1).getIntValue(); if (n > 0) return n; }
    return -1;
}

void ChucKlavierFloatingEditorContent::CompileTimer::timerCallback()
{
    auto& proc = owner_.proc_;

    if (! proc.vmParked_.load (std::memory_order_acquire))
    {
        if (++tickCount_ > 40)  // ~200ms timeout
        {
            stopTimer();
            proc.vmSuspended_.store (false, std::memory_order_release);
            owner_.setStatus ("Timeout waiting for VM to park", juce::Colours::orange);
            owner_.compileButton_->setEnabled (true);
        }
        return;
    }

    stopTimer();
    bool ok = proc.doHotSwap (script_.toStdString());
    if (ok) proc.reconcileSlots (script_);
    proc.vmSuspended_.store (false, std::memory_order_release);

    owner_.compileButton_->setEnabled (true);

    // Update status and error highlight in this floating window.
    if (auto* floater = proc.floatingEditor_)
    {
        if (ok)
        {
            floater->clearErrorHighlight();
            owner_.setStatus (proc.lastCompileMessage.isEmpty() ? "OK" : proc.lastCompileMessage,
                              juce::Colours::lightgreen);
        }
        else
        {
            floater->applyErrorHighlight (parseErrorLine (proc.lastCompileMessage));
            owner_.setStatus (proc.lastCompileMessage, juce::Colours::orange);
        }
    }
}

// ─── content component ───────────────────────────────────────────────────────

ChucKlavierFloatingEditorContent::ChucKlavierFloatingEditorContent (ChucKlavierProcessor& proc)
    : proc_ (proc), doc_ (proc.getScriptDoc())
{
    // Apply the plugin's dark look-and-feel so buttons and text fields match
    // the in-popup editor style.
    setLookAndFeel (DefaultLookAndFeel::instance());

    tokeniser_    = std::make_unique<ChucKCodeTokeniser>();
    compileTimer_ = std::make_unique<CompileTimer> (*this);

    editor_ = std::make_unique<OpenGlCodeEditor> (doc_, tokeniser_.get());
    addAndMakeVisible (editor_.get());

    editor_->setFont (juce::Font (juce::Font::getDefaultMonospacedFontName(), 13.0f, juce::Font::plain));
    editor_->setDefaultFontSize (13.0f);
    editor_->setColourScheme (tokeniser_->getDefaultColourScheme());
    editor_->setColour (juce::CodeEditorComponent::backgroundColourId, juce::Colour (0xdd000000));
    editor_->setColour (juce::CodeEditorComponent::highlightColourId,  juce::Colours::darkgreen.withAlpha (0.5f));
    editor_->setColour (juce::CodeEditorComponent::defaultTextColourId, juce::Colour (0xffe0e0e0));
    editor_->setLineNumbersShown (true);

    compileButton_ = std::make_unique<juce::TextButton> ("Send to VM");
    compileButton_->onClick = [this] { compile(); };
    addAndMakeVisible (compileButton_.get());

    statusLabel_ = std::make_unique<juce::Label> ("status", "VM ready");
    statusLabel_->setColour (juce::Label::textColourId, juce::Colours::lightgrey);
    statusLabel_->setFont (juce::Font (12.0f));
    statusLabel_->setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (statusLabel_.get());

    // Find bar — OpenGlTextEditor matches the popup's find-bar style.
    // In this software-rendered window the GL texture is unused; the component
    // renders via juce::TextEditor::paint() like any normal text field.
    findField_ = std::make_unique<OpenGlTextEditor> ("chuckFloatFind");
    findField_->setMonospace();
    findField_->setFontSize (12.0f);
    findField_->setTextToShowWhenEmpty (juce::String::fromUTF8 ("Find\xe2\x80\xa6"), juce::Colours::grey);
    findField_->onReturnKey = [this] { runFind (! juce::ModifierKeys::currentModifiers.isShiftDown()); };
    findField_->onEscapeKey = [this] { hideFindBar(); };
    addAndMakeVisible (findField_.get());
    findField_->setVisible (false);

    editor_->onFindShortcut     = [this] { showFindBar(); };
    editor_->onFindNextShortcut = [this] { if (lastFindNeedle_.isNotEmpty()) runFind (true);  else showFindBar(); };
    editor_->onFindPrevShortcut = [this] { if (lastFindNeedle_.isNotEmpty()) runFind (false); else showFindBar(); };
}

ChucKlavierFloatingEditorContent::~ChucKlavierFloatingEditorContent()
{
    compileTimer_->stopTimer();
    setLookAndFeel (nullptr);
}

void ChucKlavierFloatingEditorContent::resized()
{
    const int btnH    = 28;
    const int btnW    = 90;
    const int gap     = 6;
    const int leftPad = 8;   // clears the window's rounded corner
    const int findH   = 22;
    auto bounds       = getLocalBounds();

    auto bottomRow    = bounds.removeFromBottom (btnH);
    bottomRow.removeFromLeft (leftPad);
    compileButton_->setBounds (bottomRow.removeFromLeft (btnW));
    bottomRow.removeFromLeft (gap);
    statusLabel_->setBounds (bottomRow);

    if (findBarVisible_ && findField_ != nullptr)
    {
        auto findRow = bounds.removeFromTop (findH);
        findRow.removeFromLeft (leftPad);
        findField_->setBounds (findRow);
    }

    editor_->setBounds (bounds);
}

void ChucKlavierFloatingEditorContent::setStatus (const juce::String& text, juce::Colour colour)
{
    statusLabel_->setText (text, juce::dontSendNotification);
    statusLabel_->setColour (juce::Label::textColourId, colour);
}

void ChucKlavierFloatingEditorContent::compile()
{
    setStatus (juce::String::fromUTF8 ("Compiling\xe2\x80\xa6"), juce::Colours::yellow);
    compileButton_->setEnabled (false);
    proc_.vmParked_.store (false, std::memory_order_relaxed);
    proc_.vmSuspended_.store (true, std::memory_order_release);
    compileTimer_->start (doc_.getAllContent());
}

// ─── find bar helpers ────────────────────────────────────────────────────────

void ChucKlavierFloatingEditorContent::showFindBar()
{
    if (editor_ != nullptr)
    {
        auto sel = editor_->getHighlightedRegion();
        if (! sel.isEmpty())
        {
            auto text = editor_->getTextInRange (sel);
            if (text.isNotEmpty())
            {
                findField_->setText (text);
                findField_->selectAll();
            }
        }
    }
    findField_->setVisible (true);
    findBarVisible_ = true;
    resized();
    findField_->grabKeyboardFocus();
}

void ChucKlavierFloatingEditorContent::hideFindBar()
{
    findField_->setVisible (false);
    findBarVisible_ = false;
    resized();
    if (editor_ != nullptr)
        editor_->grabKeyboardFocus();
}

void ChucKlavierFloatingEditorContent::runFind (bool forward)
{
    if (findField_ == nullptr || editor_ == nullptr) return;
    auto text = findField_->getText();
    if (text.isEmpty()) return;
    lastFindNeedle_ = text;
    bool ok = forward ? editor_->findNext (text) : editor_->findPrev (text);
    if (! ok) flashFindNoMatch();
}

void ChucKlavierFloatingEditorContent::flashFindNoMatch()
{
    findField_->setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xffcc4400));
    findField_->repaint();
    juce::Timer::callAfterDelay (200, [this] {
        if (findField_ != nullptr)
        {
            findField_->removeColour (juce::TextEditor::backgroundColourId);
            findField_->repaint();
        }
    });
}

// ─── floating window ─────────────────────────────────────────────────────────

ChucKlavierFloatingEditor::ChucKlavierFloatingEditor (ChucKlavierProcessor& proc)
    : juce::DocumentWindow (juce::String (proc.getDisplayName()) + " \xe2\x80\x94 Script Editor",
                             juce::Colour (0xff1a1a1a),
                             juce::DocumentWindow::allButtons)
    , proc_ (proc)
{
    content_ = std::make_unique<ChucKlavierFloatingEditorContent> (proc);
    setContentNonOwned (content_.get(), false);
    setResizable (true, false);
    setResizeLimits (400, 300, 4096, 4096);
    centreWithSize (700, 500);
    setUsingNativeTitleBar (true);
    setVisible (true);

    s_liveWindows_.add (this);
}

ChucKlavierFloatingEditor::~ChucKlavierFloatingEditor()
{
    proc_.floatingEditor_ = nullptr;   // clear processor's back-reference
    s_liveWindows_.removeFirstMatchingValue (this);
    clearContentComponent();   // detaches content_ from window hierarchy first
    // content_ unique_ptr destructs here: stops compile timer, destructs editor
}

void ChucKlavierFloatingEditor::applyErrorHighlight (int line1Based)
{
    if (content_ == nullptr || line1Based < 1) return;
    auto* ed = content_->getEditor();
    if (ed == nullptr) return;

    auto& doc = proc_.getScriptDoc();
    if (line1Based > doc.getNumLines()) return;

    juce::CodeDocument::Position lineStart (doc, line1Based - 1, 0);
    int col = doc.getLine (line1Based - 1).trimEnd().length();
    if (col == 0) col = 1;
    juce::CodeDocument::Position lineEnd (doc, line1Based - 1, col);

    ed->setColour (juce::CodeEditorComponent::highlightColourId, juce::Colours::red.withAlpha (0.4f));
    ed->setHighlightedRegion (juce::Range<int> (lineStart.getPosition(), lineEnd.getPosition()));
    ed->scrollToLine (line1Based - 1);
}

void ChucKlavierFloatingEditor::clearErrorHighlight()
{
    if (content_ == nullptr) return;
    if (auto* ed = content_->getEditor())
        ed->setColour (juce::CodeEditorComponent::highlightColourId,
                       juce::Colours::darkgreen.withAlpha (0.5f));
}

void ChucKlavierFloatingEditor::closeAll()
{
    // Snapshot first — dtor modifies s_liveWindows_.
    auto snapshot = s_liveWindows_;
    for (auto* w : snapshot)
        delete w;
}
