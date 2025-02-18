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
// Definition for the ModulationPreparation constructor.  It takes three parameters: a pointer to
// a Modulation Processor p, a juce::ValueTree v, and a reference to an OpenGlWrapper object.  Initializes
// the base class members and private ModulationPreparation member proc with an initialization list.
ModulationPreparation::ModulationPreparation (std::unique_ptr<bitklavier::ModulationProcessor> p,
                                      juce::ValueTree v, OpenGlWrapper& um,SynthGuiInterface* interface) :
        PreparationSection(juce::String("Modulation"), v, um,p.get()),
        proc(*p.get()),
        _proc_ptr(std::move(p)),
        mod_list(v,interface->getSynth(),&(proc))
{

    item = std::make_unique<ModulationItem> (); // Initializes member variable `item` of PreparationSection class
    addOpenGlComponent (item->getImageComponent()); // Calls member function of SynthSection (parent class to PreparationSection)
    _open_gl.context.executeOnGLThread([this](juce::OpenGLContext& context)
                                        {item->getImageComponent()->init(_open_gl);
                                        },false);

    addAndMakeVisible (item.get());
    setSkinOverride (Skin::kModulation);
    juce::MemoryBlock data;

}

std::unique_ptr<SynthSection> ModulationPreparation::getPrepPopup()
{

    return std::make_unique<ModulationModuleSection>(
            &mod_list,
                                                      state,
                                                      findParentComponentOfClass<SynthGuiInterface>()->getGui()->modulation_manager.get());

}


void ModulationPreparation::resized()
{
    PreparationSection::resized();
}

ModulationPreparation::~ModulationPreparation()
{
}

juce::AudioProcessor* ModulationPreparation::getProcessor()
{
    return &proc;
}

std::unique_ptr<juce::AudioProcessor> ModulationPreparation::getProcessorPtr()
{
    return std::move(_proc_ptr);
}


void ModulationPreparation::paintBackground(juce::Graphics &g)  {
    for (auto * port: objects)
        port->redoImage();
    PreparationSection::paintBackground(g);
}
/*                     NESTED CLASS: ModulationPopup, inherits from PreparationPopup                 */
/*************************************************************************************************/
