// Copyright (C) 2022-2025 Dan Trueman
// SPDX-License-Identifier: GPL-3.0-or-later

//
// Created by Dan Trueman on 7/15/25.
//
#pragma once
#ifndef BITKLAVIER0_OFFSETKNOBSECTION_H
#define BITKLAVIER0_OFFSETKNOBSECTION_H
#include "synth_section.h"
#include "OffsetKnobParam.h"
#include "OpenGL_LabeledBorder.h"

class OffsetKnobSection : public SynthSection
{
public:
    OffsetKnobSection(
        juce::String name,
        OffsetKnobParam &params,
        chowdsp::ParameterListeners &listeners,
        SynthSection &parent,
        chowdsp::PluginState& pluginState) : SynthSection(name)
        {
            setComponentID(parent.getComponentID());

            offsetKnob = std::make_unique<SynthSlider>(params.offSetSliderParam->paramID,params.offSetSliderParam->getModParam());
            addSlider(offsetKnob.get());
            offsetKnob->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            offsetKnob->setPopupPlacement(juce::BubbleComponent::below);
            offsetKnob->setShowPopupOnHover(true);
            offsetKnobAttachment = std::make_unique<chowdsp::SliderAttachment>(params.offSetSliderParam, listeners, *offsetKnob, pluginState.undoManager);
            offsetKnob->addAttachment(offsetKnobAttachment.get()); // for modulations
            offsetKnob->setTooltip ("Raise or lower the entire temperament in cents");

            offset_label = std::make_shared<PlainTextComponent>(offsetKnob->getName(), params.offSetSliderParam->getName(20));
            addOpenGlComponent(offset_label);
            offset_label->setTextSize (10.0f);
            offset_label->setJustification(juce::Justification::centred);

            sectionBorder = std::make_shared<OpenGL_LabeledBorder>("tuningoffset", "Offset");
            addBorder(sectionBorder.get());
        };

    virtual ~OffsetKnobSection() {};

    void paintBackground(juce::Graphics& g) override
    {
        setLabelFont(g);
        paintKnobShadows(g);
        paintChildrenBackgrounds(g);
    }

    void resized() override
    {
        juce::Rectangle<int> area (getLocalBounds());
        sectionBorder->setBounds(area);

        int largepadding = findValue(Skin::kLargePadding);
        area.reduce(largepadding, largepadding);

        offsetKnob->setBounds(area);

        int labelsectionheight = findValue(Skin::kLabelHeight);
        //juce::Rectangle<int> label_rect (offsetKnob->getX(), offsetKnob->getBottom() - 10, offsetKnob->getWidth(), labelsectionheight );
        juce::Rectangle<int> label_rect (offsetKnob->getX(), offsetKnob->getY() + offsetKnob->getHeight() / 2 + 20, offsetKnob->getWidth(), labelsectionheight );
        offset_label->setBounds(label_rect);

        SynthSection::resized();
    }

    std::unique_ptr<SynthSlider> offsetKnob;
    std::unique_ptr<chowdsp::SliderAttachment> offsetKnobAttachment;

    std::shared_ptr<PlainTextComponent> offset_label;
    std::shared_ptr<OpenGL_LabeledBorder> sectionBorder;
};

#endif //BITKLAVIER0_OFFSETKNOBSECTION_H
