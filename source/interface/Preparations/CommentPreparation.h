#ifndef BITKLAVIER2_COMMENTPREPARATION_H
#define BITKLAVIER2_COMMENTPREPARATION_H

#pragma once
#include "CommentProcessor.h"
#include "FullInterface.h"
#include "PreparationSection.h"
#include "open_gl_image_component.h"
#include <juce_gui_basics/juce_gui_basics.h>

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
    OpenGlTextEditor textEditor;
    juce::ResizableBorderComponent resizer;
    juce::ComponentBoundsConstrainer constrainer;
};

#endif // BITKLAVIER2_COMMENTPREPARATION_H
