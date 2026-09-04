// Copyright (C) 2022-2025 Dan Trueman
// SPDX-License-Identifier: GPL-3.0-or-later

//
// ChucKKnobPanel.h
//
// A 2-column grid of SynthSliders for ChucK script-exposed global float
// variables annotated with //@knob metadata.  Mirrors VSTParametersView
// architecturally: slot-indexed params, 30 Hz live-value polling, componentID
// composition via SynthSection::addSlider.
//
// All 32 potential sliders are pre-allocated at construction; only active slots
// are visible.  This avoids the need to add/remove OpenGL components at runtime
// (SynthSection has no removeOpenGlComponent API).

#pragma once

#include "../sections/synth_section.h"
#include "../components/synth_slider.h"
#include "../components/opengl/open_gl_image_component.h"
#include "../../synthesis/framework/Processors/ChucKlavierProcessor.h"
#include "../../common/Identifiers.h"

// ---------------------------------------------------------------------------
// ChuckParamAttachment
//
// One-way link: slider changes -> chowdsp FloatParameter.
// No reverse listener — ChucK VM doesn't notify us of global float writes.
// ---------------------------------------------------------------------------
class ChuckParamAttachment final : private juce::Slider::Listener
{
public:
    // minVal/maxVal are the user-defined display range; param is normalized 0..1.
    ChuckParamAttachment (chowdsp::FloatParameter& param, SynthSlider& slider,
                          float minVal, float maxVal)
        : param_ (param), slider_ (slider), minVal_ (minVal), maxVal_ (maxVal)
    {
        const float range    = maxVal_ - minVal_;
        const float initDisp = range > 0.0f ? minVal_ + param_.getCurrentValue() * range : minVal_;
        slider_.setValue (initDisp, juce::dontSendNotification);
        slider_.addListener (this);
    }

    ~ChuckParamAttachment() override
    {
        slider_.removeListener (this);
    }

    void updateRange (float minVal, float maxVal)
    {
        minVal_ = minVal;
        maxVal_ = maxVal;
    }

private:
    void sliderValueChanged (juce::Slider* s) override
    {
        const float v     = (float) s->getValue();
        const float range = maxVal_ - minVal_;
        const float norm  = range > 0.0f ? juce::jlimit (0.0f, 1.0f, (v - minVal_) / range) : 0.0f;
        param_.beginChangeGesture();
        param_.setValueNotifyingHost (norm);
        param_.endChangeGesture();
    }

    chowdsp::FloatParameter& param_;
    SynthSlider&             slider_;
    float minVal_, maxVal_;
};

// ---------------------------------------------------------------------------
// ChucKKnobPanel
//
// All kMaxChuckModParams slots are pre-allocated; visibility reflects active/inactive
// state.  Call refresh() after reconcileSlots() to sync visibility and labels.
// ---------------------------------------------------------------------------
class ChucKKnobPanel : public SynthSection, private juce::Timer
{
public:
    static constexpr int kNumColumns = 2;
    static constexpr int kKnobSize   = 44;   // >= kKnobBodySize (40) + 2px margin
    static constexpr int kSlotW      = 80;
    static constexpr int kLabelH     = 18;
    static constexpr int kGapX       = 10;
    static constexpr int kGapY       = 60;   // room for mod-amount hover knob above

    // proc     — ChucKlavier processor owning slots_ and modulatedValues_[].
    // prepUuid — prep UUID; sets parent componentID so addSlider composes
    //            final IDs as "<prepUuid>_ChuckMod<slot>".
    ChucKKnobPanel (ChucKlavierProcessor& proc,
                    const juce::String&   prepUuid,
                    OpenGlWrapper&        opengl)
        : SynthSection ("chuckknobs"),
          proc_   (proc),
          opengl_ (opengl)
    {
        setLookAndFeel (DefaultLookAndFeel::instance());
        setSkinOverride (Skin::kDirect);
        setComponentID (prepUuid);

        auto& params      = proc_.getState().params;
        auto  modParamsVt = proc_.v.getChildWithName (IDs::MODULATABLE_PARAMS);

        for (int slot = 0; slot < ChucKlavierParams::kMaxChuckModParams; ++slot)
        {
            auto modParamVt = modParamsVt.getChildWithProperty (
                IDs::parameter,
                juce::String ("ChuckMod") + juce::String (slot));

            const juce::String sliderID = juce::String ("ChuckMod") + juce::String (slot);

            const auto& binding = proc_.getSlotBinding (slot);
            const float rangeMin = binding.active ? binding.minVal : 0.0f;
            const float rangeMax = binding.active ? binding.maxVal : 1.0f;

            auto slider = std::make_unique<SynthSlider> (sliderID, modParamVt);
            slider->setRange (rangeMin, rangeMax, 0.0);
            slider->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
            slider->setShowPopupOnHover (true);
            slider->setShowSecondaryPopupOnHover (true);

            addSlider (slider.get()); // composes final ID = <prepUuid>_ChuckMod<slot>
            slider->setVisible (false);

            auto lbl = std::make_shared<PlainTextComponent> (sliderID + "_lbl", sliderID);
            addOpenGlComponent (lbl);
            lbl->setJustification (juce::Justification::centred);
            lbl->setVisible (false);

            if (params.chuckMod[slot] != nullptr)
                attachments_.push_back (std::make_unique<ChuckParamAttachment> (
                    *params.chuckMod[slot], *slider, rangeMin, rangeMax));
            else
                attachments_.push_back (nullptr);

            sliders_.push_back (std::move (slider));
            labels_.push_back (std::move (lbl));
        }

        updateVisibility();
        startTimerHz (30);
    }

    ~ChucKKnobPanel() override
    {
        stopTimer();
    }

    // Update visibility and labels after reconcileSlots() updates the processor's slot table.
    // Call this on the message thread after a successful hot-swap.
    void refresh()
    {
        updateVisibility();
        resized();
        repaint();
    }

    int getPreferredHeight() const { return preferredHeight_; }

    void resized() override
    {
        const int gridW   = kNumColumns * kSlotW + (kNumColumns - 1) * kGapX;
        const int offsetX = juce::jmax (0, (getWidth() - gridW) / 2);

        int activeIdx = 0; // sequential layout index (skips inactive slots)
        for (int slot = 0; slot < (int) sliders_.size(); ++slot)
        {
            const auto& binding = proc_.getSlotBinding (slot);
            if (! binding.active) continue;
            if (! sliders_[slot]) continue;

            const int col   = activeIdx % kNumColumns;
            const int row   = activeIdx / kNumColumns;
            const int slotX = offsetX + col * (kSlotW + kGapX);
            const int y     = kGapY + row * (kKnobSize + kLabelH + kGapY);
            const int knobX = slotX + (kSlotW - kKnobSize) / 2;
            sliders_[slot]->setBounds (knobX, y, kKnobSize, kKnobSize);
            // Rebuild the OpenGL texture now that we have valid bounds.
            sliders_[slot]->redoImage();
            if (labels_[slot])
            {
                labels_[slot]->setBounds (slotX, y + kKnobSize, kSlotW, kLabelH);
                const float sz = findValue (Skin::kKnobLabelSizeMedium);
                if (sz > 0.0f)
                    labels_[slot]->setTextSize (sz);
            }
            ++activeIdx;
        }
    }

    void paintBackground (juce::Graphics& g) override
    {
        setLabelFont (g);
        SynthSection::paintContainer (g);
        paintKnobShadows (g);
        paintChildrenBackgrounds (g);
    }

private:
    void updateVisibility()
    {
        int activeCount = 0;
        for (int slot = 0; slot < ChucKlavierParams::kMaxChuckModParams; ++slot)
        {
            const auto& binding = proc_.getSlotBinding (slot);
            const bool  active  = binding.active;

            if (sliders_[slot])
            {
                const bool wasVisible = sliders_[slot]->isVisible();
                sliders_[slot]->setVisible (active);

                if (active)
                {
                    // Update display range from binding (may have changed since construction).
                    sliders_[slot]->setRange (binding.minVal, binding.maxVal, 0.0);

                    // Sync initial display position from normalized param value.
                    if (attachments_[slot] != nullptr)
                        attachments_[slot]->updateRange (binding.minVal, binding.maxVal);

                    if (auto* param = proc_.getState().params.chuckMod[slot].get())
                    {
                        const float range = binding.maxVal - binding.minVal;
                        const float disp  = range > 0.0f
                                                ? binding.minVal + param->getCurrentValue() * range
                                                : binding.minVal;
                        sliders_[slot]->setValue (disp, juce::dontSendNotification);
                    }

                    (void) wasVisible; // redoImage() is called in resized() after setBounds
                }
            }

            if (labels_[slot])
            {
                labels_[slot]->setVisible (active);
                if (active)
                {
                    const juce::String txt = binding.label.isNotEmpty() ? binding.label : binding.name;
                    labels_[slot]->setText (txt);
                    if (sliders_[slot]) sliders_[slot]->setTooltip (txt);
                }
            }
            if (active) ++activeCount;
        }
        const int numRows = (activeCount + kNumColumns - 1) / kNumColumns;
        preferredHeight_  = kGapY + numRows * (kKnobSize + kLabelH + kGapY);
        if (activeCount == 0) preferredHeight_ = kGapY;
    }

    // 30 Hz: push live modulated values into visible sliders for arc display.
    void timerCallback() override
    {
        for (int slot = 0; slot < (int) sliders_.size(); ++slot)
        {
            if (! sliders_[slot] || ! sliders_[slot]->isVisible()) continue;
            const float norm    = proc_.modulatedValues_[slot].load (std::memory_order_relaxed);
            const auto& binding = proc_.getSlotBinding (slot);
            const float displayV = binding.minVal + norm * (binding.maxVal - binding.minVal);
            sliders_[slot]->setLiveModulatedValue (displayV);
        }
    }

    ChucKlavierProcessor&                            proc_;
    OpenGlWrapper&                                   opengl_;
    std::vector<std::unique_ptr<SynthSlider>>        sliders_;
    std::vector<std::shared_ptr<PlainTextComponent>> labels_;
    std::vector<std::unique_ptr<ChuckParamAttachment>> attachments_;
    int preferredHeight_ = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChucKKnobPanel)
};
