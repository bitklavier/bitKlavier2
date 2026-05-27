// Copyright (C) 2022-2025 Dan Trueman
// SPDX-License-Identifier: GPL-3.0-or-later

//
// Created by Myra Norton on 11/14/25.
//

#ifndef BITKLAVIER2_EQPARAMETERVIEW_H
#define BITKLAVIER2_EQPARAMETERVIEW_H
#include "EQProcessor.h"
#include "EQFilterSection.h"
#include "OpenGL_LabeledBorder.h"
#include "synth_section.h"
#include "peak_meter_section.h"
#include "synth_slider.h"
#include "default_look_and_feel.h"
#include "OpenGL_EqualizerGraph.h"

class EQFilterSection;
class EQParameterView : public SynthSection, private juce::Timer
{
public:
    ~EQParameterView() { stopTimer(); }

    void stopAllTimers() override { stopTimer(); }

    EQParameterView (chowdsp::PluginState& pluginState, EQParams& params, juce::String name, OpenGlWrapper* open_gl, bool isPrepVersion = false) : SynthSection (""), eqparams_ (params), isPrepVersion_ (isPrepVersion)
    {
        // the name that will appear in the UI as the name of the section
        setName ("eq");
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

        // bypass EQ button
        activeEq_toggle = std::make_unique<SynthButton>(eqparams_.activeEq->paramID);
        activeEq_attachment = std::make_unique<chowdsp::ButtonAttachment>(eqparams_.activeEq, listeners, *activeEq_toggle, pluginState.undoManager);
        activeEq_toggle->setComponentID(eqparams_.activeEq->paramID);
        addSynthButton(activeEq_toggle.get(), true);
        activeEq_toggle->setText("power");


        // EQ Graph
        equalizerGraph = std::make_unique<OpenGL_EqualizerGraph> (&params, listeners);
        addOpenGlComponent (equalizerGraph->getImageComponent());
        addAndMakeVisible(equalizerGraph.get());

        // eq filter sections
        loCutSection = std::make_unique<EQCutFilterSection>(name, eqparams_.loCutFilterParams, listeners, *this, pluginState.undoManager);
        addSubSection (loCutSection.get());
        peak1Section = std::make_unique<EQPeakFilterSection>(name, eqparams_.peak1FilterParams, listeners, *this, pluginState.undoManager);
        addSubSection(peak1Section.get());
        peak2Section = std::make_unique<EQPeakFilterSection>(name, eqparams_.peak2FilterParams, listeners, *this, pluginState.undoManager);
        addSubSection(peak2Section.get());
        peak3Section = std::make_unique<EQPeakFilterSection>(name, eqparams_.peak3FilterParams, listeners, *this, pluginState.undoManager);
        addSubSection(peak3Section.get());
        hiCutSection = std::make_unique<EQCutFilterSection>(name, eqparams_.hiCutFilterParams, listeners, *this, pluginState.undoManager);
        addSubSection(hiCutSection.get());

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
            muteButton_->setText("mute");
            addSynthButton(muteButton_.get(), true);
            muteButton_->onClick = [this]() {
                eqparams_.muted_.store(muteButton_->getToggleState(), std::memory_order_relaxed);
            };
        }

        presetsButton = std::make_unique<OpenGlTextButton> ("eqPresets");
        addOpenGlComponent (presetsButton->getGlComponent());
        addAndMakeVisible (presetsButton.get());
        presetsButton->addListener (this);
        presetsButton->setLookAndFeel (DefaultLookAndFeel::instance());

        presetsBorder = std::make_shared<OpenGL_LabeledBorder>("presets border", "Power and Presets");
        addBorder(presetsBorder.get());

        powerCallbacks_ += {listeners.addParameterListener(
            eqparams_.activeEq,
            chowdsp::ParameterListenerThread::MessageThread,
            [this]() { updateControlsEnabled(eqparams_.activeEq->get()); })};
        updateControlsEnabled(eqparams_.activeEq->get());

        powerCallbacks_ += {listeners.addParameterListener(
            eqparams_.presets,
            chowdsp::ParameterListenerThread::MessageThread,
            [this]() { updatePresetButtonText(); })};
        updatePresetButtonText();

        // redraw eq graph
        eqparams_.doForAllParameters ([this, &listeners] (auto& param, size_t) {
            eqRedoImageCallbacks += {listeners.addParameterListener(
                param,
                chowdsp::ParameterListenerThread::MessageThread,
                [this]() {
                    this->equalizerGraph->redoImage();
                })
            };
        });

        // Poll at ~30Hz to update the graph when continuous modulation is active
        // (applyMonophonicModulation doesn't fire parameterValueChanged, so listeners
        // won't catch modulation-driven changes)
        startTimerHz (30);
    }

    void timerCallback() override
    {
        equalizerGraph->redoImage();
    }

    void buttonClicked (juce::Button* b) override
    {
        if (b == presetsButton.get())
        {
            juce::PopupMenu menu;
            for (int i = 0; i < eqparams_.presets->choices.size(); ++i)
                menu.addItem (i + 1, eqparams_.presets->choices[i]);
            menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (presetsButton.get()),
                [this] (int result)
                {
                    static const EqPresetComboBox vals[] = { EqOff, Highshelf, Lowshelf, EqCustom };
                    if (result >= 1 && result <= 4)
                        eqparams_.presets->setParameterValue (vals[result - 1]);
                });
            return;
        }
        SynthSection::buttonClicked (b);
    }

    void updatePresetButtonText()
    {
        presetsButton->setText (eqparams_.presets->choices[eqparams_.presets->getIndex()]);
    }

    void updateControlsEnabled(bool active)
    {
        loCutSection->setControlsEnabled(active);
        peak1Section->setControlsEnabled(active);
        peak2Section->setControlsEnabled(active);
        peak3Section->setControlsEnabled(active);
        hiCutSection->setControlsEnabled(active);
    }

    void paintBackground (juce::Graphics& g) override
    {
        setLabelFont(g);
        SynthSection::paintContainer (g);
        paintBorder (g);
        paintKnobShadows (g);
        paintChildrenBackgrounds (g);
    }

    // prep title, vertical, left side
    std::shared_ptr<PlainTextComponent> prepTitle;

    chowdsp::ScopedCallbackList eqRedoImageCallbacks;
    chowdsp::ScopedCallbackList powerCallbacks_;

    // active eq & reset buttons
    std::unique_ptr<SynthButton> activeEq_toggle;
    std::unique_ptr<chowdsp::ButtonAttachment> activeEq_attachment;

    std::unique_ptr<OpenGlTextButton> presetsButton;

    // equalizer graph
    std::shared_ptr<OpenGL_EqualizerGraph> equalizerGraph;

    // EQ Filter Sections
    std::unique_ptr<EQCutFilterSection> loCutSection;
    std::unique_ptr<EQPeakFilterSection> peak1Section;
    std::unique_ptr<EQPeakFilterSection> peak2Section;
    std::unique_ptr<EQPeakFilterSection> peak3Section;
    std::unique_ptr<EQCutFilterSection> hiCutSection;

    // level meters
    std::shared_ptr<PeakMeterSection> levelMeter;
    std::shared_ptr<PeakMeterSection> sendLevelMeter;
    std::shared_ptr<PeakMeterSection> inLevelMeter;
    std::shared_ptr<PeakMeterSection> externalLevelMeter;

    std::unique_ptr<SynthButton> muteButton_;

    bool isPrepVersion_ = false;

    std::shared_ptr<OpenGL_LabeledBorder> presetsBorder;

    EQParams& eqparams_;
    void resized() override;
};

#endif //BITKLAVIER2_EQPARAMETERVIEW_H