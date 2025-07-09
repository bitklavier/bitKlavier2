/*********************************************************************************************/
/*                           Created by Davis Polito and Joshua Warner                       */
/*********************************************************************************************/

#include "ResetPreparation.h"

#include "BKitems/BKItem.h"
// #include "ResetParametersView.h"
#include "synth_base.h"
#include "synth_slider.h"

// Definition for the ResetPreparation constructor.  It takes three parameters: a pointer to
// a Reset Processor p, a juce::ValueTree v, and a reference to an OpenGlWrapper object.  Initializes
// the base class members and private ResetPreparation member proc with an initialization list.
ResetPreparation::ResetPreparation (juce::ValueTree v, OpenGlWrapper& open_gl, juce::AudioProcessorGraph::NodeID node, SynthGuiInterface* _synth_gui_interface) : PreparationSection (juce::String ("Reset"), v, open_gl, node, *_synth_gui_interface->getUndoManager())

{
    item = std::make_unique<ResetItem>(); // Initializes member variable `item` of PreparationSection class
    addOpenGlComponent (item->getImageComponent(), true); // Calls member function of SynthSection (parent class to PreparationSection)
    _open_gl.context.executeOnGLThread ([this] (juce::OpenGLContext& context) {
        item->getImageComponent()->init (_open_gl);
    },
        false);

    addAndMakeVisible (item.get());
    // setSkinOverride (Skin::kReset);
    juce::MemoryBlock data;

    width = 245;
    height = 125;
}

std::unique_ptr<SynthSection> ResetPreparation::getPrepPopup()
{
    // if (auto parent = findParentComponentOfClass<SynthGuiInterface>())
    //     if (auto* proc = dynamic_cast<ResetProcessor*> (getProcessor()))
    //         return std::make_unique<ResetParametersView> (proc->getState(), proc->getState().params, state.getProperty (IDs::uuid).toString(), open_gl);

    return nullptr;
}

void ResetPreparation::resized()
{
    PreparationSection::resized();
}

ResetPreparation::~ResetPreparation()
{
}

void ResetPreparation::paintBackground (juce::Graphics& g)
{
    for (auto* port : objects)
        port->redoImage();
    PreparationSection::paintBackground (g);
}
