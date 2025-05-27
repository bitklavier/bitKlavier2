//
// Created by Davis Polito on 5/27/25.
//

#ifndef KEYMAPPARAMETERVIEW_H
#define KEYMAPPARAMETERVIEW_H

#include "KeymapProcessor.h"
#include "synth_section.h"
#include "default_look_and_feel.h"
#include "BKKeymapKeyboardComponent.h"
class OpenGLKeymapKeyboardComponent: public OpenGlAutoImageComponent<BKKeymapKeyboardComponent> {
public:
    OpenGLKeymapKeyboardComponent(KeymapParams & params)
        : OpenGlAutoImageComponent (&params.keyboard_state) {
        image_component_ = std::make_shared<OpenGlImageComponent>();
        setLookAndFeel(DefaultLookAndFeel::instance());
        image_component_->setComponent(this);
    }

    ~OpenGLKeymapKeyboardComponent()
    {}
    virtual void resized() override {
        OpenGlAutoImageComponent::resized();
        // if (isShowing())
        redoImage();
    }
    void mouseDrag(const juce::MouseEvent &e) override {
        OpenGlAutoImageComponent::mouseDrag(e);
        redoImage();
    }
    void mouseMove(const juce::MouseEvent &e) override {
        OpenGlAutoImageComponent::mouseMove(e);
        redoImage();
    }
    void buttonClicked(juce::Button *b) override {
        OpenGlAutoImageComponent::buttonClicked(b);
        redoImage();
    }
    void textEditorReturnKeyPressed(juce::TextEditor &textEditor) override {
        OpenGlAutoImageComponent::textEditorReturnKeyPressed(textEditor);
        redoImage();
    }

    void textEditorFocusLost(juce::TextEditor &textEditor) override {
        OpenGlAutoImageComponent::textEditorFocusLost(textEditor);
        redoImage();
    }

    void textEditorEscapeKeyPressed(juce::TextEditor &textEditor) override {
        OpenGlAutoImageComponent::textEditorEscapeKeyPressed(textEditor);
        redoImage();
    }

    void textEditorTextChanged(juce::TextEditor &textEditor) override {
        OpenGlAutoImageComponent::textEditorTextChanged(textEditor);
        redoImage();
    }

};
class MidiInputSelectorComponentListBox : public juce::ListBox,
                                                                              private juce::ListBoxModel,
private juce::ChangeListener
{
public:
    MidiInputSelectorComponentListBox (const juce::ValueTree &v)
            : juce::ListBox ({}, nullptr),
            v(v)
    {
        updateDevices();
        setModel (this);
        setOutlineThickness (1);
    }


    void updateDevices()
    {
        items = juce::MidiInput::getAvailableDevices();
    }

    int getNumRows() override
    {
        return items.size();
    }
    static void drawTextLayout (juce::Graphics& g, juce::Component& owner, juce::StringRef text, const juce::Rectangle<int>& textBounds, bool enabled)
    {
        const auto textColour = owner.findColour (juce::ListBox::textColourId, true).withMultipliedAlpha (enabled ? 1.0f : 0.6f);

        juce::AttributedString attributedString { text };
        attributedString.setColour (textColour);
        attributedString.setFont ((float) textBounds.getHeight() * 0.6f);
        attributedString.setJustification (juce::Justification::centredLeft);
        attributedString.setWordWrap (juce::AttributedString::WordWrap::none);

        juce::TextLayout textLayout;
        textLayout.createLayout (attributedString,
                                 (float) textBounds.getWidth(),
                                 (float) textBounds.getHeight());
        textLayout.draw (g, textBounds.toFloat());
    }
    void paintListBoxItem (int row, juce::Graphics& g, int width, int height, bool rowIsSelected) override
    {
        if (juce::isPositiveAndBelow (row, items.size()))
        {
            if (rowIsSelected)
                g.fillAll (findColour (juce::TextEditor::highlightColourId)
                                   .withMultipliedAlpha (0.3f));

            auto item = items[row];
            bool _enabled = isMidiInputDeviceEnabled(item.identifier);




            auto x = getTickX();
            auto tickW = (float) height * 0.75f;

            getLookAndFeel().drawTickBox (g, *this, (float) x - tickW, ((float) height - tickW) * 0.5f, tickW, tickW,
                                          _enabled, true, true, false);

            drawTextLayout (g, *this, item.name, { x + 5, 0, width - x - 5, height }, _enabled);
        }
    }

    void listBoxItemClicked (int row, const juce::MouseEvent& e) override
    {
        selectRow (row);

        if (e.x < getTickX())
            flipEnablement (row);
    }

    void listBoxItemDoubleClicked (int row, const juce::MouseEvent&) override
    {
        flipEnablement (row);
    }

    void returnKeyPressed (int row) override
    {
        flipEnablement (row);
    }

    void paint (juce::Graphics& g) override
    {
        juce::ListBox::paint (g);

        if (items.isEmpty())
        {
            g.setColour (juce::Colours::grey);
            g.setFont (0.5f * (float) getRowHeight());
            g.drawText (noItemsMessage,
                        0, 0, getWidth(), getHeight() / 2,
                        juce::Justification::centred, true);
        }
    }

    int getBestHeight (int preferredHeight)
    {
        auto extra = getOutlineThickness() * 2;

        return juce::jmax (getRowHeight() * 2 + extra,
                     juce::jmin (getRowHeight() * getNumRows() + extra,
                           preferredHeight));
    }

private:
    juce::ValueTree v;
    //==============================================================================
    void changeListenerCallback (juce::ChangeBroadcaster*) override
    {
        resized();
    }
    const juce::String noItemsMessage;
    juce::Array<juce::MidiDeviceInfo> items;
    //std::vector<bitklavier::MidiDeviceWrapper> &enabledMidiInputs;

    bool isMidiInputDeviceEnabled (const juce::String& identifier)
    {
        if (auto child = v.getChildWithProperty(IDs::midiDeviceId, identifier); child.isValid())
        {
           return true;
        }
//        for (auto mi : objects) {
//
//            if (mi->identifier == identifier) {
//                return true;
//            }
//        }
        return false;
    }
    void flipEnablement (const int row)
    {
        if (juce::isPositiveAndBelow (row, items.size()))
        {
            auto identifier = items[row].identifier;
            if (!isMidiInputDeviceEnabled(identifier))
            {

               juce::ValueTree t(IDs::midiInput);
               t.setProperty(IDs::midiDeviceId, identifier,nullptr);
               v.appendChild(t, nullptr);
            }
            else
            {

                v.removeChild(v.getChildWithProperty(IDs::midiDeviceId, identifier), nullptr);

            }

            //deviceManager.setMidiInputDeviceEnabled (identifier, ! deviceManager.isMidiInputDeviceEnabled (identifier));
        }
        resized(); // should probably do this with a changelistener/changebroadcaster
    }

    int getTickX() const
    {
        return getRowHeight();
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiInputSelectorComponentListBox)
};


class OpenGlMidiSelector : public OpenGlAutoImageComponent<MidiInputSelectorComponentListBox> {
public:
    OpenGlMidiSelector(const juce::ValueTree& v) :
            OpenGlAutoImageComponent<MidiInputSelectorComponentListBox>(v) {
        image_component_ = std::make_shared<OpenGlImageComponent>();
        setLookAndFeel(DefaultLookAndFeel::instance());
        image_component_->setComponent(this);
    }

    virtual void resized() override {
        OpenGlAutoImageComponent<MidiInputSelectorComponentListBox>::resized();
        if (isShowing())
            redoImage();
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OpenGlMidiSelector)
};


class KeymapParameterView : public SynthSection {
public:
    void setColorRecursively(juce::Component *component, int color_id, const juce::Colour& color) {
        component->setColour(color_id, color);
        for (juce::Component *child : component->getChildren())
            setColorRecursively(child, color_id, color);
    }
    // Constructor method that takes two arguments: a smart pointer to a KeymapProcessor,
    // and a reference to an OpenGlWrapper
    KeymapParameterView(KeymapProcessor &, OpenGlWrapper &);

    // Public function definitions for the class, which override the base class methods for
    // initializing, rendering, resizing, and painting OpenGl components
    //    void initOpenGlComponents(OpenGlWrapper &open_gl) override;



    void resized() override;

    void paintBackground(juce::Graphics &g) override {
        SynthSection::paintContainer(g);
        paintHeadingText(g);
        paintBorder(g);
        paintKnobShadows(g);
        paintChildrenBackgrounds(g);
    }

    int getViewPosition() {
        int view_height = getHeight();
        return view_height; //std::max(0, std::min<int>(selections_.size() * getRowHeight() - view_height, view_position_));
    }

    ~KeymapParameterView();


private:
    OpenGlWrapper &opengl;
    // Private function definitions and member variables for the KeymapParameterView class
    void redoImage();

    KeymapParams *params = nullptr;
    KeymapProcessor &proc;
    std::unique_ptr<OpenGLKeymapKeyboardComponent> keyboard_component_;
    std::unique_ptr<OpenGlMidiSelector> midi_selector_;

};
#endif //KEYMAPPARAMETERVIEW_H
