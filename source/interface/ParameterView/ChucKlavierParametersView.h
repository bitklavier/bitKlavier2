// Copyright (C) 2022-2025 Dan Trueman
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "ChucKlavierProcessor.h"
#include "FullInterface.h"
#include "Identifiers.h"
#include "peak_meter_section.h"
#include "synth_button.h"
#include "synth_section.h"
#include "synth_slider.h"
#include "open_gl_image_component.h"

class ChucKlavierParametersView : public SynthSection, public juce::Timer
{
public:
    ChucKlavierParametersView (chowdsp::PluginState& pluginState,
                               ChucKlavierParams& params,
                               juce::String uuid,
                               const juce::ValueTree& prepVT,
                               OpenGlWrapper* open_gl,
                               SynthBase* synth = nullptr,
                               juce::AudioProcessorGraph::NodeID nodeId = {})
        : SynthSection (""),
          cparams_ (params),
          prepVT_ (prepVT),
          synth_ (synth),
          nodeId_ (nodeId)
    {
        setName ("chucklavier");
        setLookAndFeel (DefaultLookAndFeel::instance());
        setComponentID (uuid);
        setSkinOverride (Skin::kDirect);

        prepTitle = std::make_shared<PlainTextComponent> (getName(), getName());
        addOpenGlComponent (prepTitle);
        prepTitle->setJustification (juce::Justification::centredLeft);
        prepTitle->setFontType (PlainTextComponent::kTitle);
        prepTitle->setRotation (-90);

        FullInterface* parent = findParentComponentOfClass<FullInterface>();
        if (parent)
            parent->hideSoundsetSelector();

        auto& listeners = pluginState.getParameterListeners();

        levelMeter = std::make_unique<PeakMeterSection> (uuid, params.outputGain, listeners, &params.outputLevels);
        levelMeter->setLabel ("Main");
        levelMeter->setVolumeTooltip ("Overall output volume of ChucKlavier");
        addSubSection (levelMeter.get());

        sendLevelMeter = std::make_unique<PeakMeterSection> (uuid, params.outputSend, listeners, &params.sendLevels);
        sendLevelMeter->setLabel ("Send");
        sendLevelMeter->setVolumeTooltip ("Volume of ChucKlavier output to connected effects");
        addSubSection (sendLevelMeter.get());

        inLevelMeter = std::make_unique<PeakMeterSection> (uuid, params.inputGain, listeners, &params.inputLevels);
        inLevelMeter->setLabel ("Internal");
        inLevelMeter->setVolumeTooltip ("Level of audio bussed in from other preparations");
        addSubSection (inLevelMeter.get());

        externalLevelMeter = std::make_unique<PeakMeterSection> (uuid, params.externalGain, listeners, &params.externalLevels);
        externalLevelMeter->setLabel ("External");
        externalLevelMeter->setVolumeTooltip ("Level of external input (mic/line in standalone, sidechain in plugin)");
        addSubSection (externalLevelMeter.get());

        // Script text editor — must add to hierarchy BEFORE setMultiLine,
        // since setMultiLine triggers resized() which calls findValue() on parent_.
        scriptEditor = std::make_unique<OpenGlTextEditor> ("chuckScript");
        addAndMakeVisible (scriptEditor.get());
        addOpenGlComponent (scriptEditor->getImageComponent());

        scriptEditor->setMultiLine (true, true);
        scriptEditor->setReturnKeyStartsNewLine (true);
        scriptEditor->setScrollbarsShown (true);
        scriptEditor->setPopupMenuEnabled (true);
        scriptEditor->setFont (juce::Font (juce::Font::getDefaultMonospacedFontName(), 13.0f, juce::Font::plain));
        scriptEditor->setColour (juce::TextEditor::backgroundColourId, juce::Colours::black.withAlpha (0.85f));
        scriptEditor->setColour (juce::TextEditor::textColourId, juce::Colours::lightgreen);
        scriptEditor->setColour (juce::TextEditor::outlineColourId, juce::Colours::darkgrey);
        scriptEditor->setColour (juce::TextEditor::highlightColourId, juce::Colours::darkgreen.withAlpha (0.6f));

        juce::String savedScript = prepVT_.getProperty (IDs::chuckScript, "");
        if (savedScript.isEmpty())
            savedScript = getDefaultScript();
        scriptEditor->setText (savedScript, juce::dontSendNotification);

        scriptEditor->onTextChange = [this] {
            prepVT_.setProperty (IDs::chuckScript, scriptEditor->getText(), nullptr);
        };

        // Send to VM button (stub — will wire to VM in Stage 2)
        sendScriptButton = std::make_unique<SynthButton> ("sendScript");
        sendScriptButton->setText ("Send to VM");
        sendScriptButton->setTooltip ("Compile and hot-swap the script into the ChucK VM (Stage 2)");
        addSynthButton (sendScriptButton.get(), true);
        sendScriptButton->onClick = [] { /* Stage 2: compile + swap */ };

        muteButton_ = std::make_unique<SynthButton> ("mute");
        muteButton_->setText ("M");
        muteButton_->setTooltip ("Mute this preparation. Option-click to mute only this one.");
        addSynthButton (muteButton_.get(), true);
        muteButton_->onClick = [this] () {
            bool isOptionClick = juce::ModifierKeys::currentModifiers.isAltDown();
            bool newMuted = !cparams_.userMuted_.load (std::memory_order_relaxed);
            cparams_.userMuted_.store (newMuted, std::memory_order_relaxed);
            cparams_.muted_.store (newMuted || cparams_.soloMuted_.load (std::memory_order_relaxed),
                                   std::memory_order_relaxed);
            if (synth_) synth_->coordinateMuteChanged (nodeId_, newMuted, isOptionClick);
        };

        soloButton_ = std::make_unique<SynthButton> ("solo");
        soloButton_->setText ("S");
        soloButton_->setTooltip ("Solo this preparation. Option-click to solo only this one.");
        addSynthButton (soloButton_.get(), true);
        soloButton_->onClick = [this] () {
            bool isOptionClick = juce::ModifierKeys::currentModifiers.isAltDown();
            bool newSoloed = !cparams_.soloed_.load (std::memory_order_relaxed);
            cparams_.soloed_.store (newSoloed, std::memory_order_relaxed);
            if (synth_) synth_->coordinateSoloChanged (nodeId_, isOptionClick);
        };

        startTimer (50);
    }

    void timerCallback() override
    {
        bool soloed    = cparams_.soloed_.load (std::memory_order_relaxed);
        bool userMuted = cparams_.userMuted_.load (std::memory_order_relaxed);
        bool soloMuted = cparams_.soloMuted_.load (std::memory_order_relaxed);
        soloButton_->setToggleState (soloed, juce::dontSendNotification);
        if (soloMuted && !userMuted)
        {
            bool blinkPhase = (juce::Time::getMillisecondCounter() / 300) % 2;
            muteButton_->setToggleState (blinkPhase, juce::dontSendNotification);
        }
        else
        {
            muteButton_->setToggleState (userMuted, juce::dontSendNotification);
        }
    }

    void stopAllTimers() override { stopTimer(); }

    void paintBackground (juce::Graphics& g) override
    {
        setLabelFont (g);
        SynthSection::paintContainer (g);
        paintBorder (g);
        paintKnobShadows (g);
        paintChildrenBackgrounds (g);
    }

    void resized() override;

    std::shared_ptr<PlainTextComponent> prepTitle;

    std::unique_ptr<PeakMeterSection> levelMeter;
    std::unique_ptr<PeakMeterSection> sendLevelMeter;
    std::unique_ptr<PeakMeterSection> inLevelMeter;
    std::unique_ptr<PeakMeterSection> externalLevelMeter;

    std::unique_ptr<OpenGlTextEditor> scriptEditor;
    std::unique_ptr<SynthButton>      sendScriptButton;
    std::unique_ptr<SynthButton>      muteButton_;
    std::unique_ptr<SynthButton>      soloButton_;

private:
    static juce::String getDefaultScript()
    {
        return "// ChucKlavier default: passthrough\nadc => dac;\nwhile (true) { 1::samp => now; }\n";
    }

    ChucKlavierParams& cparams_;
    juce::ValueTree    prepVT_;
    SynthBase*         synth_   = nullptr;
    juce::AudioProcessorGraph::NodeID nodeId_;
};
