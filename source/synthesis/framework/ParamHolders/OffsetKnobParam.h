//
// Created by Dan Trueman on 7/15/25.
//

#ifndef BITKLAVIER0_OFFSETKNOBPARAM_H
#define BITKLAVIER0_OFFSETKNOBPARAM_H

#include <PreparationStateImpl.h>
#include <chowdsp_plugin_utils/chowdsp_plugin_utils.h>

struct OffsetKnobParam : chowdsp::ParamHolder
{
    // Adds the appropriate parameters to the Tuning Processor
    OffsetKnobParam() : chowdsp::ParamHolder ("offSet")
    {
        add (offSet);
    }

    /**
     * offset of tuning system (cents)
     */
    chowdsp::FloatParameter::Ptr offSet {
        juce::ParameterID { "offSet", 100 },
        "Offset",
        chowdsp::ParamUtils::createNormalisableRange (-100.0f, 100.0f, 0.0f),
        0.0f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal,
        true
    };
};

#endif //BITKLAVIER0_OFFSETKNOBPARAM_H
