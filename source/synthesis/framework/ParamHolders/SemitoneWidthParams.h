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
        add(semitoneWidthSliderParam, reffundamental, octave);
        //add(semitoneWidthSliderParam, reffundamental);
    }

    chowdsp::FloatParameter::Ptr semitoneWidthSliderParam {
        juce::ParameterID { "semitonewidth", 100 },
        "Semitone Width",
        chowdsp::ParamUtils::createNormalisableRange (-100.0f, 200.0f, 100.0f),
        100.0f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };

    chowdsp::EnumChoiceParameter<Fundamental>::Ptr reffundamental {
        juce::ParameterID { "reffundamental", 100 },
        "RefFundamental",
        Fundamental::C,
        std::initializer_list<std::pair<char, char>> { { '_', ' ' }, { '1', '/' }, { '2', '-' }, { '3', '\'' }, { '4', '#' }, { '5', 'b' } }
    };

    /**
     * need to figure out how to make this one hold a set of integers, 0 to 8
     */
    chowdsp::EnumChoiceParameter<Octave>::Ptr octave {
        juce::ParameterID{"octave", 100},
        "Octave",
        Octave::_4 // _ is subbed by a space
    };

};

#endif //BITKLAVIER0_SEMITONEWIDTHPARAMS_H
