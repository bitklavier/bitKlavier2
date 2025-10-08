//
// Created by Dan Trueman on 10/8/25.
//
#ifndef BITKLAVIER0_RESONANCEPREPARATION_H
#define BITKLAVIER0_RESONANCEPREPARATION_H

#pragma once
#include "ResonanceProcessor.h"
#include "FullInterface.h"
#include "PreparationSection.h"
#include "popup_browser.h"

class ResonancePreparation : public PreparationSection
{
public:
    ResonancePreparation (juce::ValueTree v, OpenGlWrapper& open_gl, juce::AudioProcessorGraph::NodeID node, SynthGuiInterface* _synth_gui_interface);

    // Destructor method
    ~ResonancePreparation();

    // Static function that returns a pointer to a DirectPreparation object
    static std::unique_ptr<PreparationSection> create (const juce::ValueTree& v, SynthGuiInterface* interface)
    {
        return std::make_unique<ResonancePreparation> (v, interface->getGui()->open_gl_, juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar (v.getProperty (IDs::nodeID)), interface);
    }

    // Public function definitions for the DirectPreparation class, which override functions
    // in the PreparationSection base class
    void addSoundSet (std::map<juce::String, juce::ReferenceCountedArray<BKSamplerSound<juce::AudioFormatReader>>>* s) override
    {
        if (auto processor = dynamic_cast<ResonancePreparation*> (getProcessor()))
            processor->addSoundSet (s);
    }

    void addSoundSet (
        juce::ReferenceCountedArray<BKSamplerSound<juce::AudioFormatReader>>* s,
        juce::ReferenceCountedArray<BKSamplerSound<juce::AudioFormatReader>>* h,
        juce::ReferenceCountedArray<BKSamplerSound<juce::AudioFormatReader>>* r,
        juce::ReferenceCountedArray<BKSamplerSound<juce::AudioFormatReader>>* p) override
    {
        if (auto processor = dynamic_cast<ResonanceProcessor*> (getProcessor()))
            processor->addSoundSet (s);
    }

    std::unique_ptr<SynthSection> getPrepPopup() override;
    void resized() override;
    void paintBackground (juce::Graphics& g);
};

#endif //BITKLAVIER0_RESONANCEPREPARATION_H