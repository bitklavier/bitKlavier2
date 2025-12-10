//
// Created by Myra Norton on 11/25/25.
//

#ifndef BITKLAVIER0_EQFILTERSECTION_H
#define BITKLAVIER0_EQFILTERSECTION_H

#pragma once

#include "synth_section.h"
#include <chowdsp_plugin_utils/chowdsp_plugin_utils.h>
#include "EQFilterParams.h"
#include "open_gl_combo_box.h"
#include "synth_slider.h"

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
    std::unique_ptr<SynthButton> active_toggle;
    std::unique_ptr<chowdsp::ButtonAttachment> filter_active_attachment;

    std::unique_ptr<SynthSlider> freq_knob;
    std::unique_ptr<chowdsp::SliderAttachment> freq_knob_attachment;
    std::unique_ptr<SynthSlider> gain_knob;
    std::unique_ptr<chowdsp::SliderAttachment> gain_knob_attachment;
    std ::unique_ptr<SynthSlider> q_knob;
    std::unique_ptr<chowdsp::SliderAttachment> q_knob_attachment;

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
    std::unique_ptr<SynthButton> active_toggle;
    std::unique_ptr<chowdsp::ButtonAttachment> filter_active_attachment;

    std::unique_ptr<SynthSlider> freq_knob;
    std::unique_ptr<chowdsp::SliderAttachment> freq_knob_attachment;
    std::unique_ptr<SynthSlider> slope_knob;
    std::unique_ptr<chowdsp::SliderAttachment> slope_knob_attachment;

    juce::GroupComponent sectionBorder;
};



#endif //BITKLAVIER0_EQFILTERSECTION_H
