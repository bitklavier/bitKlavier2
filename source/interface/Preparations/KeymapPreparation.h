//
// Created by Davis Polito on 6/24/24.
//

#ifndef BITKLAVIER2_KEYMAPPREPARATION_H
#define BITKLAVIER2_KEYMAPPREPARATION_H

#include "KeymapProcessor.h"
#include "PreparationSection.h"
#include "popup_browser.h"
#include "FullInterface.h"


class KeymapPreparation : public PreparationSection {
public:

// Constructor method that takes three arguments: a smart pointer to a PolygonalOscProcessor,
// a value tree, and a reference to an OpenGlWrapper object
    KeymapPreparation(const juce::ValueTree& v, OpenGlWrapper &open_gl, juce::AudioProcessorGraph::NodeID node,  SynthGuiInterface*);

// Destructor method
    ~KeymapPreparation();

// Static function that returns a pointer to a KeymapPreparation object
    static std::unique_ptr<PreparationSection> create(const juce::ValueTree& v, SynthGuiInterface* interface) {
        return  std::make_unique<KeymapPreparation>(v, interface->getGui()->open_gl_,juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar(v.getProperty(IDs::nodeID)),interface);
    }

// Public function definitions for the KeymapPreparation class, which override functions
// in the PreparationSection base class
    std::unique_ptr<SynthSection> getPrepPopup() override;





private:

/************************************************************************************/
/*             NESTED CLASS: KeymapPopup, inherits from PreparationPopup            */
/************************************************************************************/


};

#endif //BITKLAVIER2_KEYMAPPREPARATION_H
