//
// Created by Dan Trueman on 8/2/25.
//

#ifndef BITKLAVIER0_MIDITARGETPARAMETERSVIEW_H
#define BITKLAVIER0_MIDITARGETPARAMETERSVIEW_H

#pragma once
#include "MidiTargetProcessor.h"
#include "synth_section.h"
#include "synth_slider.h"
#include "open_gl_combo_box.h"

class MidiTargetParametersView : public SynthSection
{
public:
    MidiTargetParametersView(chowdsp::PluginState& pluginState, MidiTargetParams& param, juce::String name, OpenGlWrapper *open_gl);

    void paintBackground(juce::Graphics& g) override
    {
        SynthSection::paintContainer(g);
        paintHeadingText(g);
        paintBorder(g);
        paintKnobShadows(g);
        paintChildrenBackgrounds(g);
    }

    void resized() override;

    std::unique_ptr<SynthButton> mf_button;
    std::unique_ptr<chowdsp::ButtonAttachment> mf_button_attachment;

    MidiTargetParams& params;

};
#endif //BITKLAVIER0_MIDITARGETPARAMETERSVIEW_H
