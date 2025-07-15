//
// Created by Dan Trueman on 7/15/25.
//
#pragma once
#ifndef BITKLAVIER0_SECTIONBORDERWRAP_H
#define BITKLAVIER0_SECTIONBORDERWRAP_H

#include "synth_section.h"

class SectionBorderWrap : public SynthSection
{
public:
    SectionBorderWrap(
        juce::String name,
        SemitoneWidthParams &params,
        chowdsp::ParameterListeners &listeners,
        SynthSection &parent)
        {
            addAndMakeVisible(sectionBorder);
        };

    virtual ~SectionBorderWrap() {};

    void paintBackground(juce::Graphics& g) override
    {
        paintKnobShadows(g);
        paintChildrenBackgrounds(g);
        sectionBorder.paint(g);
    }

    void resized() override
    {
        juce::Rectangle<int> area (getLocalBounds());
        sectionBorder.setBounds(area);

        SynthSection::resized();
    }

    juce::GroupComponent sectionBorder;
};

#endif //BITKLAVIER0_SECTIONBORDERWRAP_H
