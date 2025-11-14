//
// Created by Dan Trueman on 6/23/25.
//

#ifndef BITKLAVIER0_PEAK_METER_SECTION_H
#define BITKLAVIER0_PEAK_METER_SECTION_H

#pragma once

#include <chowdsp_plugin_state/chowdsp_plugin_state.h>

#include "peak_meter_viewer.h"
#include "synth_section.h"
#include <chowdsp_plugin_utils/chowdsp_plugin_utils.h>

class PeakMeterViewer;
class VolumeSlider;

class PeakMeterSection : public SynthSection {
public:
    PeakMeterSection( juce::String name,
        chowdsp::FloatParameter& param,
        chowdsp::ParameterListeners& listeners,
        const std::tuple<std::atomic<float>, std::atomic<float>> *outputLevels);
    ~PeakMeterSection();

//    int getMeterHeight();
    int getBuffer();

    void setLabel(juce::String newLabel);
    void setColor(juce::Colour color);
    void resized() override;
    void paintBackground(juce::Graphics& g) override;

    std::shared_ptr<VolumeSlider> volume_;

private:
    std::shared_ptr<PeakMeterViewer> peak_meter_left_;
    std::shared_ptr<PeakMeterViewer> peak_meter_right_;
    std::unique_ptr<chowdsp::SliderAttachment> volumeAttach_;
    std::shared_ptr<PlainTextComponent> peak_meter_label;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PeakMeterSection)
};
#endif //BITKLAVIER0_PEAK_METER_SECTION_H
