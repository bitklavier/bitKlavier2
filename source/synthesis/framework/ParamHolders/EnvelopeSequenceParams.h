//
// Created by Dan Trueman on 8/8/25.
//

#ifndef BITKLAVIER0_ENVELOPESEQUENCEPARAMS_H
#define BITKLAVIER0_ENVELOPESEQUENCEPARAMS_H

#pragma once
#include "EnvelopeSequenceState.h"
#include "EnvParams.h"
#include <PreparationStateImpl.h>
#include <chowdsp_plugin_utils/chowdsp_plugin_utils.h>
#include "utils.h"

struct EnvelopeSequenceParams : public chowdsp::ParamHolder
{
    EnvelopeSequenceParams() : chowdsp::ParamHolder("ENVELOPESEQUENCE")
    {
        add(
            envelope_1,
            envelope_2,
            envelope_3,
            envelope_4,
            envelope_5,
            envelope_6,
            envelope_7,
            envelope_8,
            envelope_9,
            envelope_10,
            envelope_11,
            envelope_12,
            currentlyEditing
        );
    }

    EnvelopeSequenceState envStates;

    chowdsp::FloatParameter::Ptr currentlyEditing {
        juce::ParameterID { "currentlyEditing", 100 },
        "env to edit",
        chowdsp::ParamUtils::createNormalisableRange (1.0f, 12.f, 6.f, 1.f),
        1.f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };

    chowdsp::BoolParameter::Ptr envelope_1 {
        juce::ParameterID { "envelope_1", 100},
        "1",
        true
    };

    chowdsp::BoolParameter::Ptr envelope_2 {
        juce::ParameterID { "envelope_2", 100},
        "2",
        false
    };

    chowdsp::BoolParameter::Ptr envelope_3 {
        juce::ParameterID { "envelope_3", 100},
        "3",
        false
    };

    chowdsp::BoolParameter::Ptr envelope_4 {
        juce::ParameterID { "envelope_4", 100},
        "4",
        false
    };

    chowdsp::BoolParameter::Ptr envelope_5 {
        juce::ParameterID { "envelope_5", 100},
        "5",
        false
    };

    chowdsp::BoolParameter::Ptr envelope_6 {
        juce::ParameterID { "envelope_6", 100},
        "6",
        false
    };

    chowdsp::BoolParameter::Ptr envelope_7 {
        juce::ParameterID { "envelope_7", 100},
        "7",
        false
    };

    chowdsp::BoolParameter::Ptr envelope_8 {
        juce::ParameterID { "envelope_8", 100},
        "8",
        false
    };

    chowdsp::BoolParameter::Ptr envelope_9 {
        juce::ParameterID { "envelope_9", 100},
        "9",
        false
    };

    chowdsp::BoolParameter::Ptr envelope_10 {
        juce::ParameterID { "envelope_10", 100},
        "10",
        false
    };

    chowdsp::BoolParameter::Ptr envelope_11 {
        juce::ParameterID { "envelope_11", 100},
        "11",
        false
    };

    chowdsp::BoolParameter::Ptr envelope_12 {
        juce::ParameterID { "envelope_12", 100},
        "12",
        false
    };
};

#endif //BITKLAVIER0_ENVELOPESEQUENCEPARAMS_H
