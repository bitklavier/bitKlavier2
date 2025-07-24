//
// Created by Dan Trueman on 7/22/25.
//

#ifndef BITKLAVIER0_PIANOSWITCHPREPARATION_H
#define BITKLAVIER0_PIANOSWITCHPREPARATION_H

#pragma once
#include "PianoSwitchProcessor.h"
#include "PreparationSection.h"
#include "popup_browser.h"
#include "FullInterface.h"

class PianoSwitchPreparation : public PreparationSection
{
public:
    PianoSwitchPreparation(juce::ValueTree v, OpenGlWrapper& open_gl, juce::AudioProcessorGraph::NodeID node, SynthGuiInterface*);
    ~PianoSwitchPreparation(){}

    static std::unique_ptr<PreparationSection> create(const juce::ValueTree& v, SynthGuiInterface* interface)
    {
        return std::make_unique<PianoSwitchPreparation> (
            v,
            interface->getGui()->open_gl_,
            juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar (v.getProperty (IDs::nodeID)),
            interface
            );
    }

    /**
     * todo: need to get this to not crash when double-clicked
     */
    std::unique_ptr<SynthSection> getPrepPopup() override;
    void resized() override;

    void buttonClicked(juce::Button *clicked_button) override;
    int currentPianoIndex = 0;

    std::shared_ptr<PlainTextComponent> pianoSelectText;
    std::unique_ptr<juce::ShapeButton> pianoSelector ;
};

#endif //BITKLAVIER0_PIANOSWITCHPREPARATION_H
