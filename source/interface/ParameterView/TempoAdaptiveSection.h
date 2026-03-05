//
// Created by Junie on 3/4/26.
//

#ifndef TEMPOADAPTIVESECTION_H
#define TEMPOADAPTIVESECTION_H

#include "synth_section.h"
#include "synth_slider.h"
#include "synth_button.h"
#include "OpenGL_LabeledBorder.h"
#include "OpenGL_HoldTimeMinMaxSlider.h"
#include "TempoProcessor.h"

class TempoAdaptiveSection : public SynthSection
{
public:
    TempoAdaptiveSection (chowdsp::PluginState& pluginState, TempoParams& params, juce::String name, TempoProcessor* proc);
    ~TempoAdaptiveSection() override = default;

    void paintBackground (juce::Graphics& g) override;
    void resized() override;
    void updateDisplays();

    std::unique_ptr<SynthSlider> historySlider;
    std::unique_ptr<chowdsp::SliderAttachment> historyAttachment;
    std::shared_ptr<PlainTextComponent> historyLabel;

    std::unique_ptr<OpenGL_HoldTimeMinMaxSlider> timeWindowMinMaxSlider;

    std::shared_ptr<PlainTextComponent> currentTempoDisplay;
    std::shared_ptr<PlainTextComponent> adaptiveMultiplierDisplay;
    std::unique_ptr<SynthButton> resetButton;

    std::shared_ptr<OpenGL_LabeledBorder> adaptiveKnobsBorder;

private:
    TempoProcessor* processor;
    TempoParams& params;
};

#endif //TEMPOADAPTIVESECTION_H
