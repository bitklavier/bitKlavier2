// Copyright (C) 2022-2025 Dan Trueman
// SPDX-License-Identifier: GPL-3.0-or-later

//
// Created by Myra Norton on 11/14/25.
//

#ifndef BITKLAVIER2_CompressorPARAMETERVIEW_H
#define BITKLAVIER2_CompressorPARAMETERVIEW_H
#include "CompressorProcessor.h"
#include "OpenGL_LabeledBorder.h"
#include "default_look_and_feel.h"
#include "peak_meter_section.h"
#include "synth_section.h"
#include "synth_slider.h"
#include "OpenGL_CompressorMeter.h"

class CompressorParameterView : public SynthSection, public juce::Timer
{
public:
    CompressorParameterView (chowdsp::PluginState& pluginState, CompressorParams& params, juce::String name, OpenGlWrapper* open_gl, bool isPrepVersion = false,
                             SynthBase* synth = nullptr, juce::AudioProcessorGraph::NodeID nodeId = {}) : SynthSection (""), compressorParams_ (params), isPrepVersion_ (isPrepVersion), synth_(synth), nodeId_(nodeId)
    {
        // the name that will appear in the UI as the name of the section
        setName ("compressor");
        setLookAndFeel (DefaultLookAndFeel::instance());
        setComponentID (name);
        setSkinOverride(Skin::kDirect);

        // pluginState is really more like preparationState; the state holder for this preparation (not the whole app/plugin)
        // we need to grab the listeners for this preparation here, so we can pass them to components below
        auto& listeners = pluginState.getParameterListeners();

        prepTitle = std::make_shared<PlainTextComponent>(getName(), getName());
        addOpenGlComponent(prepTitle);
        prepTitle->setJustification(juce::Justification::centredLeft);
        prepTitle->setFontType (PlainTextComponent::kTitle);
        prepTitle->setRotation (-90);

        // Compressor Meter
        compressorMeter = std::make_unique<OpenGL_CompressorMeter> (&params, listeners);
        addOpenGlComponent (compressorMeter->getImageComponent());
        addAndMakeVisible(compressorMeter.get());

        // power EQ button
        activeCompressor_toggle = std::make_unique<SynthButton>(compressorParams_.activeCompressor->paramID);
        activeCompressor_attachment = std::make_unique<chowdsp::ButtonAttachment>(compressorParams_.activeCompressor, listeners, *activeCompressor_toggle, pluginState.undoManager);
        activeCompressor_toggle->setComponentID(compressorParams_.activeCompressor->paramID);
        addSynthButton(activeCompressor_toggle.get(), true);
        activeCompressor_toggle->setText("power");


        // Internal (graph audio) input meter
        inLevelMeter = std::make_unique<PeakMeterSection>(name, params.inputGain, listeners, &params.inputLevels);
        inLevelMeter->setLabel(isPrepVersion_ ? "Internal" : "In");
        addSubSection(inLevelMeter.get());

        // Main output meter
        levelMeter = std::make_unique<PeakMeterSection>(name, params.outputGain, listeners, &params.outputLevels);
        levelMeter->setLabel(isPrepVersion_ ? "Main" : "Out");
        addSubSection(levelMeter.get());

        if (isPrepVersion_)
        {
            // External (mic/line/sidechain) input meter
            externalLevelMeter = std::make_shared<PeakMeterSection>(name, params.externalGain, listeners, &params.externalLevels);
            externalLevelMeter->setLabel("External");
            addSubSection(externalLevelMeter.get());

            // Send output meter
            sendLevelMeter = std::make_shared<PeakMeterSection>(name, params.outputSend, listeners, &params.sendLevels);
            sendLevelMeter->setLabel("Send");
            addSubSection(sendLevelMeter.get());

            muteButton_ = std::make_unique<SynthButton>("mute");
            muteButton_->setText("M");
            addSynthButton(muteButton_.get(), true);
            muteButton_->onClick = [this]() {
                bool isOptionClick = juce::ModifierKeys::currentModifiers.isAltDown();
                bool newMuted = !compressorParams_.userMuted_.load(std::memory_order_relaxed);
                compressorParams_.userMuted_.store(newMuted, std::memory_order_relaxed);
                compressorParams_.muted_.store(newMuted || compressorParams_.soloMuted_.load(std::memory_order_relaxed),
                                               std::memory_order_relaxed);
                if (synth_) synth_->coordinateMuteChanged(nodeId_, newMuted, isOptionClick);
            };

            soloButton_ = std::make_unique<SynthButton>("solo");
            soloButton_->setText("S");
            addSynthButton(soloButton_.get(), true);
            soloButton_->onClick = [this]() {
                bool isOptionClick = juce::ModifierKeys::currentModifiers.isAltDown();
                bool newSoloed = !compressorParams_.soloed_.load(std::memory_order_relaxed);
                compressorParams_.soloed_.store(newSoloed, std::memory_order_relaxed);
                if (synth_) synth_->coordinateSoloChanged(nodeId_, isOptionClick);
            };
        }

        // knobs
        attack_knob = std::make_unique<SynthSlider>(params.attack->paramID, params.attack->getModParam());
        addSlider(attack_knob.get());
        attack_knob->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        attack_knob->setPopupPlacement(juce::BubbleComponent::below);
        attack_knob->setShowPopupOnHover(true);
        attack_knob_attachment = std::make_unique<chowdsp::SliderAttachment>(params.attack, listeners, *attack_knob, pluginState.undoManager);
        attack_knob->addAttachment(attack_knob_attachment.get());

        release_knob = std::make_unique<SynthSlider>(params.release->paramID, params.release->getModParam());
        addSlider(release_knob.get());
        release_knob->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        release_knob->setPopupPlacement(juce::BubbleComponent::below);
        release_knob->setShowPopupOnHover(true);
        release_knob_attachment = std::make_unique<chowdsp::SliderAttachment>(params.release, listeners, *release_knob, pluginState.undoManager);
        release_knob->addAttachment(release_knob_attachment.get());

        threshold_knob = std::make_unique<SynthSlider>(params.threshold->paramID, params.threshold->getModParam());
        addSlider(threshold_knob.get());
        threshold_knob->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        threshold_knob->setPopupPlacement(juce::BubbleComponent::below);
        threshold_knob->setShowPopupOnHover(true);
        threshold_knob_attachment = std::make_unique<chowdsp::SliderAttachment>(params.threshold, listeners, *threshold_knob, pluginState.undoManager);
        threshold_knob->addAttachment(threshold_knob_attachment.get());

        makeup_knob = std::make_unique<SynthSlider>(params.makeup->paramID, params.makeup->getModParam());
        addSlider(makeup_knob.get());
        makeup_knob->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        makeup_knob->setPopupPlacement(juce::BubbleComponent::below);
        makeup_knob->setShowPopupOnHover(true);
        makeup_knob_attachment = std::make_unique<chowdsp::SliderAttachment>(params.makeup, listeners, *makeup_knob, pluginState.undoManager);
        makeup_knob->addAttachment(makeup_knob_attachment.get());

        // mix_knob = std::make_unique<SynthSlider>(params.mix->paramID, params.mix->getModParam());
        // addSlider(mix_knob.get());
        // mix_knob->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        // mix_knob->setPopupPlacement(juce::BubbleComponent::below);
        // mix_knob->setShowPopupOnHover(true);
        // mix_knob_attachment = std::make_unique<chowdsp::SliderAttachment>(params.mix, listeners, *mix_knob, nullptr);
        // mix_knob->addAttachment(mix_knob_attachment.get());

        ratio_knob = std::make_unique<SynthSlider>(params.ratio->paramID, params.ratio->getModParam());
        addSlider(ratio_knob.get());
        ratio_knob->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        ratio_knob->setPopupPlacement(juce::BubbleComponent::below);
        ratio_knob->setShowPopupOnHover(true);
        ratio_knob_attachment = std::make_unique<chowdsp::SliderAttachment>(params.ratio, listeners, *ratio_knob, pluginState.undoManager);
        ratio_knob->addAttachment(ratio_knob_attachment.get());

        knee_knob = std::make_unique<SynthSlider>(params.knee->paramID, params.knee->getModParam());
        addSlider(knee_knob.get());
        knee_knob->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        knee_knob->setPopupPlacement(juce::BubbleComponent::below);
        knee_knob->setShowPopupOnHover(true);
        knee_knob_attachment = std::make_unique<chowdsp::SliderAttachment>(params.knee, listeners, *knee_knob, pluginState.undoManager);
        knee_knob->addAttachment(knee_knob_attachment.get());

        // knob labels
        attack_knob_label = std::make_shared<PlainTextComponent>(attack_knob->getName(), "ATTACK");
        addOpenGlComponent(attack_knob_label);
        attack_knob_label->setJustification(juce::Justification::centred);

        release_knob_label = std::make_shared<PlainTextComponent>(release_knob->getName(), "RELEASE");
        addOpenGlComponent(release_knob_label);
        release_knob_label->setJustification(juce::Justification::centred);

        threshold_knob_label = std::make_shared<PlainTextComponent>(threshold_knob->getName(), "THRESHOLD");
        addOpenGlComponent(threshold_knob_label);
        threshold_knob_label->setJustification(juce::Justification::centred);

        makeup_knob_label = std::make_shared<PlainTextComponent>(makeup_knob->getName(), "MAKEUP");
        addOpenGlComponent(makeup_knob_label);
        makeup_knob_label->setJustification(juce::Justification::centred);

        // mix_knob_label = std::make_shared<PlainTextComponent>(mix_knob->getName(), "MIX");
        // addOpenGlComponent(mix_knob_label);
        // mix_knob_label->setJustification(juce::Justification::centred);

        ratio_knob_label = std::make_shared<PlainTextComponent>(ratio_knob->getName(), "RATIO");
        addOpenGlComponent(ratio_knob_label);
        ratio_knob_label->setJustification(juce::Justification::centred);

        knee_knob_label = std::make_shared<PlainTextComponent>(knee_knob->getName(), "KNEE");
        addOpenGlComponent(knee_knob_label);
        knee_knob_label->setJustification(juce::Justification::centred);

        compressorControlsBorder = std::make_shared<OpenGL_LabeledBorder>("compressor controls border", "Compressor Parameters");
        addBorder(compressorControlsBorder.get());
        presetsBorder = std::make_shared<OpenGL_LabeledBorder>("presets border", "Power and Presets");
        addBorder(presetsBorder.get());

        presetsButton = std::make_unique<OpenGlTextButton> ("compressorPresets");
        addOpenGlComponent (presetsButton->getGlComponent());
        addAndMakeVisible (presetsButton.get());
        presetsButton->addListener (this);
        presetsButton->setLookAndFeel (DefaultLookAndFeel::instance());

        powerCallbacks_ += {listeners.addParameterListener(
            compressorParams_.activeCompressor,
            chowdsp::ParameterListenerThread::MessageThread,
            [this]() { updateKnobsEnabled(compressorParams_.activeCompressor->get()); })};
        updateKnobsEnabled(compressorParams_.activeCompressor->get());

        powerCallbacks_ += {listeners.addParameterListener(
            compressorParams_.presets,
            chowdsp::ParameterListenerThread::MessageThread,
            [this]() { updatePresetButtonText(); })};
        updatePresetButtonText();

        // for updating the compressorMeter
        startTimer(50);
    }

    void buttonClicked (juce::Button* b) override
    {
        if (b == presetsButton.get())
        {
            juce::PopupMenu menu;
            for (int i = 0; i < compressorParams_.presets->choices.size(); ++i)
                menu.addItem (i + 1, compressorParams_.presets->choices[i]);
            menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (presetsButton.get()),
                [this] (int result)
                {
                    static const CompressorPresetComboBox vals[] = {
                        Default, Piano, Piano_2, Brick_Wall, Aggressive, CompressorCustom };
                    if (result >= 1 && result <= 6)
                        compressorParams_.presets->setParameterValue (vals[result - 1]);
                });
            return;
        }
        SynthSection::buttonClicked (b);
    }

    void updatePresetButtonText()
    {
        presetsButton->setText (compressorParams_.presets->choices[compressorParams_.presets->getIndex()]);
    }

    void updateKnobsEnabled(bool active)
    {
        for (auto* knob : { attack_knob.get(), release_knob.get(), threshold_knob.get(),
                            makeup_knob.get(), ratio_knob.get(), knee_knob.get() })
        {
            knob->setEnabled(active);
            knob->setActive(active);
        }
    }

    void timerCallback() override;
    void stopAllTimers() override {
        stopTimer();
    }

    void paintBackground (juce::Graphics& g) override
    {
        setLabelFont(g);
        SynthSection::paintContainer (g);
        paintBorder (g);
        paintKnobShadows (g);
        paintChildrenBackgrounds (g);

        /*
         * update needle position here
         * - not getting paint() calls inside the compressorMeter, so doing it out here
         */
        const auto bounds = compressorMeter->getBounds().toFloat();
        const float centreX = bounds.getX() + bounds.getWidth() * 0.5f;
        const float centreY = bounds.getY() + bounds.getHeight();
        const float needleLength = juce::jmin(bounds.getWidth() * 0.7f, bounds.getHeight() * 0.7f);
        g.setColour(compressorMeter->needle.needleColour);
        compressorMeter->needle.redrawNeedle (g, centreX, centreY, needleLength);
    }
    
    // prep title, vertical, left side
    std::shared_ptr<PlainTextComponent> prepTitle;

    // compressor meter
    std::shared_ptr<OpenGL_CompressorMeter> compressorMeter;

    std::unique_ptr<OpenGlTextButton> presetsButton;

    // active compressor button
    std::unique_ptr<SynthButton> activeCompressor_toggle;
    std::unique_ptr<chowdsp::ButtonAttachment> activeCompressor_attachment;

    // level meters
    std::shared_ptr<PeakMeterSection> levelMeter;
    std::shared_ptr<PeakMeterSection> sendLevelMeter;
    std::shared_ptr<PeakMeterSection> inLevelMeter;
    std::shared_ptr<PeakMeterSection> externalLevelMeter;

    std::unique_ptr<SynthButton> muteButton_;
    std::unique_ptr<SynthButton> soloButton_;

    SynthBase* synth_ = nullptr;
    juce::AudioProcessorGraph::NodeID nodeId_;

    bool isPrepVersion_ = false;

    // knobs
    std::unique_ptr<SynthSlider> attack_knob;
    std::unique_ptr<chowdsp::SliderAttachment> attack_knob_attachment;
    std::unique_ptr<SynthSlider> release_knob;
    std::unique_ptr<chowdsp::SliderAttachment> release_knob_attachment;
    std ::unique_ptr<SynthSlider> threshold_knob;
    std::unique_ptr<chowdsp::SliderAttachment> threshold_knob_attachment;
    std ::unique_ptr<SynthSlider> makeup_knob;
    std::unique_ptr<chowdsp::SliderAttachment> makeup_knob_attachment;
    // std::unique_ptr<SynthSlider> mix_knob;
    // std::unique_ptr<chowdsp::SliderAttachment> mix_knob_attachment;
    std ::unique_ptr<SynthSlider> ratio_knob;
    std::unique_ptr<chowdsp::SliderAttachment> ratio_knob_attachment;
    std ::unique_ptr<SynthSlider> knee_knob;
    std::unique_ptr<chowdsp::SliderAttachment> knee_knob_attachment;

    // knob labels
    std::shared_ptr<PlainTextComponent> attack_knob_label;
    std::shared_ptr<PlainTextComponent> release_knob_label;
    std::shared_ptr<PlainTextComponent> threshold_knob_label;
    std::shared_ptr<PlainTextComponent> makeup_knob_label;
    std::shared_ptr<PlainTextComponent> mix_knob_label;
    std::shared_ptr<PlainTextComponent> ratio_knob_label;
    std::shared_ptr<PlainTextComponent> knee_knob_label;

    // borders
    std::shared_ptr<OpenGL_LabeledBorder> compressorControlsBorder;
    std::shared_ptr<OpenGL_LabeledBorder> presetsBorder;

    chowdsp::ScopedCallbackList powerCallbacks_;
    CompressorParams& compressorParams_;
    void resized() override;
};

#endif //BITKLAVIER2_CompressorPARAMETERVIEW_H