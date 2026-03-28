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
                                               resizer (this, &constrainer),
                                               textEditor ("commentEditor")
{
    constrainer.setMinimumSize(40, 40);

    auto cItem = std::make_unique<CommentItem>();
    cItem->setCommentText(state.getProperty(IDs::commentText).toString());
    item = std::move(cItem);

    // Item image component renders first (background layer)
    addOpenGlComponent (item->getImageComponent(), true);
    _open_gl.context.executeOnGLThread ([this] (juce::OpenGLContext& context) {
        item->getImageComponent()->init (_open_gl);
    }, false);

    addAndMakeVisible (item.get());
    setSkinOverride (Skin::kDirect);

    width = state.getProperty(IDs::width);
    height = state.getProperty(IDs::height);

    // Text editor as regular JUCE component (handles keyboard/mouse events).
    // The image component must be registered (setting parent_) before any call
    // that triggers resized(), such as setMultiLine.
    addChildComponent(textEditor);
    addOpenGlComponent (textEditor.getImageComponent(), false);
    _open_gl.context.executeOnGLThread ([this] (juce::OpenGLContext& context) {
        textEditor.getImageComponent()->init (_open_gl);
    }, false);
    textEditor.getImageComponent()->setActive(false);

    textEditor.setMultiLine(true);
    textEditor.setReturnKeyStartsNewLine(true);
    textEditor.setColour(juce::TextEditor::backgroundColourId, juce::Colours::grey);
    textEditor.setColour(juce::TextEditor::textColourId, juce::Colours::white);
    textEditor.setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentWhite);
    textEditor.setColour(juce::CaretComponent::caretColourId, juce::Colours::white);
    textEditor.setColour(juce::TextEditor::highlightColourId, juce::Colours::white.withAlpha(0.4f));
    textEditor.setColour(juce::TextEditor::highlightedTextColourId, juce::Colours::white);
    textEditor.setJustification(juce::Justification::left);
    textEditor.addListener(this);

    // Load formatting state from ValueTree
    textEditor.bold_        = (bool)(int) state.getProperty (IDs::commentBold,       0);
    textEditor.italic_      = (bool)(int) state.getProperty (IDs::commentItalic,     0);
    textEditor.alignment_   =       (int) state.getProperty (IDs::commentAlignment,  0);
    textEditor.background_  =       (int) state.getProperty (IDs::commentBackground, 0);
    juce::String colorStr = state.getProperty (IDs::commentColor, juce::Colours::white.toString());
    textEditor.textColor_ = juce::Colour::fromString (colorStr);

    // Callback: save formatting to ValueTree and update the display item
    textEditor.onFormatChanged = [this]
    {
        state.setProperty (IDs::commentBold,       (int) textEditor.bold_,          &undo);
        state.setProperty (IDs::commentItalic,    (int) textEditor.italic_,        &undo);
        state.setProperty (IDs::commentAlignment, textEditor.alignment_,           &undo);
        state.setProperty (IDs::commentBackground, textEditor.background_,         &undo);
        state.setProperty (IDs::commentColor,     textEditor.textColor_.toString(), &undo);
        syncFormattingToItem();
    };

    // Apply initial formatting to the display item
    syncFormattingToItem();

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
    textEditor.getImageComponent()->setActive(true);
    textEditor.setVisible(true);
    textEditor.grabKeyboardFocus();
    textEditor.redoImage();
}

void CommentPreparation::textEditorReturnKeyPressed (juce::TextEditor& editor)
{
    // multiline: return starts a new line; focus lost saves.
}

void CommentPreparation::textEditorFocusLost (juce::TextEditor& editor)
{
    stopEditing();
}

void CommentPreparation::textEditorEscapeKeyPressed (juce::TextEditor& editor)
{
    textEditor.setVisible(false);
    textEditor.getImageComponent()->setActive(false);
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
        textEditor.getImageComponent()->setActive(false);
    }
}

void CommentPreparation::syncFormattingToItem()
{
    if (auto* cItem = dynamic_cast<CommentItem*>(item.get()))
    {
        CommentItem::Formatting f;
        f.bold       = textEditor.bold_;
        f.italic     = textEditor.italic_;
        f.alignment  = textEditor.alignment_;
        f.textColor  = textEditor.textColor_;
        f.background = textEditor.background_;
        cItem->setFormatting (f);
    }
}

void CommentPreparation::valueTreePropertyChanged (juce::ValueTree& v, const juce::Identifier& i)
{
    PreparationSection::valueTreePropertyChanged(v, i);
    if (i == IDs::commentText)
    {
        if (auto* cItem = dynamic_cast<CommentItem*>(item.get()))
            cItem->setCommentText (v.getProperty (IDs::commentText).toString());
    }
    else if (i == IDs::commentBold || i == IDs::commentItalic ||
             i == IDs::commentAlignment || i == IDs::commentColor || i == IDs::commentBackground)
    {
        textEditor.bold_       = (bool)(int) state.getProperty (IDs::commentBold,       0);
        textEditor.italic_     = (bool)(int) state.getProperty (IDs::commentItalic,     0);
        textEditor.alignment_  =       (int) state.getProperty (IDs::commentAlignment,  0);
        textEditor.background_ =       (int) state.getProperty (IDs::commentBackground, 0);
        juce::String colorStr = state.getProperty (IDs::commentColor, juce::Colours::white.toString());
        textEditor.textColor_ = juce::Colour::fromString (colorStr);
        syncFormattingToItem();
    }
}
