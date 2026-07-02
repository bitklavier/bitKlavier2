// Copyright (C) 2022-2025 Dan Trueman
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ChucKlavierPreparation.h"
#include "BKitems/BKItem.h"
#include "ChucKlavierParametersView.h"
#include "synth_slider.h"

ChucKlavierPreparation::ChucKlavierPreparation (
    juce::ValueTree v,
    OpenGlWrapper& open_gl,
    juce::AudioProcessorGraph::NodeID node,
    SynthGuiInterface* _synth_gui_interface)
    : PreparationSection ("chucklavier", v, open_gl, node, *_synth_gui_interface->getUndoManager())
{
    item = std::make_unique<ChucKlavierItem>();
    addOpenGlComponent (item->getImageComponent(), true);
    _open_gl.context.executeOnGLThread ([this] (juce::OpenGLContext&) {
        item->getImageComponent()->init (_open_gl);
    }, false);

    addAndMakeVisible (item.get());
    setSkinOverride (Skin::kBlendronic); // reuse Blendronic skin for Stage 1

    width  = state.getProperty (IDs::width);
    height = state.getProperty (IDs::height);
}

std::unique_ptr<SynthSection> ChucKlavierPreparation::getPrepPopup()
{
    if (auto parent = findParentComponentOfClass<SynthGuiInterface>())
        if (auto* proc = dynamic_cast<ChucKlavierProcessor*> (getProcessor()))
        {
            auto nodeId = juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar (
                state.getProperty (IDs::nodeID));
            return std::make_unique<ChucKlavierParametersView> (
                proc->getState(), proc->getState().params,
                state.getProperty (IDs::uuid).toString(),
                state,
                open_gl, parent->getSynth(), nodeId, proc);
        }
    return nullptr;
}

void ChucKlavierPreparation::resized()
{
    PreparationSection::resized();
}

ChucKlavierPreparation::~ChucKlavierPreparation() = default;

void ChucKlavierPreparation::paintBackground (juce::Graphics& g)
{
    for (auto* port : objects)
        port->redoImage();
    PreparationSection::paintBackground (g);
}
