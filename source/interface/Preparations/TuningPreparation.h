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
    static PreparationSection* createTuningSection(ValueTree v, OpenGlWrapper &um) {

        return new TuningPreparation(std::make_unique<TuningProcessor>(), v, um);
    }

    // Public function definitions for the TuningPreparation class, which override functions
    // in the PreparationSection base class
    std::shared_ptr<SynthSection> getPrepPopup() override;
    void resized() override;

    juce::AudioProcessor* getProcessor() override;
    std::unique_ptr<juce::AudioProcessor> getProcessorPtr() override;
private:

    // Private member variable for the TuningPreparation class: proc is a pointer to a
    // TuningProcessor Object
    TuningProcessor & proc;
    std::unique_ptr<TuningProcessor> _proc_ptr;

    /************************************************************************************/
    /*             NESTED CLASS: TuningPopup, inherits from PreparationPopup            */
    /************************************************************************************/

    class TuningPopup : public PreparationPopup {
    public:

        // Constructor method that takes two arguments: a smart pointer to a TuningProcessor,
        // and a reference to an OpenGlWrapper
        TuningPopup (TuningProcessor& proc, OpenGlWrapper& open_gl);

        // Public function definitions for the class, which override the base class methods for
        // initializing, rendering, resizing, and painting OpenGl components
        void initOpenGlComponents(OpenGlWrapper &open_gl) override;
        void renderOpenGlComponents(OpenGlWrapper& open_gl, bool animate) override;
        void resized() override;
        void paintBackground(Graphics& g) override
        {
            SynthSection::paintContainer(g);
            paintHeadingText(g);
            paintBorder(g);
            paintKnobShadows(g);
            paintChildrenBackgrounds(g);
        }

        int getViewPosition() {
            int view_height = getHeight();
            return view_height; //std::max(0, std::min<int>(selections_.size() * getRowHeight() - view_height, view_position_));
        }

        ~TuningPopup();


    private:

        // Private function definitions and member variables for the TuningPopup class
        void redoImage();
        TuningParams* params = nullptr;
        TuningProcessor& proc;

    };
};

#endif //BITKLAVIER2_TUNINGPREPARATION_H
