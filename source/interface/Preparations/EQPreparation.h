//
// Created by Myra Norton on 11/14/25.
//

#ifndef BITKLAVIER2_EQPREPARATION_H
#define BITKLAVIER2_EQPREPARATION_H

#pragma once
#include "EQProcessor.h"
#include "FullInterface.h"
#include "PreparationSection.h"

/************************************************************************************/
/*              CLASS: EQPreparation, inherits from PreparationSection              */
/************************************************************************************/

class EQPreparation : public PreparationSection
{
public:
    EQPreparation (juce::ValueTree v, OpenGlWrapper& open_gl, juce::AudioProcessorGraph::NodeID node, SynthGuiInterface* _synth_gui_interface);

    // Destructor method
    ~EQPreparation();

    // Static function that returns a pointer to an EQPreparation object
    static std::unique_ptr<PreparationSection> create (const juce::ValueTree& v, SynthGuiInterface* interface)
    {
        return std::make_unique<EQPreparation> (v, interface->getGui()->open_gl_, juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar (v.getProperty (IDs::nodeID)), interface);
    }

    std::unique_ptr<SynthSection> getPrepPopup() override;
    void resized() override;
    void paintBackground (juce::Graphics& g);
};

#endif //BITKLAVIER2_EQPREPARATION_H
