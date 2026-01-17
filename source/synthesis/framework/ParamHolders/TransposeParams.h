//
// Created by Davis Polito on 11/13/24.
//

#ifndef BITKLAVIER2_TRANSPOSEPARAMS_H
#define BITKLAVIER2_TRANSPOSEPARAMS_H
#include <PreparationStateImpl.h>
#include <chowdsp_plugin_utils/chowdsp_plugin_utils.h>
#include <bitset>

struct TransposeParams : chowdsp::ParamHolder
{
    TransposeParams() : chowdsp::ParamHolder("TRANSPOSE")
    {
        add(t0,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,
            numActiveSliders,
            transpositionUsesTuning);
    }

    chowdsp::SemitonesParameter::Ptr t0{juce::ParameterID{"t0", 100},
                                         "t0",
                                         chowdsp::ParamUtils::createNormalisableRange(-12.0f, 12.0f, 0.0f), // FIX
                                         0.0f};
    chowdsp::SemitonesParameter::Ptr t1{juce::ParameterID{"t1", 100},
                                        "t1",
                                        chowdsp::ParamUtils::createNormalisableRange(-12.0f, 12.0f, 0.0f), // FIX
                                        0.0f};
    chowdsp::SemitonesParameter::Ptr t2{juce::ParameterID{"t2", 100},
                                        "t2",
                                        chowdsp::ParamUtils::createNormalisableRange(-12.0f, 12.0f, 0.0f), // FIX
                                        0.0f};
    chowdsp::SemitonesParameter::Ptr t3{juce::ParameterID{"t3", 100},
                                        "t3",
                                        chowdsp::ParamUtils::createNormalisableRange(-12.0f, 12.0f, 0.0f), // FIX
                                        0.0f};
    chowdsp::SemitonesParameter::Ptr t4{juce::ParameterID{"t4", 100},
                                        "t4",
                                        chowdsp::ParamUtils::createNormalisableRange(-12.0f, 12.0f, 0.0f), // FIX
                                        0.0f};
    chowdsp::SemitonesParameter::Ptr t5{juce::ParameterID{"t5", 100},
                                        "t5",
                                        chowdsp::ParamUtils::createNormalisableRange(-12.0f, 12.0f, 0.0f), // FIX
                                        0.0f};
    chowdsp::SemitonesParameter::Ptr t6{juce::ParameterID{"t6", 100},
                                        "t6",
                                        chowdsp::ParamUtils::createNormalisableRange(-12.0f, 12.0f, 0.0f), // FIX
                                        0.0f};
    chowdsp::SemitonesParameter::Ptr t7{juce::ParameterID{"t7", 100},
                                        "t7",
                                        chowdsp::ParamUtils::createNormalisableRange(-12.0f, 12.0f, 0.0f), // FIX
                                        0.0f};
    chowdsp::SemitonesParameter::Ptr t8{juce::ParameterID{"t8", 100},
                                        "t8",
                                        chowdsp::ParamUtils::createNormalisableRange(-12.0f, 12.0f, 0.0f), // FIX
                                        0.0f};
    chowdsp::SemitonesParameter::Ptr t9{juce::ParameterID{"t9", 100},
                                        "t9",
                                        chowdsp::ParamUtils::createNormalisableRange(-12.0f, 12.0f, 0.0f), // FIX
                                        0.0f};
    chowdsp::SemitonesParameter::Ptr t10{juce::ParameterID{"t10", 100},
                                        "t10",
                                        chowdsp::ParamUtils::createNormalisableRange(-12.0f, 12.0f, 0.0f), // FIX
                                        0.0f};
    chowdsp::SemitonesParameter::Ptr t11{juce::ParameterID{"t11", 100},
                                         "t11",
                                         chowdsp::ParamUtils::createNormalisableRange(-12.0f, 12.0f, 0.0f), // FIX
                                         0.0f};

    // Transposition Uses Tuning param
    chowdsp::BoolParameter::Ptr transpositionUsesTuning {
            juce::ParameterID { "UseTuning", 100 },
            "TranspositionUsesTuning",
            false
    };

    chowdsp::FloatParameter::Ptr numActiveSliders{
        juce::ParameterID{"sliderValsSize", 100},
        "sliderVals_size",
        chowdsp::ParamUtils::createNormalisableRange(0.0f, 12.0f, 6.0f),
        1.f,
        [](float value) -> juce::String {
            return juce::String(value, 0); // No decimals since values are whole numbers
        },
        [](const juce::String& text) -> float {
            return text.getFloatValue();
        }};

    /*
     * processStateChanges() is used to handle state change modifications, including resets. should be called every block
     *
     * state changes are NOT audio-rate/continuous; those are handled by "modulatableParams" in the parameter definitions
     * examples: TranspParams or velocityMinMaxParams
     *
     * 'stateChanges.change' state will have size() != 0 after a mod or reset is triggered. each change
     * is handled here, and then it is cleared until another mod or reset is triggered
     *
     * in this specific case, if there is a changeState in stateChanges (which will only happen when
     * a mod is triggered) we read through the valueTree and update all the modded transpositions (t0, t1, etc...)
     */
    void processStateChanges() override
    {
        transpositionUsesTuning->processStateChanges();

        auto float_params = getFloatParams();

        int i = numActiveSliders->getCurrentValue();
        for(auto [index, change] : stateChanges.changeState)
        {
            static juce::var nullVar;

            for (i = 0; i < 12; i++)
            {
                auto str = "t" + juce::String(i);
                auto val = change.getProperty(str);

                // need to make sure we don't ignore values of 0.!
                if (val == nullVar && !val.equalsWithSameType(0.)) break;

                auto& float_param = float_params->at(i);
                float_param.get()->setParameterValue(val);
            }
            numActiveSliders->setParameterValue(i);
        }
        stateChanges.changeState.clear();
    }
};
#endif //BITKLAVIER2_TRANSPOSEPARAMS_H
