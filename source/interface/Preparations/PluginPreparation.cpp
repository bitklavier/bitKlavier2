/*********************************************************************************************/
/*                           Created by Davis Polito and Joshua Warner                       */
/*********************************************************************************************/

#include "PluginPreparation.h"
#include "BKitems/BKItem.h"
#include "synth_slider.h"

// Definition for the PluginPreparation constructor.  It takes three parameters: a pointer to
// a Plugin Processor p, a juce::ValueTree v, and a reference to an OpenGlWrapper object.  Initializes
// the base class members and private PluginPreparation member proc with an initialization list.
PluginPreparation::PluginPreparation (const juce::ValueTree& v, OpenGlWrapper &open_gl, juce::AudioProcessorGraph::NodeID node,  SynthGuiInterface* interface) :
                         PreparationSection(juce::String("Plugin"), v, open_gl,node, *interface->getUndoManager())

{

    item = std::make_unique<VSTItem> (); // Initializes member variable `item` of PreparationSection class
    addOpenGlComponent (item->getImageComponent(),true); // Calls member function of SynthSection (parent class to PreparationSection)
    _open_gl.context.executeOnGLThread([this](juce::OpenGLContext& context)
        {item->getImageComponent()->init(_open_gl);
        },false);

    addAndMakeVisible (item.get());
    setSkinOverride (Skin::kDirect);
    juce::MemoryBlock data;
}

std::unique_ptr<SynthSection> PluginPreparation::getPrepPopup()
{
    auto* interface = findParentComponentOfClass<SynthGuiInterface>();
    if (interface == nullptr)
        return nullptr;

    // The VST plugin's own processor.
    auto* vstNode = interface->getSynth()->getNodeForId (pluginID);
    if (vstNode == nullptr)
        return nullptr;
    auto* pluginInstance = dynamic_cast<juce::AudioPluginInstance*> (vstNode->getProcessor());
    if (pluginInstance == nullptr)
        return nullptr;

    // The bridge UUID is stored on the VST's state ValueTree.
    const juce::String bridgeUuidStr = state.getProperty (IDs::bridgeUuid).toString();
    if (bridgeUuidStr.isEmpty())
        return nullptr;

    const auto bridgeNodeID = juce::AudioProcessorGraph::NodeID (
        juce::Uuid (bridgeUuidStr).getTimeLow());
    auto* bridgeNode = interface->getSynth()->getNodeForId (bridgeNodeID);
    if (bridgeNode == nullptr)
        return nullptr;

    auto* bridge = dynamic_cast<VSTModulationBridge*> (bridgeNode->getProcessor());
    if (bridge == nullptr)
        return nullptr;

    return std::make_unique<VSTParametersView> (
        pluginInstance, bridge, interface->getGui()->open_gl_,
        juce::AudioProcessorGraph::Node::Ptr (vstNode));
}


void PluginPreparation::resized()
{
    PreparationSection::resized();
}

PluginPreparation::~PluginPreparation()
{
}



void PluginPreparation::paintBackground(juce::Graphics &g)  {
    for (auto * port: objects)
        port->redoImage();
    PreparationSection::paintBackground(g);
}


