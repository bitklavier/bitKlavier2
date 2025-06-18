//
// Created by Davis Polito on 6/24/24.
//

#ifndef BITKLAVIER2_KEYMAPPREPARATION_H
#define BITKLAVIER2_KEYMAPPREPARATION_H

#include "KeymapProcessor.h"
#include "PreparationSection.h"
#include "popup_browser.h"
#include "FullInterface.h"


class KeymapPreparation : public PreparationSection {
public:

// Constructor method that takes three arguments: a smart pointer to a PolygonalOscProcessor,
// a value tree, and a reference to an OpenGlWrapper object
    KeymapPreparation(std::unique_ptr<KeymapProcessor> proc, juce::ValueTree v, OpenGlWrapper &um);

// Destructor method
    ~KeymapPreparation();

// Static function that returns a pointer to a KeymapPreparation object
    static PreparationSection *createKeymapSection(juce::ValueTree v, SynthGuiInterface* interface) {

        return new KeymapPreparation(std::make_unique<KeymapProcessor>(v,interface->getAudioDeviceManager()), v, interface->getGui()->open_gl_);
    }

// Public function definitions for the KeymapPreparation class, which override functions
// in the PreparationSection base class
    std::unique_ptr<SynthSection> getPrepPopup() override;



    juce::AudioProcessor *getProcessor() override;

    std::unique_ptr<juce::AudioProcessor> getProcessorPtr() override;

private:

// Private member variable for the KeymapPreparation class: proc is a pointer to a
// KeymapProcessor Object
    KeymapProcessor &proc;
    std::unique_ptr<KeymapProcessor> _proc_ptr;

/************************************************************************************/
/*             NESTED CLASS: KeymapPopup, inherits from PreparationPopup            */
/************************************************************************************/


};

#endif //BITKLAVIER2_KEYMAPPREPARATION_H
