//
// Created by Joshua Warner on 6/27/24.
//

#ifndef BITKLAVIER2_TUNINGPREPARATION_H
#define BITKLAVIER2_TUNINGPREPARATION_H



/************************************************************************************/
/*                 Created by Davis Polito and Joshua Warner                        */
/************************************************************************************/

#include "TuningProcessor.h"
#include "PreparationSection.h"
#include "popup_browser.h"
#include "FullInterface.h"
/************************************************************************************/
/*                              CLASS: OpenGlSlider                                 */
/************************************************************************************/

class OpenGlSlider;

/************************************************************************************/
/*             CLASS: TuningPreparation, inherits from PreparationSection           */
/************************************************************************************/

class TuningPreparation : public PreparationSection {
public:

    // Constructor method that takes three arguments: a smart pointer to a PolygonalOscProcessor,
    // a value tree, and a reference to an OpenGlWrapper object
    TuningPreparation(juce::ValueTree v, OpenGlWrapper& open_gl, juce::AudioProcessorGraph::NodeID node, SynthGuiInterface*);

    // Destructor method
    ~TuningPreparation();

    // Static function that returns a pointer to a TuningPreparation object
    static std::unique_ptr<PreparationSection> create(const juce::ValueTree& v, SynthGuiInterface* interface){
        return std::make_unique<TuningPreparation> (v, interface->getGui()->open_gl_, juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar (v.getProperty (IDs::nodeID)), interface);
    }

    // Public function definitions for the TuningPreparation class, which override functions
    // in the PreparationSection base class
    std::unique_ptr<SynthSection> getPrepPopup() override;
    void resized() override;


};

#endif //BITKLAVIER2_TUNINGPREPARATION_H
