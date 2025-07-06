//
// Created by Dan Trueman on 7/5/25.
//

#ifndef BITKLAVIER0_SPRINGTUNINGPARAMS_H
#define BITKLAVIER0_SPRINGTUNINGPARAMS_H

#include <PreparationStateImpl.h>
#include <chowdsp_plugin_utils/chowdsp_plugin_utils.h>
#include "tuning_systems.h"
#include "utils.h"

struct SpringTuningParams : public chowdsp::ParamHolder
{
    SpringTuningParams() : chowdsp::ParamHolder("SPRINGTUNING")
    {
        add(
            scaleId,
            active,
            fundamentalSetsTether,
            rate,
            stiffness,
            tetherStiffness,
            intervalStiffness,
            drag,
            tetherWeightGlobal,
            tetherWeightSecondaryGlobal,
            intervalFundamental);
    }

    /**
     * todo: check the proper ranges for all of these
     */

    /**
     * scaleId = scale to use to determine resting spring lengths
     */
    chowdsp::EnumChoiceParameter<TuningSystem>::Ptr scaleId {
        juce::ParameterID { "scaleId", 100 },
        "scaleId",
        TuningSystem::Just,
        std::initializer_list<std::pair<char, char>> { { '_', ' ' }, { '1', '/' }, { '2', '-' }, { '3', '\'' } }
    };

    /**
     * springs are active?
     */
    chowdsp::BoolParameter::Ptr active {
        juce::ParameterID { "active", 100},
        "active",
        true
    };

    /**
     * when true, the fundamental will be used to set tether weights
     */
    chowdsp::BoolParameter::Ptr fundamentalSetsTether {
        juce::ParameterID { "fundamentalSetsTether", 100},
        "fundamentalSetsTether",
        false
    };

    /**
     *
     */
    chowdsp::FloatParameter::Ptr rate {
        juce::ParameterID { "rate", 100 },
        "rate",
        chowdsp::ParamUtils::createNormalisableRange (5.0f, 400.0f, 100.0f),
        100.0f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };

    /**
     *
     */
    chowdsp::FloatParameter::Ptr stiffness {
        juce::ParameterID { "stiffness", 100 },
        "stiffness",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 1.0f, 0.5f),
        0.5f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };

    /**
     *
     */
    chowdsp::FloatParameter::Ptr tetherStiffness {
        juce::ParameterID { "tetherStiffness", 100 },
        "tetherStiffness",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 1.0f, 0.5f),
        0.5f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };

    /**
     *
     */
    chowdsp::FloatParameter::Ptr intervalStiffness {
        juce::ParameterID { "intervalStiffness", 100 },
        "intervalStiffness",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 1.0f, 0.5f),
        0.5f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };

    /**
     * actually 1 - drag; drag of 1 => no drag, drag of 0 => infinite drag
     */
    chowdsp::FloatParameter::Ptr drag {
        juce::ParameterID { "drag", 100 },
        "drag",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 1.0f, 0.5f),
        0.97f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };

    /**
     *
     */
    chowdsp::FloatParameter::Ptr tetherWeightGlobal {
        juce::ParameterID { "tetherWeightGlobal", 100 },
        "tetherWeightGlobal",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 1.0f, 0.5f),
        0.8f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };

    /**
     *
     */
    chowdsp::FloatParameter::Ptr tetherWeightSecondaryGlobal {
        juce::ParameterID { "tetherWeightSecondaryGlobal", 100 },
        "tetherWeightSecondaryGlobal",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 1.0f, 0.5f),
        0.2f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };

     /**
      * intervalFundamental
      */
     chowdsp::EnumChoiceParameter<PitchClass>::Ptr intervalFundamental {
         juce::ParameterID { "intervalFundamental", 100 },
         "intervalFundamental",
         PitchClass::C,
         std::initializer_list<std::pair<char, char>> { { '_', ' ' }, { '1', '/' }, { '2', '-' }, { '3', '\'' }, { '4', '#' }, { '5', 'b' } }
     };

    juce::String tCurrentSpringFundamental_string;
};

#endif //BITKLAVIER0_SPRINGTUNINGPARAMS_H
