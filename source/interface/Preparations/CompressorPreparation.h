//
// Created by Myra Norton on 11/14/25.
//

#ifndef BITKLAVIER2_COMPRESSORPREPARATION_H
#define BITKLAVIER2_COMPRESSORPREPARATION_H

#pragma once
#include "CompressorProcessor.h"
#include "FullInterface.h"
#include "PreparationSection.h"

/************************************************************************************/
/*      CLASS: CompressorPreparation, inherits from PreparationSection              */
/************************************************************************************/

class CompressorPreparation : public PreparationSection
{
public:
    CompressorPreparation (juce::ValueTree v, OpenGlWrapper& open_gl, juce::AudioProcessorGraph::NodeID node, SynthGuiInterface* _synth_gui_interface);

    // Destructor method
    ~CompressorPreparation();

    // Static function that returns a pointer to a CompressorPreparation object
    static std::unique_ptr<PreparationSection> create (const juce::ValueTree& v, SynthGuiInterface* interface);

    std::unique_ptr<SynthSection> getPrepPopup() override;
    void resized() override;
    void paintBackground (juce::Graphics& g);
};

#endif //BITKLAVIER2_COMPRESSORPREPARATION_H