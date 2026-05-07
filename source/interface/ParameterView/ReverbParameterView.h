// Copyright (C) 2022-2025 Dan Trueman
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once
#include "ReverbProcessor.h"
#include "OpenGL_LabeledBorder.h"
#include "default_look_and_feel.h"
#include "peak_meter_section.h"
#include "synth_section.h"
#include "synth_slider.h"

class ReverbParameterView : public SynthSection, public juce::Timer, public juce::ValueTree::Listener
{
public:
    ReverbParameterView (chowdsp::PluginState& pluginState, ReverbParams& params,
                         juce::String name, OpenGlWrapper* open_gl,
                         ReverbProcessor* proc, bool isPrepVersion = false)
        : SynthSection (""), reverbParams_ (params), proc_ (proc), isPrepVersion_ (isPrepVersion)
    {
        setName ("reverb");
        setLookAndFeel (DefaultLookAndFeel::instance());
        setComponentID (name);
        setSkinOverride (Skin::kDirect);

        auto& listeners = pluginState.getParameterListeners();

        // Title
        prepTitle = std::make_shared<PlainTextComponent> (getName(), getName());
        addOpenGlComponent (prepTitle);
        prepTitle->setJustification (juce::Justification::centredLeft);
        prepTitle->setFontType (PlainTextComponent::kTitle);
        prepTitle->setRotation (-90);

        // Power button
        activeReverb_toggle = std::make_unique<SynthButton> (params.activeReverb->paramID);
        activeReverb_attachment = std::make_unique<chowdsp::ButtonAttachment> (
            params.activeReverb, listeners, *activeReverb_toggle, pluginState.undoManager);
        activeReverb_toggle->setComponentID (params.activeReverb->paramID);
        addSynthButton (activeReverb_toggle.get(), true);
        activeReverb_toggle->setText ("power");


        // Presets button (opens PopupMenu with bank submenus)
        presetsButton = std::make_unique<OpenGlTextButton> ("reverbPresets");
        addOpenGlComponent (presetsButton->getGlComponent());
        addAndMakeVisible (presetsButton.get());
        presetsButton->addListener (this);
        presetsButton->setLookAndFeel (DefaultLookAndFeel::instance());
        updatePresetButtonText();

        // Input/output meters
        inLevelMeter = std::make_unique<PeakMeterSection> (name, params.inputGain, listeners, &params.inputLevels);
        inLevelMeter->setLabel (isPrepVersion_ ? "Internal" : "In");
        addSubSection (inLevelMeter.get());

        levelMeter = std::make_unique<PeakMeterSection> (name, params.outputGain, listeners, &params.outputLevels);
        levelMeter->setLabel (isPrepVersion_ ? "Main" : "Out");
        addSubSection (levelMeter.get());

        if (isPrepVersion_)
        {
            externalLevelMeter = std::make_unique<PeakMeterSection> (name, params.externalGain, listeners, &params.externalLevels);
            externalLevelMeter->setLabel ("External");
            addSubSection (externalLevelMeter.get());

            sendLevelMeter = std::make_unique<PeakMeterSection> (name, params.outputSend, listeners, &params.sendLevels);
            sendLevelMeter->setLabel ("Send");
            addSubSection (sendLevelMeter.get());
        }

        // Borders
        presetsBorder  = std::make_shared<OpenGL_LabeledBorder> ("presets border",   "Power and Presets");
        group1Border   = std::make_shared<OpenGL_LabeledBorder> ("group1 border",    "");
        group2Border   = std::make_shared<OpenGL_LabeledBorder> ("group2 border",    "");
        group3Border   = std::make_shared<OpenGL_LabeledBorder> ("group3 border",    "");
        group4Border   = std::make_shared<OpenGL_LabeledBorder> ("group4 border",    "");
        group5Border   = std::make_shared<OpenGL_LabeledBorder> ("group5 border",    "");
        addBorder (presetsBorder.get());
        addBorder (group1Border.get());
        addBorder (group2Border.get());
        addBorder (group3Border.get());
        addBorder (group4Border.get());
        addBorder (group5Border.get());

        // Helper to create a knob + label + attachment
        auto makeKnob = [&] (chowdsp::FloatParameter::Ptr& param,
                              std::unique_ptr<SynthSlider>& knob,
                              std::unique_ptr<chowdsp::SliderAttachment>& attachment,
                              std::shared_ptr<PlainTextComponent>& label,
                              const juce::String& labelText)
        {
            knob = std::make_unique<SynthSlider> (param->paramID, param->getModParam());
            addSlider (knob.get());
            knob->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
            knob->setPopupPlacement (juce::BubbleComponent::below);
            knob->setShowPopupOnHover (true);
            attachment = std::make_unique<chowdsp::SliderAttachment> (param, listeners, *knob, pluginState.undoManager);
            knob->addAttachment (attachment.get());

            label = std::make_shared<PlainTextComponent> (knob->getName(), labelText);
            addOpenGlComponent (label);
            label->setJustification (juce::Justification::centred);
        };

        // Group 1: Dry Level, Early Level, Early Send, Late Level
        makeKnob (params.dryLevel,   dryLevel_knob,   dryLevel_att,   dryLevel_label,   "DRY");
        makeKnob (params.earlyLevel, earlyLevel_knob, earlyLevel_att, earlyLevel_label, "EARLY");
        makeKnob (params.earlySend,  earlySend_knob,  earlySend_att,  earlySend_label,  "SEND");
        makeKnob (params.lateLevel,  lateLevel_knob,  lateLevel_att,  lateLevel_label,  "LATE");

        // Group 2: Diffuse, Modulation, Spin, Wander
        makeKnob (params.diffuse,    diffuse_knob,    diffuse_att,    diffuse_label,    "DIFFUSE");
        makeKnob (params.modulation, modulation_knob, modulation_att, modulation_label, "MOD");
        makeKnob (params.spin,       spin_knob,       spin_att,       spin_label,       "SPIN");
        makeKnob (params.wander,     wander_knob,     wander_att,     wander_label,     "WANDER");

        // Group 3: Size, Width, Predelay, Decay
        makeKnob (params.size,       size_knob,       size_att,       size_label,       "SIZE");
        makeKnob (params.width,      width_knob,      width_att,      width_label,      "WIDTH");
        makeKnob (params.predelay,   predelay_knob,   predelay_att,   predelay_label,   "PREDELAY");
        makeKnob (params.decay,      decay_knob,      decay_att,      decay_label,      "DECAY");

        // Group 4: High Cut, High Cross, High Mult
        makeKnob (params.highCut,    highCut_knob,    highCut_att,    highCut_label,    "HI CUT");
        makeKnob (params.highXover,  highXover_knob,  highXover_att,  highXover_label,  "HI CROSS");
        makeKnob (params.highMult,   highMult_knob,   highMult_att,   highMult_label,   "HI MULT");

        // Group 5: Low Cut, Low Cross, Low Mult
        makeKnob (params.lowCut,     lowCut_knob,     lowCut_att,     lowCut_label,     "LO CUT");
        makeKnob (params.lowXover,   lowXover_knob,   lowXover_att,   lowXover_label,   "LO CROSS");
        makeKnob (params.lowMult,    lowMult_knob,    lowMult_att,    lowMult_label,    "LO MULT");

        // Enable/disable knobs based on power state
        powerCallbacks_ += { listeners.addParameterListener (
            params.activeReverb,
            chowdsp::ParameterListenerThread::MessageThread,
            [this]() { updateKnobsEnabled (reverbParams_.activeReverb->get()); }) };
        updateKnobsEnabled (params.activeReverb->get());

        // Update preset button when preset index changes in the VT
        // (handled via updatePresetButtonText() called from applyPreset and setCustom)

        if (proc_ != nullptr)
            proc_->v.addListener (this);

        startTimer (50);
    }

    ~ReverbParameterView() override
    {
        if (proc_ != nullptr)
            proc_->v.removeListener (this);
    }

    void timerCallback() override {}
    void stopAllTimers() override { stopTimer(); }

    void valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier& id) override
    {
        if (id == IDs::reverbPreset)
            updatePresetButtonText();
    }

    void updateKnobsEnabled (bool active)
    {
        for (auto* k : getAllKnobs())
        {
            k->setEnabled (active);
            k->setActive (active);
        }
    }

    void updatePresetButtonText()
    {
        if (proc_ == nullptr) return;
        int idx = proc_->getPresetIndex();
        if (idx < 1 || idx > 25)
            presetsButton->setText ("Custom");
        else
            presetsButton->setText (kReverbPresets[idx - 1].presetName);
    }

    void buttonClicked (juce::Button* b) override
    {
        if (b == presetsButton.get())
        {
            juce::PopupMenu menu;
            // Build 5 bank submenus — item IDs are 1-25
            const char* banks[5] = { "Rooms", "Studios", "Small Halls", "Medium Halls", "Large Halls" };
            for (int bank = 0; bank < 5; ++bank)
            {
                juce::PopupMenu sub;
                for (int preset = 0; preset < 5; ++preset)
                {
                    int idx = bank * 5 + preset + 1;
                    sub.addItem (idx, kReverbPresets[idx - 1].presetName);
                }
                menu.addSubMenu (banks[bank], sub);
            }
            menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (presetsButton.get()),
                [this] (int result)
                {
                    if (result >= 1 && result <= 25 && proc_ != nullptr)
                    {
                        proc_->applyPreset (result);
                        updatePresetButtonText();
                    }
                });
            return;
        }
        SynthSection::buttonClicked (b);
    }

    void paintBackground (juce::Graphics& g) override
    {
        setLabelFont (g);
        SynthSection::paintContainer (g);
        paintBorder (g);
        paintKnobShadows (g);
        paintChildrenBackgrounds (g);
    }

    void resized() override;

    // ── Members ──────────────────────────────────────────────────────
    std::shared_ptr<PlainTextComponent> prepTitle;

    std::unique_ptr<SynthButton> activeReverb_toggle;
    std::unique_ptr<chowdsp::ButtonAttachment> activeReverb_attachment;
    std::unique_ptr<OpenGlTextButton> presetsButton;

    std::unique_ptr<PeakMeterSection> inLevelMeter;
    std::unique_ptr<PeakMeterSection> levelMeter;
    std::unique_ptr<PeakMeterSection> externalLevelMeter;
    std::unique_ptr<PeakMeterSection> sendLevelMeter;

    bool isPrepVersion_ = false;

    std::shared_ptr<OpenGL_LabeledBorder> presetsBorder;
    std::shared_ptr<OpenGL_LabeledBorder> group1Border, group2Border, group3Border, group4Border, group5Border;

    // Group 1
    std::unique_ptr<SynthSlider> dryLevel_knob, earlyLevel_knob, earlySend_knob, lateLevel_knob;
    std::unique_ptr<chowdsp::SliderAttachment> dryLevel_att, earlyLevel_att, earlySend_att, lateLevel_att;
    std::shared_ptr<PlainTextComponent> dryLevel_label, earlyLevel_label, earlySend_label, lateLevel_label;

    // Group 2
    std::unique_ptr<SynthSlider> diffuse_knob, modulation_knob, spin_knob, wander_knob;
    std::unique_ptr<chowdsp::SliderAttachment> diffuse_att, modulation_att, spin_att, wander_att;
    std::shared_ptr<PlainTextComponent> diffuse_label, modulation_label, spin_label, wander_label;

    // Group 3
    std::unique_ptr<SynthSlider> size_knob, width_knob, predelay_knob, decay_knob;
    std::unique_ptr<chowdsp::SliderAttachment> size_att, width_att, predelay_att, decay_att;
    std::shared_ptr<PlainTextComponent> size_label, width_label, predelay_label, decay_label;

    // Group 4
    std::unique_ptr<SynthSlider> highCut_knob, highXover_knob, highMult_knob;
    std::unique_ptr<chowdsp::SliderAttachment> highCut_att, highXover_att, highMult_att;
    std::shared_ptr<PlainTextComponent> highCut_label, highXover_label, highMult_label;

    // Group 5
    std::unique_ptr<SynthSlider> lowCut_knob, lowXover_knob, lowMult_knob;
    std::unique_ptr<chowdsp::SliderAttachment> lowCut_att, lowXover_att, lowMult_att;
    std::shared_ptr<PlainTextComponent> lowCut_label, lowXover_label, lowMult_label;

    chowdsp::ScopedCallbackList powerCallbacks_;
    ReverbParams& reverbParams_;
    ReverbProcessor* proc_ = nullptr;

private:
    std::vector<SynthSlider*> getAllKnobs()
    {
        return { dryLevel_knob.get(), earlyLevel_knob.get(), earlySend_knob.get(), lateLevel_knob.get(),
                 diffuse_knob.get(), modulation_knob.get(), spin_knob.get(), wander_knob.get(),
                 size_knob.get(), width_knob.get(), predelay_knob.get(), decay_knob.get(),
                 highCut_knob.get(), highXover_knob.get(), highMult_knob.get(),
                 lowCut_knob.get(), lowXover_knob.get(), lowMult_knob.get() };
    }
};
