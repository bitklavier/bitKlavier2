#ifndef BITKLAVIER2_COMMENTPREPARATION_H
#define BITKLAVIER2_COMMENTPREPARATION_H

#pragma once
#include "CommentProcessor.h"
#include "FullInterface.h"
#include "PreparationSection.h"
#include "open_gl_image_component.h"
#include <juce_gui_basics/juce_gui_basics.h>

class CommentTextEditor : public OpenGlTextEditor
{
public:
    CommentTextEditor (juce::String name) : OpenGlTextEditor (name) {}

    // Formatting state — read/written by CommentPreparation
    bool bold_ = false;
    bool italic_ = false;
    int alignment_ = 0;   // 0=left, 1=center, 2=right
    juce::Colour textColor_ = juce::Colours::white;
    int background_ = 0;  // 0=default (grey), 1=black, 2=transparent

    std::function<void()> onFormatChanged;

    void addPopupMenuItems (juce::PopupMenu& menu, const juce::MouseEvent* e) override
    {
        OpenGlTextEditor::addPopupMenuItems (menu, e);
        menu.addSeparator();

        juce::PopupMenu alignMenu;
        alignMenu.addItem (0x5001, "Left",   true, alignment_ == 0);
        alignMenu.addItem (0x5002, "Center", true, alignment_ == 1);
        alignMenu.addItem (0x5003, "Right",  true, alignment_ == 2);
        menu.addSubMenu ("Alignment", alignMenu);

        menu.addItem (0x5010, "Bold",   true, bold_);
        menu.addItem (0x5011, "Italic", true, italic_);

        juce::PopupMenu colorMenu;
        colorMenu.addItem (0x5020, "White",       true, textColor_ == juce::Colours::white);
        colorMenu.addItem (0x5021, "Yellow",      true, textColor_ == juce::Colours::yellow);
        colorMenu.addItem (0x5022, "Cyan",        true, textColor_ == juce::Colours::cyan);
        colorMenu.addItem (0x5023, "Light Green", true, textColor_ == juce::Colours::lightgreen);
        colorMenu.addItem (0x5024, "Coral",       true, textColor_ == juce::Colours::coral);
        colorMenu.addItem (0x5025, "Orange",      true, textColor_ == juce::Colours::orange);
        menu.addSubMenu ("Color", colorMenu);

        juce::PopupMenu bgMenu;
        bgMenu.addItem (0x5030, "Default (Grey)", true, background_ == 0);
        bgMenu.addItem (0x5031, "Black",          true, background_ == 1);
        bgMenu.addItem (0x5032, "Transparent",    true, background_ == 2);
        menu.addSubMenu ("Background", bgMenu);
    }

    void performPopupMenuAction (int id) override
    {
        switch (id)
        {
            case 0x5001: alignment_ = 0; break;
            case 0x5002: alignment_ = 1; break;
            case 0x5003: alignment_ = 2; break;
            case 0x5010: bold_   = !bold_;   break;
            case 0x5011: italic_ = !italic_; break;
            case 0x5020: textColor_ = juce::Colours::white;      break;
            case 0x5021: textColor_ = juce::Colours::yellow;     break;
            case 0x5022: textColor_ = juce::Colours::cyan;       break;
            case 0x5023: textColor_ = juce::Colours::lightgreen; break;
            case 0x5024: textColor_ = juce::Colours::coral;      break;
            case 0x5025: textColor_ = juce::Colours::orange;     break;
            case 0x5030: background_ = 0; break;
            case 0x5031: background_ = 1; break;
            case 0x5032: background_ = 2; break;
            default: OpenGlTextEditor::performPopupMenuAction (id); return;
        }
        applyFormattingOverride();
        if (onFormatChanged) onFormatChanged();
    }

    void visibilityChanged() override
    {
        if (isVisible())
            setColour (juce::TextEditor::textColourId, textColor_);
        OpenGlTextEditor::visibilityChanged();
        if (isVisible())
            applyFormattingOverride();
    }

    void resized() override
    {
        OpenGlTextEditor::resized();
        if (isVisible())
            applyFormattingOverride();
    }

    void applyFormattingOverride()
    {
        int styleFlags = (bold_ ? juce::Font::bold : 0) | (italic_ ? juce::Font::italic : 0);
        setColour (juce::TextEditor::textColourId, textColor_);
        auto font = getFont().withStyle (styleFlags);
        applyFontToAllText (font, true);
        juce::Justification just = alignment_ == 1 ? juce::Justification::horizontallyCentred
                                 : alignment_ == 2 ? juce::Justification::right
                                 : juce::Justification::left;
        setJustification (just);
        juce::Colour bg = background_ == 1 ? juce::Colours::black
                        : background_ == 2 ? juce::Colours::transparentBlack
                        : juce::Colours::grey;
        setColour (juce::TextEditor::backgroundColourId, bg);
        redoImage();
    }
};

class CommentPreparation : public PreparationSection, public juce::TextEditor::Listener, public juce::ComponentListener
{
public:
    CommentPreparation (juce::ValueTree v, OpenGlWrapper& open_gl, juce::AudioProcessorGraph::NodeID node, SynthGuiInterface* _synth_gui_interface);
    ~CommentPreparation();

    static std::unique_ptr<PreparationSection> create (const juce::ValueTree& v, SynthGuiInterface* interface)
    {
        return std::make_unique<CommentPreparation> (v, interface->getGui()->open_gl_, juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar (v.getProperty (IDs::nodeID)), interface);
    }

    void resized() override;
    void paintBackground (juce::Graphics& g) override;
    void mouseDoubleClick (const juce::MouseEvent& event) override;

    void textEditorReturnKeyPressed (juce::TextEditor& editor) override;
    void textEditorFocusLost (juce::TextEditor& editor) override;
    void textEditorEscapeKeyPressed (juce::TextEditor& editor) override;
    void textEditorTextChanged (juce::TextEditor& editor) override;

    void valueTreePropertyChanged (juce::ValueTree& v, const juce::Identifier& i) override;

    void componentMovedOrResized (juce::Component& component, bool wasMoved, bool wasResized) override;

private:
    void stopEditing();
    void syncFormattingToItem();
    CommentTextEditor textEditor;
    juce::ResizableBorderComponent resizer;
    juce::ComponentBoundsConstrainer constrainer;
};

#endif // BITKLAVIER2_COMMENTPREPARATION_H
