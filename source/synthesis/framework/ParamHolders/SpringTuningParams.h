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
            scaleId,                        // menu: interval-spring-length tuning system/scale
            intervalFundamental,            // menu: fundamental for scaleId; note that this using the enum Fundamental, which is NOT ints, so can't just be cast to an actual fundamental [0-11]
            scaleId_tether,                 // menu: tether-spring-location tuning system — where the anchor/tether points are located
            tetherFundamental,              // menu: fundamental for scaleId_tether (Fundamental)
            active,                         // bool: system is on. hide from user, just needed for saving/loading
            fundamentalSetsTether,          // bool: does the intervalFundamental affect the weights for the tethers springs? hide for now, keep always true
            rate,                           // float: update rate for Timer running spring dynamics (Hz)
            drag,                           // float: drag on the system (or 1 - drag)
            intervalStiffness,              // float: relative stiffness of the interval springs
            tetherStiffness,                // float: relative stiffness of tether springs
            tetherWeightGlobal,             // float: fundamental weight; if fundamentalSetsTether, then the fundamental tether spring is weighted by this
            tetherWeightSecondaryGlobal,    // float: other weights; if fundamentalSetsTether, then all the non-fundamental tether springs are weighted by this
            tCurrentSpringTuningFundamental,// PitchClass: for keeping track of the current fundamental, given by automatic/last/first, etc... for UI
            intervalWeight_1,               // float: weight for m2 (1) interval springs
            intervalWeight_2,               // float: weight for M2 (2) interval springs
            intervalWeight_3,               // etc...
            intervalWeight_4,
            intervalWeight_5,
            intervalWeight_6,
            intervalWeight_7,
            intervalWeight_8,
            intervalWeight_9,
            intervalWeight_10,
            intervalWeight_11,
            intervalWeight_12,
            useLocalOrFundamental_1,        // toggles for set springMode for each interval weight (Local or Fundamental)
            useLocalOrFundamental_2,
            useLocalOrFundamental_3,
            useLocalOrFundamental_4,
            useLocalOrFundamental_5,
            useLocalOrFundamental_6,
            useLocalOrFundamental_7,
            useLocalOrFundamental_8,
            useLocalOrFundamental_9,
            useLocalOrFundamental_10,
            useLocalOrFundamental_11,
            useLocalOrFundamental_12
            );
    }

    /**
     * active = springs are active if true
     */
     /**
      * todo: probably should be false by default, so the timer doesn't start
      */
    chowdsp::BoolParameter::Ptr active {
        juce::ParameterID { "active", 100},
        "active",
        true
    };

    /**
     * scaleId = scale to use to determine resting spring lengths
     */
    chowdsp::EnumChoiceParameter<TuningSystem>::Ptr scaleId {
        juce::ParameterID { "scaleId", 100 },
        "Interval Tuning",
        TuningSystem::Just,
        std::initializer_list<std::pair<char, char>> { { '_', ' ' }, { '1', '/' }, { '2', '-' }, { '3', '\'' } }
    };

    /**
      * intervalFundamental determines which fundamental to use for determining non-unique JI intervals
      *     - for instance, if the fundamental is C, the C-D whole-step will be 9/8, and the D-E whole-step will be 10/9
      *     - the PitchClass enum also includes other options
      *         - none      => the system will always choose the most basic option (9/8, for instance, in the above example),
      *                         so non-unique interval options go away
  *             - lowest    => the lowest pitch in the current sounding cluster will be the fundamental
      *         - highest   => the highest pitch in the current sounding cluster will be the fundamental
      *         - last      => the most recent pitch played in the current sounding cluster will be the fundamental
      *         - automatic => the fundamental is chosen based on an analysis of the current sounding cluster
      *                         this analysis is based on the notion of the phantom fundamental
      *                         described in our 2020 Computer Music Journal article
      */
    chowdsp::EnumChoiceParameter<Fundamental>::Ptr intervalFundamental {
        juce::ParameterID { "intervalFundamental", 100 },
        "Interval Fundamental",
        Fundamental::automatic,
        std::initializer_list<std::pair<char, char>> { { '_', ' ' }, { '1', '/' }, { '2', '-' }, { '3', '\'' }, { '4', '#' }, { '5', 'b' } }
    };

    /**
     * scaleId_tether = scale to use to determine the placement of the tether/anchor positions
     *      - usually ET
     */
    chowdsp::EnumChoiceParameter<TuningSystem>::Ptr scaleId_tether {
        juce::ParameterID { "scaleIdTether", 100 },
        "Anchor Tuning",
        TuningSystem::Equal_Temperament,
        std::initializer_list<std::pair<char, char>> { { '_', ' ' }, { '1', '/' }, { '2', '-' }, { '3', '\'' } }
    };

    /**
     * tetherFundamental = sets the fundamental for the anchor scale.
     *      - the tether/anchor scale is most commonly ET, so tetherFundamental usually doesn't matter
     *      - this is type PitchClass, since the additions in Fundamental (automatic, for instance) are not relevant
     */
    chowdsp::EnumChoiceParameter<PitchClass>::Ptr tetherFundamental {
        juce::ParameterID { "tetherFundamental", 100 },
        "Anchor Fundamental",
        PitchClass::C,
        std::initializer_list<std::pair<char, char>> { { '_', ' ' }, { '1', '/' }, { '2', '-' }, { '3', '\'' }, { '4', '#' }, { '5', 'b' } }
    };

    /**
     *  literally how often does the clock run updating the system, in Hz
     */
    chowdsp::FloatParameter::Ptr rate {
        juce::ParameterID { "rate", 100 },
        "RATE",
        chowdsp::ParamUtils::createNormalisableRange (5.0f, 400.0f, 100.0f),
        100.0f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal,
        true
    };

    /**
     * sets the friction on the spring system; higher drag (actually represented by a low number) will reduce oscillations
     *  - from the original bK code: "actually 1 - drag; drag of 1 => no drag, drag of 0 => infinite drag"
     *      right: so in old bK, the UI shows a high drag value doing what you expect, but sends 1 - drag to the system
     *          so, this default value should be very low, for high drag
     *          need to make sure this is handled properly on the UI side, or perhaps in SpringTuning::setDrag, do the 1 - there?
     *  - also, may want the center much higher, or a log slider in some way?
     */
    chowdsp::FloatParameter::Ptr drag {
        juce::ParameterID { "drag", 100 },
        "DRAG",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 1.0f, 0.5f),
        0.98f, // high drag
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal,
        true
    };

    /**
     * stiffness of the tether springs
     */
    chowdsp::FloatParameter::Ptr tetherStiffness {
        juce::ParameterID { "tetherStiffness", 100 },
        "TETHER STIFF",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 1.0f, 0.5f),
        0.5f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal,
        true
    };

    /**
     * stiffness of the interval springs
     */
    chowdsp::FloatParameter::Ptr intervalStiffness {
        juce::ParameterID { "intervalStiffness", 100 },
        "INTERVAL STIFF",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 1.0f, 0.5f),
        0.5f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal,
        true
    };

    /**
     * when true, the fundamental will be used to set the relative tether weights
     *  - the fundamental will have a weight set by tetherWeightGlobal
     *  - the rest of the pitches will have weights set by tetherWeightSecondaryGlobal
     *
     *  this is super handy, so we don't have to set all the weights individually
     *  - in fact, it may be smart to remove the old functionality, with a slider for EVERY pitch,
     *      since it really isn't practical.
     */
    chowdsp::BoolParameter::Ptr fundamentalSetsTether {
        juce::ParameterID { "fundamentalSetsTether", 100},
        "fundamentalSetsTether",
        true
    };

    /**
     * sets the weight of the tether spring for the fundamental, if fundamentalSetsTether = true
     *  - this will generally be higher than tetherWeightSecondaryGlobal, to keep
     *      the fundamental more anchored, and allow the rest of the pitches
     *      to be tuned more closely to that fundamental
     */
    chowdsp::FloatParameter::Ptr tetherWeightGlobal {
        juce::ParameterID { "tetherWeightGlobal", 100 },
        "FUND WEIGHTS",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 1.0f, 0.5f),
        0.5f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal,
        true
    };

    /**
     * sets the weights of all non-fundamental pitches, if fundamentalSetsTether = true
     */
    chowdsp::FloatParameter::Ptr tetherWeightSecondaryGlobal {
        juce::ParameterID { "tetherWeightSecondaryGlobal", 100 },
        "OTHER WEIGHTS",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 1.0f, 0.5f),
        0.1f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal,
        true
    };

    chowdsp::EnumChoiceParameter<PitchClass>::Ptr tCurrentSpringTuningFundamental {
        juce::ParameterID { "tCurrentSpringTuningFundamental", 100 },
        "tCurrentSpringTuningFundamental",
        PitchClass::C,
        std::initializer_list<std::pair<char, char>> { { '_', ' ' }, { '1', '/' }, { '2', '-' }, { '3', '\'' }, { '4', '#' }, { '5', 'b' } }
    };


     /**
      * todo:
      *     - figure out where:
      *         - the individual interval params are, for setting relative weights of various intervals,
      *         - and toggle buttons for whether to use F(undamental) or L(ocal) (non-unique) interval types
      *         interval weights seem to be held in the "springWeights" array, which isn't "Moddable" in the old code, so I missed it here
*             perhaps ignore for now? this should be addable pretty easily later, especially if we keep them non-moddable.
      *
      */

    chowdsp::FloatParameter::Ptr intervalWeight_1 { // minor 2nd
        juce::ParameterID { "intervalWeight1", 100 },
        "m2",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 1.0f, 0.5f),
        0.5f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal,
        true
    };

    chowdsp::FloatParameter::Ptr intervalWeight_2 { // major 2nd
        juce::ParameterID { "intervalWeight2", 100 },
        "M2",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 1.0f, 0.5f),
        0.5f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal,
        true
    };

    chowdsp::FloatParameter::Ptr intervalWeight_3 { // minor 3
        juce::ParameterID { "intervalWeight3", 100 },
        "m3",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 1.0f, 0.5f),
        0.5f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal,
        true
    };

    chowdsp::FloatParameter::Ptr intervalWeight_4 { // major 3
        juce::ParameterID { "intervalWeight4", 100 },
        "M3",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 1.0f, 0.5f),
        0.5f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal,
        true
    };

    chowdsp::FloatParameter::Ptr intervalWeight_5 { // perfect 4
        juce::ParameterID { "intervalWeight5", 100 },
        "P4",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 1.0f, 0.5f),
        0.5f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal,
        true
    };

    chowdsp::FloatParameter::Ptr intervalWeight_6 { // tritone
        juce::ParameterID { "intervalWeight6", 100 },
        "TT",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 1.0f, 0.5f),
        0.5f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal,
        true
    };

    chowdsp::FloatParameter::Ptr intervalWeight_7 { // perfect 5
        juce::ParameterID { "intervalWeight7", 100 },
        "P5",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 1.0f, 0.5f),
        0.5f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal,
        true
    };

    chowdsp::FloatParameter::Ptr intervalWeight_8 { // minor 6
        juce::ParameterID { "intervalWeight8", 100 },
        "m6",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 1.0f, 0.5f),
        0.5f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal,
        true
    };

    chowdsp::FloatParameter::Ptr intervalWeight_9 { // major 6
        juce::ParameterID { "intervalWeight9", 100 },
        "M6",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 1.0f, 0.5f),
        0.5f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal,
        true
    };

    chowdsp::FloatParameter::Ptr intervalWeight_10 { // minor 7
        juce::ParameterID { "intervalWeight10", 100 },
        "m7",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 1.0f, 0.5f),
        0.5f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal,
        true
    };

    chowdsp::FloatParameter::Ptr intervalWeight_11 { // major 7
        juce::ParameterID { "intervalWeight11", 100 },
        "M7",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 1.0f, 0.5f),
        0.5f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal,
        true
    };

    chowdsp::FloatParameter::Ptr intervalWeight_12 { // octave
        juce::ParameterID { "intervalWeight12", 100 },
        "P8",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 1.0f, 0.5f),
        0.5f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal,
        true
    };

    chowdsp::BoolParameter::Ptr useLocalOrFundamental_1 { // button for setting springMode for each interval weight
        juce::ParameterID { "useLocalOrFundamental1", 100},
        "F",
        true
    };

    chowdsp::BoolParameter::Ptr useLocalOrFundamental_2 { // button for setting springMode for each interval weight
        juce::ParameterID { "useLocalOrFundamental2", 100},
        "F",
        true
    };

    chowdsp::BoolParameter::Ptr useLocalOrFundamental_3 { // button for setting springMode for each interval weight
        juce::ParameterID { "useLocalOrFundamental3", 100},
        "F",
        true
    };

    chowdsp::BoolParameter::Ptr useLocalOrFundamental_4 { // button for setting springMode for each interval weight
        juce::ParameterID { "useLocalOrFundamental4", 100},
        "F",
        true
    };

    chowdsp::BoolParameter::Ptr useLocalOrFundamental_5 { // button for setting springMode for each interval weight
        juce::ParameterID { "useLocalOrFundamental5", 100},
        "F",
        true
    };

    chowdsp::BoolParameter::Ptr useLocalOrFundamental_6 { // button for setting springMode for each interval weight
        juce::ParameterID { "useLocalOrFundamental6", 100},
        "F",
        true
    };

    chowdsp::BoolParameter::Ptr useLocalOrFundamental_7 { // button for setting springMode for each interval weight
        juce::ParameterID { "useLocalOrFundamental7", 100},
        "F",
        false
    };

    chowdsp::BoolParameter::Ptr useLocalOrFundamental_8 { // button for setting springMode for each interval weight
        juce::ParameterID { "useLocalOrFundamental8", 100},
        "F",
        true
    };

    chowdsp::BoolParameter::Ptr useLocalOrFundamental_9 { // button for setting springMode for each interval weight
        juce::ParameterID { "useLocalOrFundamental9", 100},
        "F",
        true
    };

    chowdsp::BoolParameter::Ptr useLocalOrFundamental_10 { // button for setting springMode for each interval weight
        juce::ParameterID { "useLocalOrFundamental10", 100},
        "F",
        true
    };

    chowdsp::BoolParameter::Ptr useLocalOrFundamental_11 { // button for setting springMode for each interval weight
        juce::ParameterID { "useLocalOrFundamental11", 100},
        "F",
        true
    };

    chowdsp::BoolParameter::Ptr useLocalOrFundamental_12 { // button for setting springMode for each interval weight
        juce::ParameterID { "useLocalOrFundamental12", 100},
        "F",
        true
    };

    juce::String tCurrentSpringFundamental_string;
};

#endif //BITKLAVIER0_SPRINGTUNINGPARAMS_H
