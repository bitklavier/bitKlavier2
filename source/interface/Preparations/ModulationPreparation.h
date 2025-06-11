//
// Created by Davis Polito on 1/30/25.
//

#ifndef BITKLAVIER2_MODULATIONPREPARATION_H
#define BITKLAVIER2_MODULATIONPREPARATION_H


/************************************************************************************/
/*                 Created by Davis Polito                   */
/************************************************************************************/

#include "ModulationProcessor.h"
#include "PreparationSection.h"
#include "popup_browser.h"

#include "FullInterface.h"

#include "envelope_section.h"
#include "ModulationList.h"
class ModulationModuleSection;
/************************************************************************************/
/*             CLASS: ModulationPreparation, inherits from PreparationSection           */
/************************************************************************************/

class ModulationPreparation : public PreparationSection
{
public:

    // Constructor method that takes three arguments: a smart pointer to a PolygonalOscProcessor,
    // a value tree, and a reference to an OpenGlWrapper object
    ModulationPreparation( juce::ValueTree v, OpenGlWrapper &open_gl, juce::AudioProcessorGraph::NodeID node,  SynthGuiInterface*);

    // Destructor method
    ~ModulationPreparation();

    // Static function that returns a pointer to a ModulationPreparation object
    static PreparationSection* createModulationSection(juce::ValueTree v, SynthGuiInterface* interface) {

        return new ModulationPreparation(v, interface->getGui()->open_gl_,juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar(v.getProperty(IDs::nodeID)),interface);
    }





    std::unique_ptr<SynthSection> getPrepPopup() override;
    void resized() override;
    void paintBackground(juce::Graphics &g);


private:
    ModulationList mod_list;

};



#endif //BITKLAVIER2_MODULATIONPREPARATION_H
