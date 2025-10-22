//
// Created by Dan Trueman on 10/8/25.
//

#ifndef BITKLAVIER0_RESONANCEPARAMETERSVIEW_H
#define BITKLAVIER0_RESONANCEPARAMETERSVIEW_H

#include "ResonanceProcessor.h"
#include "peak_meter_section.h"
#include "synth_section.h"
#include "synth_slider.h"
#include "synth_button.h"
#include "envelope_section.h"
#include "OpenGL_AbsoluteKeyboardSlider.h"
#include "OpenGL_KeymapKeyboard.h"

class ResonanceParametersView : public SynthSection, juce::Timer, BKKeymapKeyboardComponent::Listener
{
public:
    ResonanceParametersView (chowdsp::PluginState& pluginState,
        ResonanceParams& params,
        juce::String name,
        OpenGlWrapper* open_gl) : SynthSection (""), sparams_ (params)
    {
        // the name that will appear in the UI as the name of the section
        setName ("resonance");

        // every section needs a LaF
        //  main settings for this LaF are in assets/default.bitklavierskin
        //  different from the bk LaF that we've taken from the old JUCE, to support the old UI elements
        //  we probably want to merge these in the future, but ok for now
        setLookAndFeel (DefaultLookAndFeel::instance());
        setComponentID (name);

        // pluginState is really more like preparationState; the state holder for this preparation (not the whole app/plugin)
        // we need to grab the listeners for this preparation here, so we can pass them to components below
        auto& listeners = pluginState.getParameterListeners();

        for (auto& param_ : *params.getFloatParams())
        {
            if ( // make group of params to display together
                param_->paramID == "rpresence" ||
                param_->paramID == "rsustain" ||
                param_->paramID == "rvariance")
                //param_->paramID == "rsmoothness") // i'm not persuaded this is a useful parameter to expose
            {
                auto slider = std::make_unique<SynthSlider> (param_->paramID,param_->getModParam());
                auto attachment = std::make_unique<chowdsp::SliderAttachment> (*param_.get(), listeners, *slider.get(), nullptr);
                slider->addAttachment(attachment.get()); // necessary for mods to be able to display properly
                addSlider (slider.get()); // adds the slider to the synthSection
                slider->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
                slider->setShowPopupOnHover(true);
                floatAttachments.emplace_back (std::move (attachment));
                _sliders.emplace_back (std::move (slider));
            }
        }

        fundamentalKeyboard = std::make_unique<OpenGLKeymapKeyboardComponent>(params.fundamentalKeymap, false, true);
        addStateModulatedComponent(fundamentalKeyboard.get());
        fundamentalKeyboard->setName("fundamental");
        fundamentalKeyboard->setAvailableRange(0, numKeys);
        fundamentalKeyboard->setOctaveForMiddleC(5);

        fundamentalKeyboard_label = std::make_shared<PlainTextComponent>("fundamental", "Held Key/Fundamental");
        addOpenGlComponent(fundamentalKeyboard_label);
        fundamentalKeyboard_label->setTextSize (12.0f);
        fundamentalKeyboard_label->setJustification(juce::Justification::centredBottom);

        closestKeyboard = std::make_unique<OpenGLKeymapKeyboardComponent>(params.closestKeymap, false);
        addStateModulatedComponent(closestKeyboard.get());
        closestKeyboard->setName("closest");
        closestKeyboard->setAvailableRange(0, numKeys);
        closestKeyboard->setOctaveForMiddleC(5);

        closestKeyboard_label = std::make_shared<PlainTextComponent>("closest", "Resonant Keys/Partials");
        addOpenGlComponent(closestKeyboard_label);
        closestKeyboard_label->setTextSize (12.0f);
        closestKeyboard_label->setJustification(juce::Justification::centredBottom);

        heldKeysKeyboard = std::make_unique<OpenGLKeymapKeyboardComponent>(params.heldKeymap, false);
        addStateModulatedComponent(heldKeysKeyboard.get());
        heldKeysKeyboard->setName("heldKeys");
        heldKeysKeyboard->addMyListener(this);

        offsetsKeyboard = std::make_unique<OpenGLAbsoluteKeyboardSlider>(dynamic_cast<ResonanceParams*>(&params)->offsetsKeyboardState);
        addStateModulatedComponent(offsetsKeyboard.get());
        offsetsKeyboard->setName("offsets");
        offsetsKeyboard->setAvailableRange(0, numKeys);
        offsetsKeyboard->setOctaveForMiddleC(5);

        offsetsKeyboard_label = std::make_shared<PlainTextComponent>("offsets", "Offsets from ET (cents) for Partials");
        addOpenGlComponent(offsetsKeyboard_label);
        offsetsKeyboard_label->setTextSize (12.0f);
        offsetsKeyboard_label->setJustification(juce::Justification::centredBottom);

        gainsKeyboard = std::make_unique<OpenGLAbsoluteKeyboardSlider>(dynamic_cast<ResonanceParams*>(&params)->gainsKeyboardState);
        addStateModulatedComponent(gainsKeyboard.get());
        gainsKeyboard->setName("gains");
        gainsKeyboard->setAvailableRange(0, numKeys);
        gainsKeyboard->setMinMidMaxValues(0., 0.5, 1., 2); // min, mid, max, display resolution
        gainsKeyboard->setOctaveForMiddleC(5);

        gainsKeyboard_label = std::make_shared<PlainTextComponent>("gains", "Gains for Partials");
        addOpenGlComponent(gainsKeyboard_label);
        gainsKeyboard_label->setTextSize (12.0f);
        gainsKeyboard_label->setJustification(juce::Justification::centredBottom);

        // ADSR
        envSection = std::make_unique<EnvelopeSection>( params.env ,listeners, *this);
        addSubSection (envSection.get());

        // the level meter and output gain slider (right side of preparation popup)
        // need to pass it the param.outputGain and the listeners so it can attach to the slider and update accordingly
        levelMeter = std::make_unique<PeakMeterSection>(name, params.outputGain, listeners, &params.outputLevels);
        levelMeter->setLabel("Main");
        addSubSection(levelMeter.get());

        // similar for send level meter/slider
        sendLevelMeter = std::make_unique<PeakMeterSection>(name, params.outputSendGain, listeners, &params.sendLevels);
        sendLevelMeter->setLabel("Send");
        addSubSection(sendLevelMeter.get());

        startTimer(50);
    }

    ~ResonanceParametersView()
    {
        stopTimer();
    }

    void paintBackground (juce::Graphics& g) override
    {
        setLabelFont(g);
        SynthSection::paintContainer (g);
        paintHeadingText (g);
        paintBorder (g);
        paintKnobShadows (g);

        for (auto& slider : _sliders)
        {
            //drawLabelForComponent (g, slider->getName(), slider.get());
            if (slider->getName() == "rsustain") drawLabelForComponent(g, TRANS("Sustain"), slider.get());
            if (slider->getName() == "rvariance") drawLabelForComponent(g, TRANS("Overlap Variance"), slider.get());
            if (slider->getName() == "rpresence") drawLabelForComponent(g, TRANS("Presence"), slider.get());
            //if (slider->getName() == "rsmoothness") drawLabelForComponent(g, TRANS("Smoothness"), slider.get());
        }

        paintChildrenBackgrounds (g);
    }

    void BKKeymapKeyboardChanged (juce::String name, std::bitset<128> keys, int lastKey) override
    {
        DBG("BKKeymapKeyboardChanged called in ResonanceParametersView " + juce::String(lastKey));
        sparams_.heldKeymap_changedInUI = lastKey; // notify processor that the held keymap has changed vai the UI
    }

    chowdsp::ScopedCallbackList sliderChangedCallback;

    std::unique_ptr<OpenGLKeymapKeyboardComponent> fundamentalKeyboard;
    std::unique_ptr<OpenGLKeymapKeyboardComponent> closestKeyboard;
    std::unique_ptr<OpenGLKeymapKeyboardComponent> heldKeysKeyboard;
    std::unique_ptr<OpenGLAbsoluteKeyboardSlider> offsetsKeyboard;
    std::unique_ptr<OpenGLAbsoluteKeyboardSlider> gainsKeyboard;
    int numKeys = TotalNumberOfPartialKeysInUI;

    std::shared_ptr<PlainTextComponent> fundamentalKeyboard_label;
    std::shared_ptr<PlainTextComponent> closestKeyboard_label;
    std::shared_ptr<PlainTextComponent> offsetsKeyboard_label;
    std::shared_ptr<PlainTextComponent> gainsKeyboard_label;

    std::unique_ptr<EnvelopeSection> envSection;

    // level meters with gain sliders
    std::shared_ptr<PeakMeterSection> levelMeter;
    std::shared_ptr<PeakMeterSection> sendLevelMeter;

    // place to store generic sliders/knobs for this prep, with their attachments for tracking/updating values
    std::vector<std::unique_ptr<SynthSlider>> _sliders;
    std::vector<std::unique_ptr<chowdsp::SliderAttachment>> floatAttachments;

    ResonanceParams& sparams_;

    void timerCallback(void) override;

    void resized() override;
};

#endif //BITKLAVIER0_RESONANCEPARAMETERSVIEW_H
