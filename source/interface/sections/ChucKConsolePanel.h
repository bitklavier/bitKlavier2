// Copyright (C) 2022-2025 Dan Trueman
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <deque>
#include "open_gl_background.h"
#include "open_gl_code_editor.h"
#include "paths.h"
#include "synth_button.h"
#include "synth_section.h"

// Floating, draggable, closable console that displays ChucK <<<>>> output.
// One global instance lives on FullInterface; all ChucKlavier preps share it.
class ChucKConsolePanel : public SynthSection
{
public:
    static constexpr int kHeaderH    = 28;
    static constexpr int kExitW      = 20;
    static constexpr int kClearW     = 48;
    static constexpr int kPadding    = 4;

    ChucKConsolePanel (OpenGlWrapper& openGl)
        : SynthSection ("chuckConsole"),
          openGl_ (openGl)
    {
        setLookAndFeel (DefaultLookAndFeel::instance());

        background_ = std::make_shared<OpenGlBackground>();
        addBackgroundComponent (background_.get());
        background_->setComponent (this);

        title_ = std::make_shared<PlainTextComponent> ("chuckConsoleTitle", "ChucK Console");
        title_->setFontType (PlainTextComponent::kRegular);
        title_->setTextSize (11.0f);
        title_->setJustification (juce::Justification::centredLeft);
        title_->setColor (juce::Colours::lightgrey);
        addOpenGlComponent (title_);

        exit_ = std::make_shared<OpenGlShapeButton> ("consoleExit");
        addAndMakeVisible (exit_.get());
        addOpenGlComponent (exit_->getGlComponent());
        exit_->setShape (Paths::exitX());
        exit_->onClick = [this] { setVisible (false); };

        clearButton_ = std::make_unique<SynthButton> ("consoleClear");
        clearButton_->setText ("Clear");
        addSynthButton (clearButton_.get(), true);
        clearButton_->onClick = [this] { clear(); };

        // Code editor for console output — must be added before any resized() call.
        logEditor_ = std::make_unique<OpenGlCodeEditor> (logDoc_, nullptr);
        addAndMakeVisible (logEditor_.get());
        addOpenGlComponent (logEditor_->getImageComponent());
        logEditor_->setReadOnly (true);
        logEditor_->setLineNumbersShown (false);
        logEditor_->setFont (juce::Font (juce::Font::getDefaultMonospacedFontName(), 11.0f, juce::Font::plain));
        logEditor_->setDefaultFontSize (11.0f);
        logEditor_->setColour (juce::CodeEditorComponent::backgroundColourId, juce::Colour (0xff111111));
        logEditor_->setColour (juce::CodeEditorComponent::defaultTextColourId, juce::Colour (0xffdddddd));

        constrainer_.setMinimumOnscreenAmounts (kHeaderH * 2, 40, 40, 40);
    }

    ~ChucKConsolePanel() override = default;

    // Append one line to the console and scroll to show it.
    void append (const juce::String& line)
    {
        juce::CodeDocument::Position end (logDoc_, std::numeric_limits<int>::max(),
                                          std::numeric_limits<int>::max());
        const bool empty = logDoc_.getNumCharacters() == 0;
        logDoc_.insertText (end, empty ? line : ("\n" + line));
        // Move caret to document end — JUCE calls scrollToKeepCaretOnScreen()
        // which places the new line near the bottom of the visible area.
        juce::CodeDocument::Position newEnd (logDoc_, std::numeric_limits<int>::max(),
                                             std::numeric_limits<int>::max());
        logEditor_->moveCaretTo (newEnd, false);
    }

    // Dump buffered lines on show (does NOT clear before dumping).
    void dumpBacklog (const std::deque<juce::String>& lines)
    {
        for (const auto& l : lines)
            append (l);
    }

    void clear()
    {
        logDoc_.replaceAllContent ("");
    }

    bool hasBeenPositioned_ = false;

    // -------------------------------------------------------------------------
    void paintBackground (juce::Graphics& g) override
    {
        g.fillAll (juce::Colours::black);
        // Header strip
        g.setColour (juce::Colour (0xff2a2a2a));
        g.fillRect (0, 0, getWidth(), kHeaderH);
        paintChildrenBackgrounds (g);
    }

    void resized() override
    {
        auto area = getLocalBounds();

        // Header row
        auto header = area.removeFromTop (kHeaderH);
        const int exitX = header.getRight() - kPadding - kExitW;
        exit_->setBounds (exitX, header.getY() + (kHeaderH - kExitW) / 2, kExitW, kExitW);

        const int clearX = exitX - kPadding - kClearW;
        clearButton_->setBounds (clearX, header.getY() + kPadding,
                                 kClearW, kHeaderH - 2 * kPadding);

        title_->setBounds (header.getX() + kPadding, header.getY(),
                           clearX - kPadding * 2, kHeaderH);

        // Editor fills the rest
        area.reduce (2, 2);
        if (logEditor_ != nullptr)
            logEditor_->setBounds (area);

        SynthSection::resized();
    }

    // Drag by header strip only
    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.getPosition().getY() < kHeaderH)
        {
            dragger_.startDraggingComponent (this, e);
            hasBeenPositioned_ = true;
        }
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (e.getDistanceFromDragStartY() != 0 || e.getDistanceFromDragStartX() != 0)
            dragger_.dragComponent (this, e, &constrainer_);
    }

    bool keyPressed (const juce::KeyPress& key) override
    {
        if (key == juce::KeyPress::escapeKey)
        {
            setVisible (false);
            return true;
        }
        return false;
    }

private:
    OpenGlWrapper&                          openGl_;
    juce::CodeDocument                      logDoc_;
    std::unique_ptr<OpenGlCodeEditor>       logEditor_;
    std::shared_ptr<OpenGlBackground>       background_;
    std::shared_ptr<PlainTextComponent>     title_;
    std::shared_ptr<OpenGlShapeButton>      exit_;
    std::unique_ptr<SynthButton>            clearButton_;
    juce::ComponentDragger                  dragger_;
    juce::ComponentBoundsConstrainer        constrainer_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChucKConsolePanel)
};
