/************************************************************************************/
/*                 Created by Davis Polito and Joshua Warner                        */
/************************************************************************************/

#ifndef BITKLAVIER2_ResetPREPARATION_H
#define BITKLAVIER2_ResetPREPARATION_H


#include "FullInterface.h"
#include "PreparationSection.h"
#include "popup_browser.h"

/************************************************************************************/
/*             CLASS: ResetPreparation, inherits from PreparationSection           */
/************************************************************************************/

class ResetPreparation : public PreparationSection
{
public:
    ResetPreparation (juce::ValueTree v, OpenGlWrapper& open_gl, juce::AudioProcessorGraph::NodeID node, SynthGuiInterface* _synth_gui_interface);

    // Destructor method
    ~ResetPreparation();

    // Static function that returns a pointer to a ResetPreparation object
    static std::unique_ptr<PreparationSection> create (const juce::ValueTree& v, SynthGuiInterface* interface)
    {
        return std::make_unique<ResetPreparation> (v, interface->getGui()->open_gl_, juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar (v.getProperty (IDs::nodeID)), interface);
    }
    void mouseDoubleClick(const juce::MouseEvent &event) override {

    }

    std::unique_ptr<SynthSection> getPrepPopup() override;
    void resized() override;
    void paintBackground (juce::Graphics& g);
};

#endif // BITKLAVIER2_ResetPREPARATION_H