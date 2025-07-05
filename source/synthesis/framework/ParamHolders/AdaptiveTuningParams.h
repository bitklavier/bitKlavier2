//
// Created by Dan Trueman on 7/4/25.
//

#ifndef BITKLAVIER0_ADAPTIVETUNINGPARAMS_H
#define BITKLAVIER0_ADAPTIVETUNINGPARAMS_H
#include <PreparationStateImpl.h>
#include <chowdsp_plugin_utils/chowdsp_plugin_utils.h>
#include "tuning_systems.h"
#include "utils.h"

struct AdaptiveTuningParams : public chowdsp::ParamHolder
{
    AdaptiveTuningParams() : chowdsp::ParamHolder("ADAPTIVETUNING")
    {
        add(
            tAdaptiveIntervalScale,
            tAdaptiveAnchorScale,
            tAdaptiveAnchorFundamental,
            tAdaptiveInversional,
            tAdaptiveClusterThresh,
            tAdaptiveHistory,
            tReset,
            tCurrentAdaptiveFundamental);
    }

    /**
     * tAdaptiveIntervalScale = scale to use to determine successive interval tuning
     */
    chowdsp::EnumChoiceParameter<TuningSystem>::Ptr tAdaptiveIntervalScale {
        juce::ParameterID { "tAdaptiveIntervalScale", 100 },
        "tAdaptiveIntervalScale",
        TuningSystem::Partial,
        std::initializer_list<std::pair<char, char>> { { '_', ' ' }, { '1', '/' }, { '2', '-' }, { '3', '\'' } }
    };

    /**
     * tAdaptiveAnchorScale = scale to tune new fundamentals to when in anchored
     */
    chowdsp::EnumChoiceParameter<TuningSystem>::Ptr tAdaptiveAnchorScale {
        juce::ParameterID { "tAdaptiveAnchorScale", 100 },
        "tAdaptiveAnchorScale",
        TuningSystem::Equal_Temperament,
        std::initializer_list<std::pair<char, char>> { { '_', ' ' }, { '1', '/' }, { '2', '-' }, { '3', '\'' } }
    };

    /**
     * tAdaptiveAnchorFundamental = fundamental for anchor scale
     */
    chowdsp::EnumChoiceParameter<PitchClass>::Ptr tAdaptiveAnchorFundamental {
        juce::ParameterID { "tAdaptiveAnchorFundamental", 100 },
        "tAdaptiveAnchorFundamental",
        PitchClass::C,
        std::initializer_list<std::pair<char, char>> { { '_', ' ' }, { '1', '/' }, { '2', '-' }, { '3', '\'' }, { '4', '#' }, { '5', 'b' } }
    };

    /**
     * tAdaptiveInversional = treat the tuningType scale inversionally?
     *      - when toggled on, downward intervals are treated like vertical intervals in the scale
     *          so, for Partial tuning, going down a whole-step would be treated like a 9/8 (not a 8/7) when this is true
     */
    chowdsp::BoolParameter::Ptr tAdaptiveInversional {
        juce::ParameterID { "tAdaptiveInversional", 100},
        "on_off",
        false
    };

    /**
     * tAdaptiveClusterThresh = ms; max time before fundamental is reset
     */
    chowdsp::TimeMsParameter::Ptr tAdaptiveClusterThresh {
        juce::ParameterID { "tAdaptiveClusterThresh", 100 },
        "tAdaptiveClusterThresh",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 1000.0f, 500.0f),
        0.0f,
        true
    };

    /**
     * tAdaptiveHistory = max number of notes before fundamental is reset
     */
    chowdsp::FloatParameter::Ptr tAdaptiveHistory {
        juce::ParameterID { "tAdaptiveHistory", 100 },
        "tAdaptiveHistory",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 8.0f, 4.0f),
        0.0f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };

    /**
     * to catch presses of the reset button, for TuningState::adaptiveReset()
     */
    chowdsp::BoolParameter::Ptr tReset {
        juce::ParameterID { "tReset", 100},
        "reset",
        false
    };

    /**
     * tCurrentAdaptiveFundamental = current adaptive fundamental, for display
     */
    chowdsp::FloatParameter::Ptr tCurrentAdaptiveFundamental {
        juce::ParameterID { "tCurrentAdaptiveFundamental", 100 },
        "tCurrentAdaptiveFundamental",
            chowdsp::ParamUtils::createNormalisableRange (0.0f, 12.0f, 6.0f),
            0.0f,
            &chowdsp::ParamUtils::floatValToString,
            &chowdsp::ParamUtils::stringToFloatVal
    };

    juce::String tCurrentAdaptiveFundamental_string;
};

#endif //BITKLAVIER0_ADAPTIVETUNINGPARAMS_H
