#include "CommentPreparation.h"
#include "BKitems/BKItem.h"

CommentPreparation::CommentPreparation (
    juce::ValueTree v,
    OpenGlWrapper& open_gl,
    juce::AudioProcessorGraph::NodeID node,
    SynthGuiInterface* _synth_gui_interface) :
                                               PreparationSection (juce::String ("comment"),
                                                   v,
                                                   open_gl,
                                                   node,
                                                   *_synth_gui_interface->getUndoManager()
                                                   ),
                                               resizer (this, &constrainer)
{
    constrainer.setMinimumSize(40, 40);

    auto cItem = std::make_unique<CommentItem>();
    cItem->setCommentText(state.getProperty(IDs::commentText).toString());
    item = std::move(cItem);

    addOpenGlComponent (item->getImageComponent(), true);
    _open_gl.context.executeOnGLThread ([this] (juce::OpenGLContext& context) {
        item->getImageComponent()->init (_open_gl);
    },
        false);

    addAndMakeVisible (item.get());
    setSkinOverride (Skin::kDirect);

    width = state.getProperty(IDs::width);
    height = state.getProperty(IDs::height);

    addChildComponent(textEditor);
    textEditor.setMultiLine(true);
    textEditor.setReturnKeyStartsNewLine(true);
    textEditor.setColour(juce::TextEditor::backgroundColourId, juce::Colours::grey);
    textEditor.setColour(juce::TextEditor::textColourId, juce::Colours::white);
    textEditor.setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentWhite);
    textEditor.setJustification(juce::Justification::left);
    textEditor.addListener(this);

    addAndMakeVisible(resizer);
    addComponentListener(this);
}

CommentPreparation::~CommentPreparation()
{
    removeComponentListener(this);
}

void CommentPreparation::resized()
{
    PreparationSection::resized();
    textEditor.setBounds(getLocalBounds().reduced(10));
    resizer.setBounds(getLocalBounds());
}

void CommentPreparation::componentMovedOrResized (juce::Component& component, bool wasMoved, bool wasResized)
{
    if (wasResized && &component == this)
    {
        state.setProperty(IDs::width, (float)getWidth(), &undo);
        state.setProperty(IDs::height, (float)getHeight(), &undo);
    }
}

void CommentPreparation::paintBackground (juce::Graphics& g)
{
    PreparationSection::paintBackground (g);
}

void CommentPreparation::mouseDoubleClick (const juce::MouseEvent& event)
{
    textEditor.setText(state.getProperty(IDs::commentText).toString(), false);
    textEditor.setVisible(true);
    textEditor.grabKeyboardFocus();
}

void CommentPreparation::textEditorReturnKeyPressed (juce::TextEditor& editor)
{
    // For multiline, return might just be a new line, but maybe we want cmd+return to finish?
    // User said "when you close the preparation, the text is reflected".
    // I'll use focus lost as the trigger for finishing.
}

void CommentPreparation::textEditorFocusLost (juce::TextEditor& editor)
{
    stopEditing();
}

void CommentPreparation::textEditorEscapeKeyPressed (juce::TextEditor& editor)
{
    textEditor.setVisible(false);
}

void CommentPreparation::textEditorTextChanged (juce::TextEditor& editor)
{
    state.setProperty(IDs::commentText, editor.getText(), &undo);
}

void CommentPreparation::stopEditing()
{
    if (textEditor.isVisible())
    {
        state.setProperty(IDs::commentText, textEditor.getText(), &undo);
        textEditor.setVisible(false);
    }
}

void CommentPreparation::valueTreePropertyChanged (juce::ValueTree& v, const juce::Identifier& i)
{
    PreparationSection::valueTreePropertyChanged(v, i);
    if (i == IDs::commentText)
    {
        if (auto* cItem = dynamic_cast<CommentItem*>(item.get()))
        {
            cItem->setCommentText(v.getProperty(IDs::commentText).toString());
        }
    }
}
