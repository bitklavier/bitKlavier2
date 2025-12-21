//
// Created by Davis Polito on 1/30/25.
//

#ifndef BITKLAVIER2_MODULATIONPREPARATION_H
#define BITKLAVIER2_MODULATIONPREPARATION_H

#include "ModulationProcessor.h"
#include "PreparationSection.h"
#include "popup_browser.h"
#include "FullInterface.h"

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

    static std::unique_ptr<PreparationSection> create(const juce::ValueTree& v, SynthGuiInterface* interface) {

        return std::make_unique<ModulationPreparation> (v, interface->getGui()->open_gl_,juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar(v.getProperty(IDs::nodeID)),interface);
    }

    void mouseDoubleClick(const juce::MouseEvent &event) override {

        showPrepPopup(std::move(this->getPrepPopup()),state,bitklavier::BKPreparationType::PreparationTypeModulation);
    }

    std::unique_ptr<SynthSection> getPrepPopup() override;
    void resized() override;
    void paintBackground(juce::Graphics &g);

private:
    juce::UndoManager& undo;

};



#endif //BITKLAVIER2_MODULATIONPREPARATION_H
