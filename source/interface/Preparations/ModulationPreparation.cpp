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
// Definition for the ModulationPreparation constructor.  It takes three parameters: a pointer to
// a Modulation Processor p, a juce::ValueTree v, and a reference to an OpenGlWrapper object.  Initializes
// the base class members and private ModulationPreparation member proc with an initialization list.
ModulationPreparation::ModulationPreparation ( juce::ValueTree v, OpenGlWrapper &open_gl, juce::AudioProcessorGraph::NodeID no,  SynthGuiInterface* interface) :
        PreparationSection(juce::String("Modulation"), v, open_gl,no, *interface->getUndoManager()),
        undo (*interface->getUndoManager()),
        mod_list(v,interface->getSynth(),dynamic_cast<bitklavier::ModulationProcessor*>(interface->getSynth()->getNodeForId(no)->getProcessor()))
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
            &mod_list,
                                                      state,
                                                      findParentComponentOfClass<SynthGuiInterface>()->getGui()->modulation_manager.get(),
                                                      undo);

    return nullptr;

}


void ModulationPreparation::resized()
{
    PreparationSection::resized();
}

ModulationPreparation::~ModulationPreparation()
{
}


void ModulationPreparation::paintBackground(juce::Graphics &g)  {
    for (auto * port: objects)
        port->redoImage();
    PreparationSection::paintBackground(g);
}

