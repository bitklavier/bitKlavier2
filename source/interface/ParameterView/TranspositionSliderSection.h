//
// Created by Davis Polito on 2/17/25.
//

#ifndef BITKLAVIER2_TRANSPOSITIONSLIDERSECTION_H
#define BITKLAVIER2_TRANSPOSITIONSLIDERSECTION_H
#include "synth_section.h"
#include "OpenGlTranspositionSlider.h" 
#include "TransposeParams.h"
class TranspositionSliderSection : public SynthSection
{
    TranspositionSliderSection(TransposeParams *params, chowdsp::ParameterListenerThread& listeners)
    : slider(std::make_unique<OpenGlTranspositionSlider>(params,listeners)), SynthSection("")
    {
        
    }
    
    void resized();
    std::unique_ptr<OpenGlTranspositionSlider> slider; 
    std::unique_ptr<SynthButton> on_;
};
#endif //BITKLAVIER2_TRANSPOSITIONSLIDERSECTION_H
