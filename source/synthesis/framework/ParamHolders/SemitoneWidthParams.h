//
// Created by Dan Trueman on 6/28/25.
//

#ifndef BITKLAVIER0_SEMITONEWIDTHPARAMS_H
#define BITKLAVIER0_SEMITONEWIDTHPARAMS_H

#include <PreparationStateImpl.h>
#include <chowdsp_plugin_utils/chowdsp_plugin_utils.h>
#include "utils.h"

struct SemitoneWidthParams : chowdsp::ParamHolder
{
    SemitoneWidthParams() : chowdsp::ParamHolder("SEMITONEWIDTH")
    {
        add(semitoneWidthSliderParam, fundamental);
    }

    chowdsp::FloatParameter::Ptr semitoneWidthSliderParam {
        juce::ParameterID { "semitonewidth", 100 },
        "Semitone Width",
        chowdsp::ParamUtils::createNormalisableRange (-100.0f, 200.0f, 0.0f),
        0.0f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };

    chowdsp::EnumChoiceParameter<Fundamental>::Ptr fundamental {
        juce::ParameterID { "fundamental", 100 },
        "Fundamental",
        Fundamental::C,
        std::initializer_list<std::pair<char, char>> { { '_', ' ' }, { '1', '/' }, { '2', '-' }, { '3', '\'' }, { '4', '#' }, { '5', 'b' } }
    };

};

#endif //BITKLAVIER0_SEMITONEWIDTHPARAMS_H
