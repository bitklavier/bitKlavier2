/*********************************************************************************************/
/*                           Created by Davis Polito and Joshua Warner                       */
/*********************************************************************************************/

#include "DirectPreparation.h"
#include "BKitems/BKItem.h"
#include "DirectParametersView.h"
#include "synth_slider.h"

// Definition for the DirectPreparation constructor.  It takes three parameters: a pointer to
// a Direct Processor p, a juce::ValueTree v, and a reference to an OpenGlWrapper object.  Initializes
// the base class members and private DirectPreparation member proc with an initialization list.
DirectPreparation::DirectPreparation (
    juce::ValueTree v,
    OpenGlWrapper& open_gl,
    juce::AudioProcessorGraph::NodeID node,
    SynthGuiInterface* _synth_gui_interface) :
                                               PreparationSection (juce::String ("direct"),
                                                   v,
                                                   open_gl,
                                                   node,
                                                   *_synth_gui_interface->getUndoManager()
                                                   )

{
    item = std::make_unique<DirectItem>(); // Initializes member variable `item` of PreparationSection class
    addOpenGlComponent (item->getImageComponent(), true); // Calls member function of SynthSection (parent class to PreparationSection)
    _open_gl.context.executeOnGLThread ([this] (juce::OpenGLContext& context) {
        item->getImageComponent()->init (_open_gl);
    },
        false);

    addAndMakeVisible (item.get());
    setSkinOverride (Skin::kDirect);
    juce::MemoryBlock data;

    width = 245;
    height = 125;
}

std::unique_ptr<SynthSection> DirectPreparation::getPrepPopup()
{
    if (auto parent = findParentComponentOfClass<SynthGuiInterface>())
        if (auto* proc = dynamic_cast<DirectProcessor*> (getProcessor()))
            return std::make_unique<DirectParametersView> (proc->getState(), proc->getState().params, state.getProperty (IDs::uuid).toString(), open_gl);

    return nullptr;
}

void DirectPreparation::resized()
{
    PreparationSection::resized();
}

DirectPreparation::~DirectPreparation()
{
}

void DirectPreparation::paintBackground (juce::Graphics& g)
{
    for (auto* port : objects)
        port->redoImage();
    PreparationSection::paintBackground (g);
}
