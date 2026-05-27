// Copyright (C) 2022-2025 Dan Trueman
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ReverbPreparation.h"
#include "BKitems/BKItem.h"
#include "ReverbParameterView.h"

ReverbPreparation::ReverbPreparation (
    juce::ValueTree v,
    OpenGlWrapper& open_gl,
    juce::AudioProcessorGraph::NodeID node,
    SynthGuiInterface* _synth_gui_interface) :
        PreparationSection (juce::String ("Reverb"),
            v,
            open_gl,
            node,
            *_synth_gui_interface->getUndoManager())
{
    item = std::make_unique<ReverbItem>();
    addOpenGlComponent (item->getImageComponent(), true);
    _open_gl.context.executeOnGLThread ([this] (juce::OpenGLContext& context) {
        item->getImageComponent()->init (_open_gl);
    }, false);

    addAndMakeVisible (item.get());
}

std::unique_ptr<SynthSection> ReverbPreparation::getPrepPopup()
{
    if (auto parent = findParentComponentOfClass<SynthGuiInterface>())
        if (auto* proc = dynamic_cast<ReverbProcessor*> (getProcessor())) {
            auto nodeId = juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar (state.getProperty (IDs::nodeID));
            return std::make_unique<ReverbParameterView> (proc->getState(), proc->getState().params, state.getProperty (IDs::uuid).toString(), open_gl, proc, true, parent->getSynth(), nodeId);
        }

    return nullptr;
}

void ReverbPreparation::resized()
{
    PreparationSection::resized();
}

ReverbPreparation::~ReverbPreparation()
{
}

void ReverbPreparation::paintBackground (juce::Graphics& g)
{
    for (auto* port : objects)
        port->redoImage();
    PreparationSection::paintBackground (g);
}
