//
// Created by Dan Trueman on 7/11/25.
//

#ifndef BITKLAVIER0_MIDIFILTERPREPARATION_H
#define BITKLAVIER0_MIDIFILTERPREPARATION_H
#pragma once
#include "MidiFilterProcessor.h"
#include "PreparationSection.h"
#include "popup_browser.h"
#include "FullInterface.h"

class MidiFilterPreparation : public PreparationSection
{
public:
    // Constructor method that takes three arguments: a smart pointer to a PolygonalOscProcessor,
    // a value tree, and a reference to an OpenGlWrapper object
    MidiFilterPreparation(juce::ValueTree v, OpenGlWrapper& open_gl, juce::AudioProcessorGraph::NodeID node, SynthGuiInterface*);

    // Destructor method
    ~MidiFilterPreparation();

    // Static function that returns a pointer to a MidiFilterPreparation object
    static std::unique_ptr<PreparationSection> create(const juce::ValueTree& v, SynthGuiInterface* interface){
        return std::make_unique<MidiFilterPreparation> (v, interface->getGui()->open_gl_, juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar (v.getProperty (IDs::nodeID)), interface);
    }

    // Public function definitions for the MidiFilterPreparation class, which override functions
    // in the PreparationSection base class
    std::unique_ptr<SynthSection> getPrepPopup() override;
    void resized() override;


};

#endif //BITKLAVIER0_MIDIFILTERPREPARATION_H
