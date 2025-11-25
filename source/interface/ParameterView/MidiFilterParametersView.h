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

    std::unique_ptr<SynthButton> invert_button;
    std::unique_ptr<chowdsp::ButtonAttachment> invert_button_attachment;

    std::unique_ptr<SynthButton> ignoreNoteOff_button;
    std::unique_ptr<chowdsp::ButtonAttachment> ignoreNoteOff_button_attachment;

    // place to store all the toggles for this prep, with their attachments for tracking/updating values
    std::vector<std::unique_ptr<SynthButton>> _paramToggles;
    std::vector<std::unique_ptr<chowdsp::ButtonAttachment>> _paramToggles_attachments;

    MidiFilterParams& params;

};

#endif //BITKLAVIER0_MIDIFILTERPARAMETERSVIEW_H
