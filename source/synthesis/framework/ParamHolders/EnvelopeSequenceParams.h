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
        add(currentlyEditing,
            envelope0,
            envelope1,
            envelope2,
            envelope3,
            envelope4,
            envelope5,
            envelope6,
            envelope7,
            envelope8,
            envelope9,
            envelope10,
            envelope11
        );
    }

    EnvelopeSequenceState envStates;

    chowdsp::FloatParameter::Ptr currentlyEditing {
        juce::ParameterID { "currentlyEditing", 100 },
        "env to edit",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 12.f, 6.f, 1.f),
        0.f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };

    chowdsp::BoolParameter::Ptr envelope0 {
        juce::ParameterID { "envelope0", 100},
        "0",
        true
    };

    chowdsp::BoolParameter::Ptr envelope1 {
        juce::ParameterID { "envelope1", 100},
        "1",
        false
    };

    chowdsp::BoolParameter::Ptr envelope2 {
        juce::ParameterID { "envelope2", 100},
        "2",
        false
    };

    chowdsp::BoolParameter::Ptr envelope3 {
        juce::ParameterID { "envelope3", 100},
        "3",
        false
    };

    chowdsp::BoolParameter::Ptr envelope4 {
        juce::ParameterID { "envelope4", 100},
        "4",
        false
    };

    chowdsp::BoolParameter::Ptr envelope5 {
        juce::ParameterID { "envelope5", 100},
        "5",
        false
    };

    chowdsp::BoolParameter::Ptr envelope6 {
        juce::ParameterID { "envelope6", 100},
        "6",
        false
    };

    chowdsp::BoolParameter::Ptr envelope7 {
        juce::ParameterID { "envelope7", 100},
        "7",
        false
    };

    chowdsp::BoolParameter::Ptr envelope8 {
        juce::ParameterID { "envelope8", 100},
        "8",
        false
    };

    chowdsp::BoolParameter::Ptr envelope9 {
        juce::ParameterID { "envelope9", 100},
        "9",
        false
    };

    chowdsp::BoolParameter::Ptr envelope10 {
        juce::ParameterID { "envelope10", 100},
        "10",
        false
    };

    chowdsp::BoolParameter::Ptr envelope11 {
        juce::ParameterID { "envelope11", 100},
        "11",
        false
    };
};

#endif //BITKLAVIER0_ENVELOPESEQUENCEPARAMS_H
