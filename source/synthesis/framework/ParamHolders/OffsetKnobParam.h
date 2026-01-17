//
// Created by Dan Trueman on 7/15/25.
//

#ifndef BITKLAVIER0_OFFSETKNOBPARAM_H
#define BITKLAVIER0_OFFSETKNOBPARAM_H

#include <PreparationStateImpl.h>

struct OffsetKnobParam : chowdsp::ParamHolder
{
    // Adds the appropriate parameters to the Tuning Processor
    OffsetKnobParam() : chowdsp::ParamHolder ("OFFSET")
    {
        add (offSetSliderParam);
    }

    /**
     * offset of tuning system (cents)
     */
    chowdsp::FloatParameter::Ptr offSetSliderParam {
        juce::ParameterID { "offset", 100 },
        "CENTS",
        chowdsp::ParamUtils::createNormalisableRange (-100.0f, 100.0f, 0.0f),
        0.0f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal,
        true
    };
};

#endif //BITKLAVIER0_OFFSETKNOBPARAM_H
