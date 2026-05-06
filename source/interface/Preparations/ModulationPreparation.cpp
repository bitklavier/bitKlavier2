// Copyright (C) 2022-2025 Dan Trueman
// SPDX-License-Identifier: GPL-3.0-or-later

//
// Created by Davis Polito on 1/30/25.
//
/*********************************************************************************************/
/*                           Created by Davis Polito and Joshua Warner                       */
/*********************************************************************************************/

#include "ModulationPreparation.h"
#include "BKitems/BKItem.h"
#include "synth_slider.h"
#include "ModulationItem.h"
#include "ModulationModuleSection.h"
#include "synth_base.h"
#include "VSTParametersView.h"
#include "VSTModulationBridge.h"
// Definition for the ModulationPreparation constructor.  It takes three parameters: a pointer to
// a Modulation Processor p, a juce::ValueTree v, and a reference to an OpenGlWrapper object.  Initializes
// the base class members and private ModulationPreparation member proc with an initialization list.
ModulationPreparation::ModulationPreparation ( juce::ValueTree v, OpenGlWrapper &open_gl, juce::AudioProcessorGraph::NodeID no,  SynthGuiInterface* interface) :
        PreparationSection(juce::String("Modulation"), v, open_gl,no, *interface->getUndoManager()),
        undo (*interface->getUndoManager())

{
    item = std::make_unique<ModulationItem> (); // Initializes member variable `item` of PreparationSection class
    addOpenGlComponent (item->getImageComponent(),true); // Calls member function of SynthSection (parent class to PreparationSection)
    _open_gl.context.executeOnGLThread([this](juce::OpenGLContext& context)
                                        {item->getImageComponent()->init(_open_gl);
                                        },false);

    addAndMakeVisible (item.get());
    setSkinOverride (Skin::kModulation);
    juce::MemoryBlock data;
}

std::unique_ptr<SynthSection> ModulationPreparation::getPrepPopup()
{
    if (auto parent = findParentComponentOfClass<SynthGuiInterface>())
        if (auto *proc = dynamic_cast<bitklavier::ModulationProcessor*>(getProcessor()))
            return std::make_unique<ModulationModuleSection>(
                proc->mod_list.get(),
                state,
                findParentComponentOfClass<SynthGuiInterface>()->getGui()->modulation_manager.get(),
                undo);

    return nullptr;
}

void ModulationPreparation::mouseDoubleClick (const juce::MouseEvent&)
{
    // If this modulation prep is wired to a VST, show VSTParametersView in prepDisplay.
    // Detect via MODCONNECTION NodeID-based src/dest (present as soon as the user
    // drags mod→VST, before any per-parameter ModulationConnections are created).

    auto pianoVt = state.getParent().getParent();
    if (pianoVt.isValid())
    {
        if (auto* interface = findParentComponentOfClass<SynthGuiInterface>())
        {
            const auto myNodeID = juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar (
                state.getProperty (IDs::nodeID));

            auto preparationsVt   = pianoVt.getChildWithName (IDs::PREPARATIONS);
            auto modConnectionsVt = pianoVt.getChildWithName (IDs::MODCONNECTIONS);

            for (int i = 0; i < modConnectionsVt.getNumChildren() && i >= 0; ++i)
            {
                auto conn = modConnectionsVt.getChild (i);
                if (! conn.hasType (IDs::MODCONNECTION)) continue;

                const auto srcID = juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar (
                    conn.getProperty (IDs::src));
                if (srcID != myNodeID) continue;

                const auto destID = juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar (
                    conn.getProperty (IDs::dest));

                // Find the destination prep VT by NodeID, then look for a vstbridge child.
                for (int j = 0; j < preparationsVt.getNumChildren(); ++j)
                {
                    auto destPrepVt = preparationsVt.getChild (j);
                    const auto prepNodeID = juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar (
                        destPrepVt.getProperty (IDs::nodeID));
                    if (prepNodeID != destID) continue;

                    auto bridgeVt = destPrepVt.getChildWithName (IDs::vstbridge);
                    if (! bridgeVt.isValid()) break;

                    const juce::String bridgeUuidStr = bridgeVt.getProperty (IDs::uuid).toString();
                    const auto bridgeNodeID = juce::AudioProcessorGraph::NodeID (
                        juce::Uuid (bridgeUuidStr).getTimeLow());
                    auto* bridgeNode = interface->getSynth()->getNodeForId (bridgeNodeID);
                    if (bridgeNode == nullptr) break;

                    auto* bridge = dynamic_cast<VSTModulationBridge*> (bridgeNode->getProcessor());
                    if (bridge == nullptr) break;

                    auto* vstNode = interface->getSynth()->getNodeForId (destID);
                    if (vstNode == nullptr) break;

                    auto* pluginInstance = dynamic_cast<juce::AudioPluginInstance*> (vstNode->getProcessor());
                    if (pluginInstance == nullptr) break;

                    auto popup = std::make_unique<VSTParametersView> (
                        pluginInstance, bridge,
                        interface->getGui()->open_gl_,
                        juce::AudioProcessorGraph::Node::Ptr (vstNode));
                    showPrepPopup (std::move (popup), destPrepVt,
                                   bitklavier::BKPreparationTypeNil);
                    break;
                }
            }
        }
    }

    // Always show the normal modulation popup.
    showPrepPopup (std::move (getPrepPopup()), state,
                   bitklavier::BKPreparationType::PreparationTypeModulation);
}

void ModulationPreparation::resized()
{
    PreparationSection::resized();
}

ModulationPreparation::~ModulationPreparation() {}

void ModulationPreparation::paintBackground(juce::Graphics &g)
{
    for (auto * port: objects)
        port->redoImage();
    PreparationSection::paintBackground(g);
}

