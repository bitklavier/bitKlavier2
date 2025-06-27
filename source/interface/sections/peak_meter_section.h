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

    int getMeterHeight();
    int getBuffer();
    void resized() override;
    void paintBackground(juce::Graphics& g) override;

private:

    std::shared_ptr<PeakMeterViewer> peak_meter_left_;
    std::shared_ptr<PeakMeterViewer> peak_meter_right_;
    std::shared_ptr<VolumeSlider> volume_;
    std::unique_ptr<chowdsp::SliderAttachment> volumeAttach_;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PeakMeterSection)
};
#endif //BITKLAVIER0_PEAK_METER_SECTION_H
