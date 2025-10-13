//
// Created by Dan Trueman on 6/19/25.
//

#ifndef BITKLAVIER0_TRANSPOSITIONSLIDERSECTION_H
#define BITKLAVIER0_TRANSPOSITIONSLIDERSECTION_H
#include "../components/opengl/OpenGL_TranspositionSlider.h"
#include "TransposeParams.h"
#include "synth_section.h"

/*
 * TranspositionSliderSection combines a StackedSlider (wrapped in OpenGL_TranspositionSlider) with a toggle
 *  the toggle "on" is to set whether the transposition slider uses the attached Tuning or not
 *  so, if it is toggled true, and the TranspositionSlider has values of 0 and 4, the 4 will be tuned according
 *  to the Tuning (say, 14c flat for basic 5/4 M3rd)
 *  Otherwise, the 4 will be an ET M3rd
 */
class TranspositionSliderSection : public SynthSection
{
public:
    TranspositionSliderSection(TransposeParams *params, chowdsp::ParameterListeners& listeners, std::string parent_uuid)
        : slider(std::make_unique<OpenGL_TranspositionSlider>(params,listeners)), SynthSection("")
    {
        setComponentID(parent_uuid);

        //button for setting whether the transposition slider uses tuning from an attached Tuning
        on = std::make_unique<SynthButton>(params->transpositionUsesTuning->paramID);

        // "on" has to have an attachment so we can track its value; the listeners for this preparation will now know about it
        on_attachment = std::make_unique<chowdsp::ButtonAttachment>(params->transpositionUsesTuning,listeners,*on,nullptr);

        // unique ID that will be combined with parentID in addSynthButton so we can track this
        on->setComponentID(params->transpositionUsesTuning->paramID);

        // add it and show it
        addSynthButton(on.get(), true, true);

        // what we want the button to show to the user
        on->setText("use Tuning?");

        //set component id to map to statechange params set in processor constructor
        slider->setComponentID("transpose");

        //addStateModulatedComponent is needed for this complex slider to get picked up by modulations
        // since we have 12 individual sliders here, these will be wrapped into one state change for all together
        // don't need to call this for simpler things like a basic slider or the button above
        addStateModulatedComponent(slider.get());
    }

    ~TranspositionSliderSection() {}

    void paintBackground(juce::Graphics& g) {
//        paintContainer(g);
//        paintHeadingText(g);
//
//        paintKnobShadows(g);
//        paintChildrenBackgrounds(g);
//        paintBorder(g);
    }

    void resized() override;

    std::unique_ptr<OpenGL_TranspositionSlider> slider;
    std::unique_ptr<SynthButton> on;
    std::unique_ptr<chowdsp::ButtonAttachment> on_attachment;
};

#endif //BITKLAVIER0_TRANSPOSITIONSLIDERSECTION_H
