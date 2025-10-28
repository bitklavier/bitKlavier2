//
// Created by Myra Norton on 9/22/25.
//

#ifndef WAVEDISTUNDERTOWPARAMS_H
#define WAVEDISTUNDERTOWPARAMS_H
#include <PreparationStateImpl.h>
struct WaveDistUndertowParams : chowdsp::ParamHolder
{
    WaveDistUndertowParams() : chowdsp::ParamHolder("WAVE")
    {
        add(waveDistanceParam, undertowParam);
    }

    chowdsp::FloatParameter::Ptr waveDistanceParam {
        juce::ParameterID { "wavedistance", 100 },
        "Wave Distance",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 20000.0f, 7500.0f),
        0.0f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };

    chowdsp::FloatParameter::Ptr undertowParam {
        juce::ParameterID { "undertow", 100 },
        "Undertow",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 20000.0f, 7500.0f),
        0.0f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };

    juce::Array<int> displaySliderPositions;




    void processStateChanges() override
    {
        stateChanges.changeState.clear();
    }
};
#endif //WAVEDISTUNDERTOWPARAMS_H
