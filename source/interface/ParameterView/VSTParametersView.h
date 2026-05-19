// Copyright (C) 2022-2025 Dan Trueman
// SPDX-License-Identifier: GPL-3.0-or-later

//
// VSTParametersView.h
//
// A scrollable SynthSection displaying one SynthSlider per automatable VST
// parameter. Each slider's componentID matches the modulation destination_name
// ("<bridgeUuid>_<slotIdx>") so that the existing modulation drag-and-drop
// workflow works identically to built-in preparations.
//
// Opened from ModulationPreparation::mouseDoubleClick when the modulation prep
// is connected to a VST, alongside the native PluginWindow.

#pragma once

#include "../sections/synth_section.h"
#include "../components/synth_slider.h"
#include "../components/opengl/open_gl_image_component.h"
#include "../../synthesis/framework/Processors/VSTModulationBridge.h"
#include "../../common/Identifiers.h"

// ---------------------------------------------------------------------------
// VSTParamAttachment
//
// Bidirectional link between a juce::AudioProcessorParameter and a SynthSlider:
//   - Param changes (from the VST itself) → updates the slider on the message thread
//   - Slider changes (user drag) → calls setValueNotifyingHost + updates bridge base value
// ---------------------------------------------------------------------------
class VSTParamAttachment final : private juce::AudioProcessorParameter::Listener,
                                  private juce::Slider::Listener
{
public:
    VSTParamAttachment (juce::AudioProcessorParameter& param,
                        SynthSlider& slider,
                        VSTModulationBridge& bridge,
                        int slot)
        : param_ (param), slider_ (slider), bridge_ (bridge), slot_ (slot)
    {
        slider_.setValue (param_.getValue(), juce::dontSendNotification);
        param_.addListener (this);
        slider_.addListener (this);
    }

    ~VSTParamAttachment() override
    {
        slider_.removeListener (this);
        param_.removeListener (this);
    }

private:
    void parameterValueChanged (int, float newValue) override
    {
        // Called when the parameter changes from any source OTHER than the bridge's
        // own param->setValue() (which does not fire listeners).  This includes:
        //   - the native plugin UI (user drags a knob in the plugin window)
        //   - our own sliderValueChanged → setValueNotifyingHost (same value back)
        // Update the knob position and bridge base so both stay in sync with the
        // native UI.  dontSendNotification prevents a feedback loop via sliderValueChanged.
        juce::MessageManager::callAsync ([this, newValue]
        {
            slider_.setValue (newValue, juce::dontSendNotification);
            slider_.redoImage();   // setValue+dontSendNotification skips valueChanged→redoImage
            bridge_.setBaseValue (slot_, newValue);
        });
    }

    void parameterGestureChanged (int, bool) override {}

    void sliderValueChanged (juce::Slider* s) override
    {
        const float v = (float) s->getValue();
        param_.beginChangeGesture();
        param_.setValueNotifyingHost (v);
        param_.endChangeGesture();
        bridge_.setBaseValue (slot_, v);
    }

    juce::AudioProcessorParameter& param_;
    SynthSlider& slider_;
    VSTModulationBridge& bridge_;
    int slot_;
};

// ---------------------------------------------------------------------------
// VSTParametersView
// ---------------------------------------------------------------------------
class VSTParametersView : public SynthSection, private juce::Timer
{
public:
    // kNumColumns — knobs per row
    // kKnobSize   — diameter of the rotary knob
    // kSlotW      — horizontal cell width (wider than knob so labels aren't clipped)
    // kLabelH     — height of the text label underneath each knob
    // kGapX/Y     — horizontal / vertical gap between cells
    static constexpr int kNumColumns  = 10;
    // kKnobSize must be >= kKnobBodySize (40) + some margin so the sleeve renders
    // without clipping on the sides.  44 gives a 2-pixel margin each side.
    static constexpr int kKnobSize    = 44;
    static constexpr int kSlotW       = 80;
    static constexpr int kLabelH      = 18;
    static constexpr int kGapX        = 10;
    // kGapY must leave room above the knob for the modulation-amount hover knob
    // (24 px tall, placed at centreY - 48).  60 gives a comfortable gap.
    static constexpr int kGapY        = 60;

    VSTParametersView (juce::AudioPluginInstance* plugin,
                       VSTModulationBridge* bridge,
                       OpenGlWrapper& opengl,
                       juce::AudioProcessorGraph::Node::Ptr nodeRef)
        : SynthSection ("vstparams"), opengl_ (opengl),
          bridge_ (bridge), nodeRef_ (std::move (nodeRef))
    {
        jassert (plugin != nullptr);
        jassert (bridge != nullptr);

        setLookAndFeel (DefaultLookAndFeel::instance());
        setSkinOverride (Skin::kDirect);

        // Vertical title on the left, matching all other preparation panels.
        prepTitle_ = std::make_shared<PlainTextComponent> ("vstparams_title", "VST Modifications");
        addOpenGlComponent (prepTitle_);
        prepTitle_->setJustification (juce::Justification::centredLeft);
        prepTitle_->setFontType (PlainTextComponent::kTitle);
        prepTitle_->setRotation (-90);

        const juce::String bridgeUuid =
            bridge->getBridgeState().getProperty (IDs::uuid).toString();
        auto& pluginParams = plugin->getParameters();
        auto modParamsVt   = bridge->getBridgeState().getChildWithName (IDs::MODULATABLE_PARAMS);

        // SynthSection::addSlider composes the final component ID as:
        //   parent.getComponentID() + "_" + slider.getComponentID()
        // We want the final ID to be "<bridgeUuid>_<slotIndex>" so that
        // connectModulation's UUID-prefix parsing and ParamOffsetBank lookup
        // both work correctly.
        setComponentID (bridgeUuid);

        for (int slot = 0; slot < bridge->getNumAutomatableParams(); ++slot)
        {
            const int vstParamIdx = bridge->automatableParamIndices_[slot];
            auto* param = pluginParams[vstParamIdx];

            auto modParamVt = modParamsVt.getChild (slot);

            auto slider = std::make_unique<SynthSlider> (juce::String (slot), modParamVt);
            slider->setRange (0.0, 1.0, 0.0);
            slider->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
            slider->setShowPopupOnHover (true);

            const juce::String paramName = param->getName (40);
            slider->setTooltip (paramName);

            // Use PlainTextComponent (OpenGL) for labels — plain juce::Label is
            // invisible in OpenGL-rendered SynthSections.
            auto label = std::make_shared<PlainTextComponent> (
                juce::String (slot) + "_lbl", paramName);
            addOpenGlComponent (label);
            label->setJustification (juce::Justification::centred);

            addSlider (slider.get());

            auto attachment = std::make_unique<VSTParamAttachment> (
                *param, *slider, *bridge, slot);

            labels_.push_back (std::move (label));
            attachments_.push_back (std::move (attachment));
            sliders_.push_back (std::move (slider));
        }

        const int numSliders = (int) sliders_.size();
        const int numRows    = (numSliders + kNumColumns - 1) / kNumColumns;
        preferredHeight_     = kGapY + numRows * (kKnobSize + kLabelH + kGapY);

        // Poll the bridge for live modulated values at 30 Hz so the knob's
        // modulation-meter arc and hover popup reflect the actual modulated value.
        startTimerHz (30);
    }

    int getPreferredHeight() const { return preferredHeight_; }

    // juce::Timer: poll the bridge for live modulated values so the knob arcs
    // and hover popups show the current (post-modulation) value.
    void timerCallback() override
    {
        if (bridge_ == nullptr)
            return;
        for (int slot = 0; slot < (int) sliders_.size(); ++slot)
            sliders_[slot]->setLiveModulatedValue (bridge_->getModulatedValue (slot));
    }

    void resized() override
    {
        // Hide the soundset selector — this view has no sample-set concept.
        if (auto* fi = findParentComponentOfClass<FullInterface>())
            fi->hideSoundsetSelector();

        const int   titleW    = (int) getTitleWidth();
        const float titleSize = findValue (Skin::kPrepTitleSize);

        // PlainTextComponent with rotation=-90 maps x_draw → y_screen.
        // Left-aligned text (x_draw ≈ 0) maps to near the top of the panel,
        // so long titles like "VST Modifications" appear above centre.
        // Shift the title component DOWN by (textW - titleW) / 2 to compensate.
        const juce::Font titleFont =
            Fonts::instance()->proportional_title().withPointHeight (titleSize);
        const int textW      = juce::GlyphArrangement::getStringWidthInt (titleFont, "VST Modifications");
        const int titleShift = juce::jmax (0, (textW - titleW) / 2);

        juce::Rectangle<int> titleArea = getLocalBounds().removeFromLeft (titleW);
        prepTitle_->setBounds (titleArea.withY (titleShift));
        prepTitle_->setTextSize (titleSize);

        const float labelTextSize = findValue (Skin::kKnobLabelSizeMedium);
        const int   offsetX       = titleW; // grid starts after title column
        const int   slotW         = juce::jmax (kKnobSize, (getWidth() - offsetX) / kNumColumns);

        const int n = (int) sliders_.size();
        for (int i = 0; i < n; ++i)
        {
            const int col   = i % kNumColumns;
            const int row   = i / kNumColumns;
            const int slotX = offsetX + col * slotW;
            const int y     = kGapY + row * (kKnobSize + kLabelH + kGapY);
            const int knobX = slotX + (slotW - kKnobSize) / 2;

            sliders_[i]->setBounds (knobX, y, kKnobSize, kKnobSize);
            labels_[i]->setBounds  (slotX, y + kKnobSize, slotW, kLabelH);
            if (labelTextSize > 0.0f)
                labels_[i]->setTextSize (labelTextSize);
        }
    }

private:
    OpenGlWrapper& opengl_;
    VSTModulationBridge* bridge_; // non-owning; kept alive via nodeRef_
    // Keeps the plugin node (and thus its AudioProcessorParameters) alive for as
    // long as this view exists.
    juce::AudioProcessorGraph::Node::Ptr nodeRef_;

    std::shared_ptr<PlainTextComponent>              prepTitle_;
    std::vector<std::unique_ptr<SynthSlider>>        sliders_;
    std::vector<std::unique_ptr<VSTParamAttachment>> attachments_;
    std::vector<std::shared_ptr<PlainTextComponent>> labels_;
    int preferredHeight_ = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VSTParametersView)
};
