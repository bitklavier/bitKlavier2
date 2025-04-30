//
// Created by Joshua Warner on 6/27/24.
//

#ifndef BITKLAVIER2_TUNINGPREPARATION_H
#define BITKLAVIER2_TUNINGPREPARATION_H



/************************************************************************************/
/*                 Created by Davis Polito and Joshua Warner                        */
/************************************************************************************/

#include "TuningProcessor.h"
#include "PreparationSection.h"
#include "popup_browser.h"
#include "FullInterface.h"
/************************************************************************************/
/*                              CLASS: OpenGlSlider                                 */
/************************************************************************************/

class OpenGlSlider;

/************************************************************************************/
/*             CLASS: TuningPreparation, inherits from PreparationSection           */
/************************************************************************************/

class TuningPreparation : public PreparationSection {
public:

    // Constructor method that takes three arguments: a smart pointer to a PolygonalOscProcessor,
    // a value tree, and a reference to an OpenGlWrapper object
    TuningPreparation(std::unique_ptr<TuningProcessor> proc, juce::ValueTree v, OpenGlWrapper& um);

    // Destructor method
    ~TuningPreparation();

    // Static function that returns a pointer to a TuningPreparation object
    static PreparationSection* createTuningSection(juce::ValueTree v, SynthGuiInterface* interface) {

        return new TuningPreparation(std::make_unique<TuningProcessor>(interface->getSynth(),v), v, interface->getGui()->open_gl_);
    }

    // Public function definitions for the TuningPreparation class, which override functions
    // in the PreparationSection base class
    std::unique_ptr<SynthSection> getPrepPopup() override;
    void resized() override;

    juce::AudioProcessor* getProcessor() override;
    std::unique_ptr<juce::AudioProcessor> getProcessorPtr() override;
private:

    // Private member variable for the TuningPreparation class: proc is a pointer to a
    // TuningProcessor Object
    TuningProcessor & proc;
    std::unique_ptr<TuningProcessor> _proc_ptr;



};

#endif //BITKLAVIER2_TUNINGPREPARATION_H
