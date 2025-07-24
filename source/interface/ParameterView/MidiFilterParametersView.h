//
// Created by Dan Trueman on 7/11/25.
//

#ifndef BITKLAVIER0_MIDIFILTERPARAMETERSVIEW_H
#define BITKLAVIER0_MIDIFILTERPARAMETERSVIEW_H
#pragma once
#include "MidiFilterProcessor.h"
#include "synth_section.h"
#include "synth_slider.h"
#include "open_gl_combo_box.h"

class MidiFilterParametersView : public SynthSection
{
public:
    MidiFilterParametersView(chowdsp::PluginState& pluginState, MidiFilterParams& param, juce::String name, OpenGlWrapper *open_gl);

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

    MidiFilterParams& params;

};

#endif //BITKLAVIER0_MIDIFILTERPARAMETERSVIEW_H
