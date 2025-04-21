//
// Created by Davis Polito on 11/13/24.
//

#ifndef BITKLAVIER2_TRANSPOSEPARAMS_H
#define BITKLAVIER2_TRANSPOSEPARAMS_H
#include <PreparationStateImpl.h>
#include <chowdsp_plugin_utils/chowdsp_plugin_utils.h>
struct TransposeParams : chowdsp::ParamHolder
{
    TransposeParams() : chowdsp::ParamHolder("TRANSPOSE")
    {
//        add(initialparam);
        add(t0,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,transpositionUsesTuning);

    }
//    std::vector<chowdsp::FloatParameter::Ptr> transposeVec = {chowdsp::FloatParameter::Ptr initialparam(
//    chowdsp::SemitonesParameter::Ptr initialparam {
//            juce::ParameterID{"t0", 100},
//            "t0",
//            chowdsp::ParamUtils::createNormalisableRange(-12.0f,12.0f,0.0f), // FIX
//           0.0f
//    };
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
    int numActive = 1;
    void processStateChanges() override
    {
        auto float_params = getFloatParams();
        for(auto [index, change] : stateChanges.changeState)
        {

            static juce::var nullVar;
            for (int i = 0; i< 11; i++)
            {
                auto str = "t" + juce::String(i);
                auto val = change.getProperty(str);
                if (val == nullVar)
                    break;
                auto& float_param = float_params->at(i);
                float_param.get()->setParameterValue(val);
                //float_params[i].data()->get()->setParameterValue(val);
            }
        }
        stateChanges.changeState.clear();

    }

};
#endif //BITKLAVIER2_TRANSPOSEPARAMS_H
