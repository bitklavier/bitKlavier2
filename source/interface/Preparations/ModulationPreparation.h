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
//#include "ParameterView/ModulationParametersView.h"
#include "ModulationProcessor.h"
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
    ModulationPreparation(std::unique_ptr<bitklavier::ModulationProcessor> proc, juce::ValueTree v, OpenGlWrapper& um, SynthGuiInterface*);

    // Destructor method
    ~ModulationPreparation();

    // Static function that returns a pointer to a ModulationPreparation object
    static PreparationSection* createModulationSection(juce::ValueTree v, SynthGuiInterface* interface) {

        return new ModulationPreparation(std::make_unique<bitklavier::ModulationProcessor>(v), v, interface->getGui()->open_gl_,interface);
    }





    std::unique_ptr<SynthSection> getPrepPopup() override;
    void resized() override;
    void paintBackground(juce::Graphics &g);
    juce::AudioProcessor* getProcessor() override;
    std::unique_ptr<juce::AudioProcessor> getProcessorPtr() override;

private:
    // Private member variable for the ModulationPreparation class: proc is a pointer to a
    // ModulationProcessor Object
    bitklavier::ModulationProcessor & proc;
    std::unique_ptr<bitklavier::ModulationProcessor> _proc_ptr;

    ModulationList mod_list;

};



#endif //BITKLAVIER2_MODULATIONPREPARATION_H
