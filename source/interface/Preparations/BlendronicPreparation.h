//
// Created by Dan Trueman on 7/26/25.
//

#ifndef BITKLAVIER0_BLENDRONICPREPARATION_H
#define BITKLAVIER0_BLENDRONICPREPARATION_H
#pragma once
#include "BlendronicProcessor.h"
#include "FullInterface.h"
#include "PreparationSection.h"
#include "popup_browser.h"

class BlendronicPreparation : public PreparationSection
{
public:
    BlendronicPreparation (juce::ValueTree v, OpenGlWrapper& open_gl, juce::AudioProcessorGraph::NodeID node, SynthGuiInterface* _synth_gui_interface);

    // Destructor method
    ~BlendronicPreparation();

    // Static function that returns a pointer to a DirectPreparation object
    static std::unique_ptr<PreparationSection> create (const juce::ValueTree& v, SynthGuiInterface* interface)
    {
        return std::make_unique<BlendronicPreparation> (v, interface->getGui()->open_gl_, juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar (v.getProperty (IDs::nodeID)), interface);
    }

    std::unique_ptr<SynthSection> getPrepPopup() override;
    void resized() override;
    void paintBackground (juce::Graphics& g);
};

#endif //BITKLAVIER0_BLENDRONICPREPARATION_H
