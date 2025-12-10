//
// Created by Myra Norton on 11/25/25.
//

#ifndef BITKLAVIER0_EQFILTERPARAMS_H
#define BITKLAVIER0_EQFILTERPARAMS_H
#include <PreparationStateImpl.h>

struct EQPeakFilterParams : chowdsp::ParamHolder
{

    // gain slider params, for all gain-type knobs
    float rangeStart = -80.0f;
    float rangeEnd = 6.0f;
    float skewFactor = 2.0f;

    EQPeakFilterParams(juce::String idPrepen) : chowdsp::ParamHolder("EQPEAKFILTER"), idPrepend(idPrepen)
    {
      add(filterActive,
          filterFreq,
          filterGain,
          filterQ,
          filterSlope);
    }
    juce::String idPrepend;


    // filter active bool
    chowdsp::BoolParameter::Ptr filterActive {
        juce::ParameterID { idPrepend + "Active", 100 },
        idPrepend + "Active",
        false
    };

    // filter freq param
    chowdsp::FloatParameter::Ptr filterFreq {
        juce::ParameterID { idPrepend + "Freq", 100 },
        idPrepend + "Freq",
        juce::NormalisableRange { 0.0f, 10.00f, 0.0f, skewFactor, false },
        1.0f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal,
        true
    };

    // filter Q param
    chowdsp::FloatParameter::Ptr filterQ {
        juce::ParameterID { idPrepend + "Q", 100 },
        idPrepend + "Q",
        juce::NormalisableRange { 0.0f, 10.00f, 0.0f, skewFactor, false },
        1.0f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal,
        true
    };

    // filter gain param
    chowdsp::GainDBParameter::Ptr filterGain {
        juce::ParameterID { idPrepend + "Gain", 100 },
        idPrepend + "Gain",
        juce::NormalisableRange { rangeStart, rangeEnd, 0.0f, skewFactor, false },
        0.0f,
        true
    };

    // FilterSlope
    chowdsp::FloatParameter::Ptr filterSlope {
        juce::ParameterID { idPrepend + "Slope", 100 },
        idPrepend + "Slope",
        chowdsp::ParamUtils::createNormalisableRange (12.0f, 48.f, 24.f, 12.f),
        12.0f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal,
        true
    };

    void resetToDefault()
    {
        filterFreq->setParameterValue(filterFreq->getDefaultValue());
        filterGain->setParameterValue(filterGain->getDefaultValue());
        filterQ->setParameterValue(filterQ->getDefaultValue());
    }
};

struct EQCutFilterParams : chowdsp::ParamHolder
{

    // gain slider params, for all gain-type knobs
    float rangeStart = -80.0f;
    float rangeEnd = 6.0f;
    float skewFactor = 2.0f;

    EQCutFilterParams(juce::String idPrepen) : chowdsp::ParamHolder("EQFILTER"), idPrepend(idPrepen)
    {
      add(filterActive,
          filterFreq,
          filterSlope);
    }
    juce::String idPrepend;

    // filter active bool
    chowdsp::BoolParameter::Ptr filterActive {
        juce::ParameterID { idPrepend + "Active", 100 },
        idPrepend + "Active",
        false
    };

    // filter freq param
    chowdsp::FloatParameter::Ptr filterFreq {
        juce::ParameterID { idPrepend + "Freq", 100 },
        idPrepend + "Freq",
        juce::NormalisableRange { 0.0f, 10.00f, 0.0f, skewFactor, false },
        1.0f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal,
        true
    };

    // FilterSlope
    chowdsp::FloatParameter::Ptr filterSlope {
        juce::ParameterID { idPrepend + "Slope", 100 },
        idPrepend + "Slope",
        chowdsp::ParamUtils::createNormalisableRange (12.0f, 48.f, 24.f, 12.f),
        12.0f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal,
        true
    };

    void resetToDefault()
    {
        filterFreq->setParameterValue(filterFreq->getDefaultValue());
        filterSlope->setParameterValue(filterSlope->getDefaultValue());
    }
};
#endif //BITKLAVIER0_EQFILTERPARAMS_H
