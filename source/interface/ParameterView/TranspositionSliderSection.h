//
// Created by Davis Polito on 2/17/25.
//

#ifndef BITKLAVIER2_TRANSPOSITIONSLIDERSECTION_H
#define BITKLAVIER2_TRANSPOSITIONSLIDERSECTION_H
#include "OpenGL_TranspositionSlider.h"
#include "TransposeParams.h"
#include "synth_section.h"
class TranspositionSliderSection : public SynthSection
{
    TranspositionSliderSection(TransposeParams *params, chowdsp::ParameterListenerThread& listeners)
    : slider(std::make_unique<OpenGL_TranspositionSlider>(params,listeners)), SynthSection("")
    {
        
    }
    
    void resized();
    std::unique_ptr<OpenGL_TranspositionSlider> slider;
    std::unique_ptr<SynthButton> on_;
};
#endif //BITKLAVIER2_TRANSPOSITIONSLIDERSECTION_H
