//
// Created by Davis Polito on 5/27/25.
//

#ifndef KEYMAPPARAMETERVIEW_H
#define KEYMAPPARAMETERVIEW_H

#include "KeymapProcessor.h"
#include "synth_section.h"
#include "synth_slider.h"
#include "synth_button.h"
#include "default_look_and_feel.h"
#include "OpenGL_KeymapKeyboard.h"
#include "OpenGL_VelocityMinMaxSlider.h"
#include "VelocityMinMaxParams.h"
#include "OpenGL_LabeledBorder.h"


class MidiInputSelectorComponentListBox : public juce::ListBox, private juce::ListBoxModel, private juce::ChangeListener
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
        setColour(backgroundColourId, juce::Colours::transparentBlack);
        setColour(outlineColourId, juce::Colours::transparentBlack);

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


class KeymapParameterView : public SynthSection, juce::Timer
{
public:
    KeymapParameterView(KeymapProcessor &, KeymapParams& kparams, juce::String name, OpenGlWrapper *open_gl);
    ~KeymapParameterView();
    void resized() override;

    void setColorRecursively(juce::Component *component, int color_id, const juce::Colour& color) {
        component->setColour(color_id, color);
        for (juce::Component *child : component->getChildren())
            setColorRecursively(child, color_id, color);
    }

    void paintBackground(juce::Graphics &g) override {
        SynthSection::paintContainer(g);
        paintBorder(g);
        paintKnobShadows(g);
        paintChildrenBackgrounds(g);
        drawVelocityCurve(g);
    }

    int getViewPosition() {
        int view_height = getHeight();
        return view_height; //std::max(0, std::min<int>(selections_.size() * getRowHeight() - view_height, view_position_));
    }

    void drawVelocityCurve(juce::Graphics &g);

private:
    void timerCallback(void) override;

    KeymapParams& params;
    KeymapProcessor &proc;

    // prep title, vertical, left side
    std::shared_ptr<PlainTextComponent> prepTitle;

    std::unique_ptr<OpenGLKeymapKeyboardComponent> keyboard_component_;
    std::unique_ptr<OpenGlMidiSelector> midi_selector_;

    // velocity curve knobs
    std::unique_ptr<SynthSlider> asymmetricalWarp_knob;
    std::unique_ptr<chowdsp::SliderAttachment> asymmetricalWarp_knob_attach;

    std::unique_ptr<SynthSlider> symmetricalWarp_knob;
    std::unique_ptr<chowdsp::SliderAttachment> symmetricalWarp_knob_attach;

    std::unique_ptr<SynthSlider> scale_knob;
    std::unique_ptr<chowdsp::SliderAttachment> scale_knob_attach;

    std::unique_ptr<SynthSlider> offset_knob;
    std::unique_ptr<chowdsp::SliderAttachment> offset_knob_attach;

    std::unique_ptr<SynthButton> invert;
    std::unique_ptr<chowdsp::ButtonAttachment> invert_attachment;

    juce::Rectangle<int> velocityCurveBox;
    std::shared_ptr<PlainTextComponent> asymmetricalWarp_knob_label;
    std::shared_ptr<PlainTextComponent> symmetricalWarp_knob_label;
    std::shared_ptr<PlainTextComponent> scale_knob_label;
    std::shared_ptr<PlainTextComponent> offset_knob_label;
    std::shared_ptr<OpenGL_LabeledBorder> velocityCurveControls;

    std::shared_ptr<PlainTextComponent> velocityInLabel;
    std::shared_ptr<PlainTextComponent> velocityOutLabel;
    chowdsp::ScopedCallbackList drawCalls;
    // pad values for curve drawing
    int leftPadding = 25;        // space for "velocity out" label
    int rightPadding = 4;       // space for the graph's right edge to fully display
    int topPadding = 6;         // space for the dot and graph's top edge to fully display
    int bottomPadding = 25;      // space for "velocity in" label
    int leftAdditional = 40;    // additional space for number labels on left

    // min/max filter slider
    std::unique_ptr<OpenGL_VelocityMinMaxSlider> velocityMinMaxSlider;
    void stopAllTimers() override {
        stopTimer();
    }
};
#endif //KEYMAPPARAMETERVIEW_H
