//
// Created by Dan Trueman on 8/5/25.
//

#ifndef BITKLAVIER0_SYNCHRONICPREPARATION_H
#define BITKLAVIER0_SYNCHRONICPREPARATION_H

#pragma once
#include "SynchronicProcessor.h"
#include "FullInterface.h"
#include "PreparationSection.h"
#include "popup_browser.h"

/************************************************************************************/
/*             CLASS: SynchronicPreparation, inherits from PreparationSection           */
/************************************************************************************/

class SynchronicPreparation : public PreparationSection
{
public:
    SynchronicPreparation (juce::ValueTree v, OpenGlWrapper& open_gl, juce::AudioProcessorGraph::NodeID node, SynthGuiInterface* _synth_gui_interface);

    // Destructor method
    ~SynchronicPreparation();

    // Static function that returns a pointer to a DirectPreparation object
    static std::unique_ptr<PreparationSection> create (const juce::ValueTree& v, SynthGuiInterface* interface)
    {
        return std::make_unique<SynchronicPreparation> (v, interface->getGui()->open_gl_, juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar (v.getProperty (IDs::nodeID)), interface);
    }

    // Public function definitions for the DirectPreparation class, which override functions
    // in the PreparationSection base class
    void addSoundSet (std::map<juce::String, juce::ReferenceCountedArray<BKSamplerSound<juce::AudioFormatReader>>>* s) override
    {
        if (auto processor = dynamic_cast<SynchronicPreparation*> (getProcessor()))
            processor->addSoundSet (s);
    }

    void addSoundSet (
        juce::ReferenceCountedArray<BKSamplerSound<juce::AudioFormatReader>>* s,
        juce::ReferenceCountedArray<BKSamplerSound<juce::AudioFormatReader>>* h,
        juce::ReferenceCountedArray<BKSamplerSound<juce::AudioFormatReader>>* r,
        juce::ReferenceCountedArray<BKSamplerSound<juce::AudioFormatReader>>* p) override
    {
        if (auto processor = dynamic_cast<SynchronicProcessor*> (getProcessor()))
            processor->addSoundSet (s);
    }

    std::unique_ptr<SynthSection> getPrepPopup() override;
    void resized() override;
    void paintBackground (juce::Graphics& g);
};

#endif //BITKLAVIER0_SYNCHRONICPREPARATION_H
