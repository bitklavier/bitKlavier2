/************************************************************************************/
/*                 Created by Davis Polito and Joshua Warner                        */
/************************************************************************************/

#ifndef BITKLAVIER2_DIRECTPREPARATION_H
#define BITKLAVIER2_DIRECTPREPARATION_H

#include "DirectProcessor.h"
#include "FullInterface.h"
#include "PreparationSection.h"
#include "popup_browser.h"

/************************************************************************************/
/*             CLASS: DirectPreparation, inherits from PreparationSection           */
/************************************************************************************/

class DirectPreparation : public PreparationSection
{
public:
    DirectPreparation (juce::ValueTree v, OpenGlWrapper& open_gl, juce::AudioProcessorGraph::NodeID node, SynthGuiInterface*);

    // Destructor method
    ~DirectPreparation();

    // Static function that returns a pointer to a DirectPreparation object
    static std::unique_ptr<PreparationSection> create (const juce::ValueTree& v, SynthGuiInterface* interface)
    {
        return std::make_unique<DirectPreparation> (v, interface->getGui()->open_gl_, juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar (v.getProperty (IDs::nodeID)), interface);
    }

    // Public function definitions for the DirectPreparation class, which override functions
    // in the PreparationSection base class
    void addSoundSet (std::map<juce::String, juce::ReferenceCountedArray<BKSamplerSound<juce::AudioFormatReader>>>* s) override
    {
        if (auto processor = dynamic_cast<DirectProcessor*> (getProcessor()))
            processor->addSoundSet (s);
    }

    void addSoundSet (
        juce::ReferenceCountedArray<BKSamplerSound<juce::AudioFormatReader>>* s,
        juce::ReferenceCountedArray<BKSamplerSound<juce::AudioFormatReader>>* h,
        juce::ReferenceCountedArray<BKSamplerSound<juce::AudioFormatReader>>* r,
        juce::ReferenceCountedArray<BKSamplerSound<juce::AudioFormatReader>>* p) override
    {
        if (auto processor = dynamic_cast<DirectProcessor*> (getProcessor()))
            processor->addSoundSet (s, h, r, p);
    }

    std::unique_ptr<SynthSection> getPrepPopup() override;
    void resized() override;
    void paintBackground (juce::Graphics& g);
};

#endif // BITKLAVIER2_DIRECTPREPARATION_H