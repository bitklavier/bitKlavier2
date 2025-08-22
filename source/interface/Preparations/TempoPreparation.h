//
// Created by Myra Norton on 8/20/25.
//

#ifndef TEMPOPREPARATION_H
#define TEMPOPREPARATION_H

#pragma once
#include "TempoProcessor.h"
#include "FullInterface.h"
#include "PreparationSection.h"


/************************************************************************************/
/*             CLASS: TempoPreparation, inherits from PreparationSection           */
/************************************************************************************/

class TempoPreparation : public PreparationSection
{
public:
    TempoPreparation (juce::ValueTree v, OpenGlWrapper& open_gl, juce::AudioProcessorGraph::NodeID node, SynthGuiInterface* _synth_gui_interface);
    ~TempoPreparation();

    // Static function that returns a pointer to a DirectPreparation object
    static std::unique_ptr<PreparationSection> create (const juce::ValueTree& v, SynthGuiInterface* interface)
    {
        return std::make_unique<TempoPreparation> (v, interface->getGui()->open_gl_, juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar (v.getProperty (IDs::nodeID)), interface);
    }

    std::unique_ptr<SynthSection> getPrepPopup() override;
    void resized() override;
    void paintBackground (juce::Graphics& g);
};

#endif //TEMPOPREPARATION_H
