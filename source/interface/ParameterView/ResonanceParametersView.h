// Copyright (C) 2022-2025 Dan Trueman
// SPDX-License-Identifier: GPL-3.0-or-later

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
        OpenGlWrapper* open_gl,
        SynthBase* synth = nullptr,
        juce::AudioProcessorGraph::NodeID nodeId = {}) : SynthSection (""), sparams_ (params), synth_(synth), nodeId_(nodeId)
    {
        // the name that will appear in the UI as the name of the section
        setName ("resonance");

        // every section needs a LaF
        //  main settings for this LaF are in assets/default.bitklavierskin
        //  different from the bk LaF that we've taken from the old JUCE, to support the old UI elements
        //  we probably want to merge these in the future, but ok for now
        setLookAndFeel (DefaultLookAndFeel::instance());
        setComponentID (name);

        prepTitle = std::make_shared<PlainTextComponent>(getName(), getName());
        addOpenGlComponent(prepTitle);
        prepTitle->setJustification(juce::Justification::centredLeft);
        prepTitle->setFontType (PlainTextComponent::kTitle);
        prepTitle->setRotation (-90);

        setSkinOverride(Skin::kDirect);

        // pluginState is really more like preparationState; the state holder for this preparation (not the whole app/plugin)
        // we need to grab the listeners for this preparation here, so we can pass them to components below
        auto& listeners = pluginState.getParameterListeners();

        for (auto& param_ : *params.getFloatParams())
        {
            if ( // make group of params to display together
                param_->paramID == "rpresence" ||
                param_->paramID == "rsustain" ||
                param_->paramID == "rvariance" ||
                param_->paramID == "rstretch")
                //param_->paramID == "rsmoothness") // i'm not persuaded this is a useful parameter to expose
            {
                auto slider = std::make_unique<SynthSlider> (param_->paramID,param_->getModParam());
                if (param_->paramID == "rpresence")
                    slider->setTooltip ("Controls onset speed of resonance - higher values create a faster start");
                else if (param_->paramID == "rsustain")
                    slider->setTooltip ("Controls how long the resonance sustains");
                else if (param_->paramID == "rvariance")
                    slider->setTooltip ("Overlap sensitivity - higher values create more overlap between partials, resulting in more sympathetic resonance with more variable tuning of partials");
                else if (param_->paramID == "rstretch")
                    slider->setTooltip ("Inharmonicity stretch - applies Railsback-style stretch to partials, simulating natural piano string behavior");
                auto attachment = std::make_unique<chowdsp::SliderAttachment> (*param_.get(), listeners, *slider.get(), pluginState.undoManager);
                slider->addAttachment(attachment.get()); // necessary for mods to be able to display properly
                addSlider (slider.get()); // adds the slider to the synthSection
                slider->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
                slider->setShowPopupOnHover(true);
                auto slider_label = std::make_shared<PlainTextComponent>(slider->getName(), param_->getName(20));
                addOpenGlComponent(slider_label);
                slider_label->setTextSize (12.0f);
                slider_label->setJustification(juce::Justification::centred);
                slider_labels.emplace_back(slider_label);
                floatAttachments.emplace_back (std::move (attachment));
                if ( // make group of params to display together
                    param_->paramID == "rpresence" ||
                    param_->paramID == "rsustain")
                {
                    _sliders_row1.emplace_back (std::move (slider));
                }
                else
                {
                    _sliders_row2.emplace_back (std::move (slider));
                }

            }
        }

        variousControlsBorder = std::make_shared<OpenGL_LabeledBorder>("various controls border", "Resonance Controls");
        addBorder(variousControlsBorder.get());

        fundamentalKeyboard = std::make_unique<OpenGLKeymapKeyboardComponent>(params.fundamentalKeymap, false, true, false, false);
        fundamentalKeyboard->setComponentID("fundamentalKeyboard");
        fundamentalKeyboard->defaultState = params.fundamentalKeymapStateChanges.defaultState;
        addStateModulatedComponent(fundamentalKeyboard.get());
        fundamentalKeyboard->setName("fundamental");
        fundamentalKeyboard->setAvailableRange(0, numKeys);
        fundamentalKeyboard->setOctaveForMiddleC(5);
        fundamentalKeyboard->addMyListener(this);

        fundamentalKeyboard_label = std::make_shared<PlainTextComponent>("fundamental", "Fundamental");
        addOpenGlComponent(fundamentalKeyboard_label);
        fundamentalKeyboard_label->setJustification(juce::Justification::centredBottom);

        closestKeyboard = std::make_unique<OpenGLKeymapKeyboardComponent>(params.closestKeymap, false, false, false, false);
        closestKeyboard->setComponentID("closestKeyboard");
        closestKeyboard->defaultState = params.closestKeymapStateChanges.defaultState;
        addStateModulatedComponent(closestKeyboard.get());
        closestKeyboard->setName("closest");
        closestKeyboard->setAvailableRange(0, numKeys);
        closestKeyboard->setOctaveForMiddleC(5);
        closestKeyboard->addMyListener(this);

        closestKeyboard_label = std::make_shared<PlainTextComponent>("closest", "Keys for Closest Partials");
        addOpenGlComponent(closestKeyboard_label);
        closestKeyboard_label->setJustification(juce::Justification::centredBottom);

        heldKeysKeyboard = std::make_unique<OpenGLKeymapKeyboardComponent>(params.heldKeymap, false, false, true);
        heldKeysKeyboard->setComponentID("heldKeysKeyboard");
        heldKeysKeyboard->defaultState = params.heldKeymapStateChanges.defaultState;
        addStateModulatedComponent(heldKeysKeyboard.get());
        heldKeysKeyboard->setName("heldKeys");
        heldKeysKeyboard->addMyListener(this);

        offsetsKeyboard = std::make_unique<OpenGLAbsoluteKeyboardSlider>(dynamic_cast<ResonanceParams*>(&params)->offsetsKeyboardState);
        offsetsKeyboard->setComponentID("offsetsKeyboard");
        addStateModulatedComponent(offsetsKeyboard.get());
        offsetsKeyboard->setName("offsets");
        offsetsKeyboard->setAvailableRange(0, numKeys);
        offsetsKeyboard->setOctaveForMiddleC(5);
        offsetsKeyboard->onChange = [&params]() { params.spectrum->setParameterValue(SpectralType::Custom); };

        offsetsKeyboard_label = std::make_shared<PlainTextComponent>("offsets", "Offsets from ET (cents) for Partials");
        addOpenGlComponent(offsetsKeyboard_label);
        offsetsKeyboard_label->setJustification(juce::Justification::centredBottom);

        gainsKeyboard = std::make_unique<OpenGLAbsoluteKeyboardSlider>(dynamic_cast<ResonanceParams*>(&params)->gainsKeyboardState);
        gainsKeyboard->setComponentID("gainsKeyboard");
        addStateModulatedComponent(gainsKeyboard.get());
        gainsKeyboard->setName("gains");
        gainsKeyboard->setAvailableRange(0, numKeys);
        gainsKeyboard->setMinMidMaxValues(-100.f, 0.f, 6.f, 2); // min, mid, max, display resolution; dBFS
        gainsKeyboard->setOctaveForMiddleC(5);
        gainsKeyboard->onChange = [&params]() { params.spectrum->setParameterValue(SpectralType::Custom); };

        gainsKeyboard_label = std::make_shared<PlainTextComponent>("gains", "Gains for Partials");
        addOpenGlComponent(gainsKeyboard_label);
        gainsKeyboard_label->setJustification(juce::Justification::centredBottom);

        allNotesOffButton = std::make_unique<SynthButton>("allNotesOff");
        allNotesOffButton_attachment = std::make_unique<chowdsp::ButtonAttachment>(params.rAllNotesOff, listeners,*allNotesOffButton, nullptr);
        allNotesOffButton->setComponentID("allNotesOff");
        addSynthButton(allNotesOffButton.get(), true);
        allNotesOffButton->setText("all notes off!");
        allNotesOffButton->setTooltip ("Stop all currently sounding resonance");
        allNotesOffButton->setToggleable(true);

        // ADSR
        envSection = std::make_unique<EnvelopeSection>( params.env ,listeners, *this);
        addSubSection (envSection.get());

        // spectrum choices
        spectrum_combo_box = std::make_unique<OpenGLComboBox>(params.spectrum->paramID.toStdString());
        spectrum_attachment = std::make_unique<chowdsp::ComboBoxAttachment>(*params.spectrum.get(), listeners, *spectrum_combo_box, pluginState.undoManager);
        spectrum_combo_box->setTooltip ("Select the spectral type defining the partial structure");
        addComboBox(spectrum_combo_box.get(), true, true);

        // the level meter and output gain slider (right side of preparation popup)
        // need to pass it the param.outputGain and the listeners so it can attach to the slider and update accordingly
        levelMeter = std::make_unique<PeakMeterSection>(name, params.outputGain, listeners, &params.outputLevels);
        levelMeter->setLabel("Main");
        levelMeter->setVolumeTooltip ("Master output level for this preparation");
        addSubSection(levelMeter.get());

        // similar for send level meter/slider
        sendLevelMeter = std::make_unique<PeakMeterSection>(name, params.outputSendGain, listeners, &params.sendLevels);
        sendLevelMeter->setLabel("Send");
        sendLevelMeter->setVolumeTooltip ("Signal level sent to connected effects");
        addSubSection(sendLevelMeter.get());

        muteButton_ = std::make_unique<SynthButton>("mute");
        muteButton_->setText("M");
        muteButton_->setTooltip ("Mute this preparation. Option-click to mute only this one.");
        addSynthButton(muteButton_.get(), true);
        muteButton_->onClick = [this]() {
            bool isOptionClick = juce::ModifierKeys::currentModifiers.isAltDown();
            bool newMuted = !sparams_.userMuted_.load(std::memory_order_relaxed);
            sparams_.userMuted_.store(newMuted, std::memory_order_relaxed);
            sparams_.muted_.store(newMuted || sparams_.soloMuted_.load(std::memory_order_relaxed),
                                  std::memory_order_relaxed);
            if (synth_) synth_->coordinateMuteChanged(nodeId_, newMuted, isOptionClick);
        };

        soloButton_ = std::make_unique<SynthButton>("solo");
        soloButton_->setText("S");
        soloButton_->setTooltip ("Solo this preparation. Option-click to solo only this one.");
        addSynthButton(soloButton_.get(), true);
        soloButton_->onClick = [this]() {
            bool isOptionClick = juce::ModifierKeys::currentModifiers.isAltDown();
            bool newSoloed = !sparams_.soloed_.load(std::memory_order_relaxed);
            sparams_.soloed_.store(newSoloed, std::memory_order_relaxed);
            if (synth_) synth_->coordinateSoloChanged(nodeId_, isOptionClick);
        };

        resonanceCallbacks += {listeners.addParameterListener(
            params.spectrum,
            chowdsp::ParameterListenerThread::MessageThread,
            [this, &params]() {
                params.setSpectrumFromMenu(params.spectrum->getIndex());
                fundamentalKeyboard->redoImage();
                offsetsKeyboard->redoImage();
                gainsKeyboard->redoImage();
                closestKeyboard->redoImage();
            })
        };

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
        //paintHeadingText (g);
        paintBorder (g);
        paintKnobShadows (g);

        // for (auto& slider : _sliders)
        // {
        //     //drawLabelForComponent (g, slider->getName(), slider.get());
        //     if (slider->getName() == "rsustain") drawLabelForComponent(g, TRANS("Sustain"), slider.get());
        //     if (slider->getName() == "rvariance") drawLabelForComponent(g, TRANS("Overlap Sensitivity"), slider.get());
        //     if (slider->getName() == "rpresence") drawLabelForComponent(g, TRANS("Presence"), slider.get());
        //     if (slider->getName() == "rstretch") drawLabelForComponent(g, TRANS("Stretch"), slider.get());
        //     //if (slider->getName() == "rsmoothness") drawLabelForComponent(g, TRANS("Smoothness"), slider.get());
        // }

        paintChildrenBackgrounds (g);
    }

    void BKKeymapKeyboardChanged (juce::String name, std::bitset<128> keys, int lastKey, juce::ModifierKeys mods = juce::ModifierKeys()) override
    {
        if (name == "heldKeys")
        {
            sparams_.heldKeymap_changedInUI = lastKey;
        }
        else if (name == "fundamental" || name == "closest")
        {
            sparams_.spectrum->setParameterValue (SpectralType::Custom);
        }
    }

    chowdsp::ScopedCallbackList sliderChangedCallback;

    // prep title, vertical, left side
    std::shared_ptr<PlainTextComponent> prepTitle;

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
    std::vector<std::unique_ptr<SynthSlider>> _sliders_row1;
    std::vector<std::unique_ptr<SynthSlider>> _sliders_row2;
    std::vector<std::unique_ptr<chowdsp::SliderAttachment>> floatAttachments;
    std::vector<std::shared_ptr<PlainTextComponent> > slider_labels;

    // for knobs
    std::shared_ptr<OpenGL_LabeledBorder> variousControlsBorder;

    std::unique_ptr<OpenGLComboBox> spectrum_combo_box;
    std::unique_ptr<chowdsp::ComboBoxAttachment> spectrum_attachment;

    std::unique_ptr<SynthButton> muteButton_;
    std::unique_ptr<SynthButton> soloButton_;

    SynthBase* synth_ = nullptr;
    juce::AudioProcessorGraph::NodeID nodeId_;

    // for all notes off
    std::unique_ptr<SynthButton> allNotesOffButton;
    std::unique_ptr<chowdsp::ButtonAttachment> allNotesOffButton_attachment;

    chowdsp::ScopedCallbackList resonanceCallbacks;

    ResonanceParams& sparams_;

    void timerCallback(void) override;

    void resized() override;
};

#endif //BITKLAVIER0_RESONANCEPARAMETERSVIEW_H
