/************************************************************************************/
/*                 Created by Davis Polito and Joshua Warner                        */
/************************************************************************************/

#ifndef BITKLAVIER2_NOSTALGICPREPARATION_H
#define BITKLAVIER2_NOSTALGICPREPARATION_H

#pragma once
#include "NostalgicProcessor.h"
#include "FullInterface.h"
#include "PreparationSection.h"
#include "popup_browser.h"

/************************************************************************************/
/*             CLASS: NostalgicPreparation, inherits from PreparationSection           */
/************************************************************************************/

class NostalgicPreparation : public PreparationSection
{
public:
    NostalgicPreparation (juce::ValueTree v, OpenGlWrapper& open_gl, juce::AudioProcessorGraph::NodeID node, SynthGuiInterface* _synth_gui_interface);

    // Destructor method
    ~NostalgicPreparation();

    // Static function that returns a pointer to a NostalgicPreparation object
    static std::unique_ptr<PreparationSection> create (const juce::ValueTree& v, SynthGuiInterface* interface)
    {
        return std::make_unique<NostalgicPreparation> (v, interface->getGui()->open_gl_, juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar (v.getProperty (IDs::nodeID)), interface);
    }

    // Public function definitions for the NostalgicPreparation class, which override functions
    // in the PreparationSection base class
    void addSoundSet (std::map<juce::String, juce::ReferenceCountedArray<BKSamplerSound<juce::AudioFormatReader>>>* s) override
    {
        // if (auto processor = dynamic_cast<NostalgicProcessor*> (getProcessor()))
        //     processor->addSoundSet (s);
    }

    void addSoundSet (
        juce::ReferenceCountedArray<BKSamplerSound<juce::AudioFormatReader>>* s,
        juce::ReferenceCountedArray<BKSamplerSound<juce::AudioFormatReader>>* h,
        juce::ReferenceCountedArray<BKSamplerSound<juce::AudioFormatReader>>* r,
        juce::ReferenceCountedArray<BKSamplerSound<juce::AudioFormatReader>>* p) override
    {
        // if (auto processor = dynamic_cast<NostalgicProcessor*> (getProcessor()))
        //     processor->addSoundSet (s, h, r, p);
    }

    std::unique_ptr<SynthSection> getPrepPopup() override;
    void resized() override;
    void paintBackground (juce::Graphics& g);
};

#endif // BITKLAVIER2_NOSTALGICPREPARATION_H