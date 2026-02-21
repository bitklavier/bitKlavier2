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
    if (_open_gl.context.isAttached() && _open_gl.context.isActive())
    {
        _open_gl.context.executeOnGLThread([this](juce::OpenGLContext& context)
            {item->getImageComponent()->init(_open_gl);
            },false);
    }

    addAndMakeVisible (item.get());
    setSkinOverride (Skin::kDirect);
    juce::MemoryBlock data;
}

std::unique_ptr<SynthSection> PluginPreparation::getPrepPopup()
{
    return nullptr;//std::make_unique<PluginParametersView>(proc.getState(), proc.getState().params,proc.v.getProperty(IDs::uuid).toString(), open_gl);
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


