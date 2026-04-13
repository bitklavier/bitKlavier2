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
    // If this modulation prep is connected to a VST parameter, also show the
    // VSTParametersView (knob grid) in addition to the normal modulation popup.
    // Connections live in PIANO/MODCONNECTIONS as ModulationConnection children:
    //   IDs::src  = "<thisUUID>_<modDetails>"
    //   IDs::dest = "<bridgeUUID>_<slotIdx>"   (slotIdx is a pure integer)

    const juce::String myUuid = state.getProperty (IDs::uuid).toString();

    auto pianoVt = state.getParent().getParent();
    if (pianoVt.isValid())
    {
        auto modConnectionsVt = pianoVt.getChildWithName (IDs::MODCONNECTIONS);
        juce::String foundBridgeUuid;

        auto checkConn = [&] (const juce::ValueTree& conn)
        {
            if (! conn.hasType (IDs::ModulationConnection)) return;
            const juce::String src = conn.getProperty (IDs::src).toString();
            if (! src.startsWith (myUuid + "_")) return;
            const juce::String dest = conn.getProperty (IDs::dest).toString();
            const int lastUnder = dest.lastIndexOfChar ('_');
            if (lastUnder <= 0) return;
            const juce::String slotPart = dest.substring (lastUnder + 1);
            if (slotPart.isNotEmpty() && slotPart.containsOnly ("0123456789"))
                foundBridgeUuid = dest.substring (0, lastUnder);
        };

        for (int i = 0; i < modConnectionsVt.getNumChildren(); ++i)
        {
            auto child = modConnectionsVt.getChild (i);
            if (child.hasType (IDs::ModulationConnection))
                checkConn (child);
            else if (child.hasType (IDs::MODCONNECTION))
                for (int j = 0; j < child.getNumChildren(); ++j)
                    checkConn (child.getChild (j));
        }

        if (foundBridgeUuid.isNotEmpty())
        {
            if (auto* interface = findParentComponentOfClass<SynthGuiInterface>())
            {
                const auto bridgeNodeID = juce::AudioProcessorGraph::NodeID (
                    juce::Uuid (foundBridgeUuid).getTimeLow());
                auto* bridgeNode = interface->getSynth()->getNodeForId (bridgeNodeID);
                if (bridgeNode)
                {
                    if (auto* bridge = dynamic_cast<VSTModulationBridge*> (bridgeNode->getProcessor()))
                    {
                        auto vstPrepVt = bridge->getBridgeState().getParent();
                        const auto vstNodeID =
                            juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar (
                                vstPrepVt.getProperty (IDs::nodeID));
                        auto* vstNode = interface->getSynth()->getNodeForId (vstNodeID);
                        if (vstNode)
                        {
                            if (auto* pluginInstance =
                                    dynamic_cast<juce::AudioPluginInstance*> (vstNode->getProcessor()))
                            {
                                auto popup = std::make_unique<VSTParametersView> (
                                    pluginInstance, bridge,
                                    interface->getGui()->open_gl_,
                                    juce::AudioProcessorGraph::Node::Ptr (vstNode));
                                showPrepPopup (std::move (popup), vstPrepVt,
                                               bitklavier::BKPreparationTypeNil);
                            }
                        }
                    }
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

