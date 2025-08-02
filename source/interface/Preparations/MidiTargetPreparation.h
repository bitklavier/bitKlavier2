//
// Created by Dan Trueman on 8/2/25.
//

#ifndef BITKLAVIER0_MIDITARGETPREPARATION_H
#define BITKLAVIER0_MIDITARGETPREPARATION_H

#pragma once
#include "MidiTargetProcessor.h"
#include "PreparationSection.h"
#include "popup_browser.h"
#include "FullInterface.h"

class MidiTargetPreparation : public PreparationSection
{
public:
    // Constructor method that takes three arguments: a smart pointer to a PolygonalOscProcessor,
    // a value tree, and a reference to an OpenGlWrapper object
    MidiTargetPreparation(juce::ValueTree v, OpenGlWrapper& open_gl, juce::AudioProcessorGraph::NodeID node, SynthGuiInterface*);

    // Destructor method
    ~MidiTargetPreparation();

    // Static function that returns a pointer to a MidiTargetPreparation object
    static std::unique_ptr<PreparationSection> create(const juce::ValueTree& v, SynthGuiInterface* interface){
        return std::make_unique<MidiTargetPreparation> (v, interface->getGui()->open_gl_, juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar (v.getProperty (IDs::nodeID)), interface);
    }

    // Public function definitions for the MidiTargetPreparation class, which override functions
    // in the PreparationSection base class
    std::unique_ptr<SynthSection> getPrepPopup() override;
    void resized() override;


};
#endif //BITKLAVIER0_MIDITARGETPREPARATION_H
