//
// Created by Dan Trueman on 7/15/25.
//
#pragma once
#ifndef BITKLAVIER0_OFFSETKNOBSECTION_H
#define BITKLAVIER0_OFFSETKNOBSECTION_H
#include "synth_section.h"
#include "OffsetKnobParam.h"

class OffsetKnobSection : public SynthSection
{
public:
    OffsetKnobSection(
        juce::String name,
        OffsetKnobParam &params,
        chowdsp::ParameterListeners &listeners,
        SynthSection &parent) : SynthSection(name)
        {
            setComponentID(parent.getComponentID());

            offsetKnob = std::make_unique<SynthSlider>(params.offSet->paramID);
            addSlider(offsetKnob.get());
            offsetKnob->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            offsetKnob->setPopupPlacement(juce::BubbleComponent::below);
            offsetKnob->setShowPopupOnHover(true);
            offsetKnobAttachment = std::make_unique<chowdsp::SliderAttachment>(params.offSet, listeners, *offsetKnob, nullptr);

            sectionBorder.setName("tuningoffset");
            sectionBorder.setText("Offset");
            sectionBorder.setTextLabelPosition(juce::Justification::centred);
            addAndMakeVisible(sectionBorder);
        };

    virtual ~OffsetKnobSection() {};

    void paintBackground(juce::Graphics& g) override
    {
        paintKnobShadows(g);
        paintChildrenBackgrounds(g);
//        drawLabelForComponent(g, offsetKnob->getName(), offsetKnob.get());
        drawLabelForComponent(g, TRANS("cents"), offsetKnob.get());
        sectionBorder.paint(g);
    }

    void resized() override
    {
        juce::Rectangle<int> area (getLocalBounds());
        sectionBorder.setBounds(area);

        int largepadding = findValue(Skin::kLargePadding);
        area.reduce(largepadding, largepadding);

        offsetKnob->setBounds(area);

        SynthSection::resized();
    }

    std::unique_ptr<SynthSlider> offsetKnob;
    std::unique_ptr<chowdsp::SliderAttachment> offsetKnobAttachment;
    juce::GroupComponent sectionBorder;
};

#endif //BITKLAVIER0_OFFSETKNOBSECTION_H
