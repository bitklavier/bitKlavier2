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
        paintBorder(g);
        paintKnobShadows(g);
        paintChildrenBackgrounds(g);
    }

    void resized() override;

    std::shared_ptr<PlainTextComponent> prepTitle;

    // place to store all the toggles for this prep, with their attachments for tracking/updating values
    std::vector<std::unique_ptr<SynthButton>> _paramToggles;
    std::vector<std::unique_ptr<chowdsp::ButtonAttachment>> _paramToggles_attachments;

    std::vector<std::unique_ptr<OpenGLComboBox>> _noteModeMenus;
    std::vector<std::unique_ptr<chowdsp::ComboBoxAttachment>> _noteModeMenus_attachments;

    MidiTargetParams& params;

};
#endif //BITKLAVIER0_MIDITARGETPARAMETERSVIEW_H
