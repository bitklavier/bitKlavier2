/*********************************************************************************************/
/*                           Created by Davis Polito and Joshua Warner                       */
/*********************************************************************************************/

#include "PluginPreparation.h"
#include "BKitems/BKItem.h"
#include "synth_slider.h"

// Definition for the PluginPreparation constructor.  It takes three parameters: a pointer to
// a Plugin Processor p, a juce::ValueTree v, and a reference to an OpenGlWrapper object.  Initializes
// the base class members and private PluginPreparation member proc with an initialization list.
PluginPreparation::PluginPreparation (std::unique_ptr<juce::AudioPluginInstance> p,
    juce::ValueTree v, OpenGlWrapper& um) :
                         PreparationSection(juce::String("Plugin"), v, um,p.get()),
                         proc(*p.get()),
                         _proc_ptr(std::move(p))
{

    item = std::make_unique<DirectItem> (); // Initializes member variable `item` of PreparationSection class
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
    return nullptr;//std::make_unique<PluginParametersView>(proc.getState(), proc.getState().params,proc.v.getProperty(IDs::uuid).toString(), open_gl);
}


void PluginPreparation::resized()
{
    PreparationSection::resized();
}

PluginPreparation::~PluginPreparation()
{
}

juce::AudioProcessor* PluginPreparation::getProcessor()
{
    return &proc;
}

std::unique_ptr<juce::AudioProcessor> PluginPreparation::getProcessorPtr()
{
    return std::move(_proc_ptr);
}


void PluginPreparation::paintBackground(juce::Graphics &g)  {
    for (auto * port: objects)
        port->redoImage();
    PreparationSection::paintBackground(g);
}


