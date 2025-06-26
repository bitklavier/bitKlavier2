//
// Created by Dan Trueman on 6/23/25.
//

#ifndef BITKLAVIER0_PEAK_METER_SECTION_H
#define BITKLAVIER0_PEAK_METER_SECTION_H

#pragma once

#include "synth_section.h"
#include "peak_meter_viewer.h"

class PeakMeterViewer;
class VolumeSlider;

class PeakMeterSection : public SynthSection {
public:
    PeakMeterSection(juce::String name, const std::tuple<std::atomic<float>, std::atomic<float>> *outputLevels);
    ~PeakMeterSection();

    int getMeterHeight();
    int getBuffer();
    void resized() override;
    void paintBackground(juce::Graphics& g) override;

private:

    std::shared_ptr<PeakMeterViewer> peak_meter_left_; // this shouldn't have to be shared_ptr, perhaps passing the pointer to the constructor above is the problem?
    std::shared_ptr<PeakMeterViewer> peak_meter_right_;
    std::shared_ptr<VolumeSlider> volume_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PeakMeterSection)
};
#endif //BITKLAVIER0_PEAK_METER_SECTION_H
