//
// Created by Myra Norton on 11/25/25.
//

#ifndef BITKLAVIER0_EQFILTERSECTION_H
#define BITKLAVIER0_EQFILTERSECTION_H

#pragma once

#include "EQFilterParams.h"
#include "filter_button.h"
#include "open_gl_combo_box.h"
#include "synth_section.h"
#include "synth_slider.h"
#include <chowdsp_plugin_utils/chowdsp_plugin_utils.h>

class EQPeakFilterSection : public SynthSection
{
public:
    EQPeakFilterSection (
        juce::String name,
        EQPeakFilterParams &params,
        chowdsp::ParameterListeners &listeners,
        SynthSection &parent);

    ~EQPeakFilterSection() override {}
    void paintBackground(juce::Graphics& g) override;
    void resized() override;

private:
    std::unique_ptr<filter_button> peak_filter_button;
    std::unique_ptr<chowdsp::ButtonAttachment> filter_active_attachment;

    std::unique_ptr<SynthSlider> freq_knob;
    std::unique_ptr<chowdsp::SliderAttachment> freq_knob_attachment;
    std::unique_ptr<SynthSlider> gain_knob;
    std::unique_ptr<chowdsp::SliderAttachment> gain_knob_attachment;
    std ::unique_ptr<SynthSlider> q_knob;
    std::unique_ptr<chowdsp::SliderAttachment> q_knob_attachment;

    // knob labels
    std::shared_ptr<PlainTextComponent> freq_knob_label;
    std::shared_ptr<PlainTextComponent> gain_knob_label;
    std::shared_ptr<PlainTextComponent> q_knob_label;


    juce::GroupComponent sectionBorder;
};

class EQCutFilterSection : public SynthSection
{
public:
    EQCutFilterSection (
        juce::String name,
        EQCutFilterParams &params,
        chowdsp::ParameterListeners &listeners,
        SynthSection &parent);

    ~EQCutFilterSection() override {}
    void paintBackground(juce::Graphics& g) override;
    void resized() override;

private:
    std::unique_ptr<filter_button> cut_filter_button;
    std::unique_ptr<chowdsp::ButtonAttachment> filter_active_attachment;

    std::unique_ptr<SynthSlider> freq_knob;
    std::unique_ptr<chowdsp::SliderAttachment> freq_knob_attachment;
    std::unique_ptr<SynthSlider> slope_knob;
    std::unique_ptr<chowdsp::SliderAttachment> slope_knob_attachment;

    // knob labels
    std::shared_ptr<PlainTextComponent> freq_knob_label;
    std::shared_ptr<PlainTextComponent> slope_knob_label;

    juce::GroupComponent sectionBorder;
};



#endif //BITKLAVIER0_EQFILTERSECTION_H
