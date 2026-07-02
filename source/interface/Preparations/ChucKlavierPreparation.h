// Copyright (C) 2022-2025 Dan Trueman
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "ChucKlavierProcessor.h"
#include "FullInterface.h"
#include "PreparationSection.h"
#include "popup_browser.h"

class ChucKlavierPreparation : public PreparationSection
{
public:
    ChucKlavierPreparation (juce::ValueTree v, OpenGlWrapper& open_gl,
                            juce::AudioProcessorGraph::NodeID node,
                            SynthGuiInterface* _synth_gui_interface);
    ~ChucKlavierPreparation();

    static std::unique_ptr<PreparationSection> create (const juce::ValueTree& v, SynthGuiInterface* interface)
    {
        return std::make_unique<ChucKlavierPreparation> (
            v,
            interface->getGui()->open_gl_,
            juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar (v.getProperty (IDs::nodeID)),
            interface);
    }

    std::unique_ptr<SynthSection> getPrepPopup() override;
    void resized() override;
    void paintBackground (juce::Graphics& g) override;
};
