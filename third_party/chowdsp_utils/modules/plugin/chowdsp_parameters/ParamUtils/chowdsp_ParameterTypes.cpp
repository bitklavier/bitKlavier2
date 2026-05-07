#include "chowdsp_ParameterTypes.h"

namespace chowdsp
{
FloatParameter::FloatParameter (const ParameterID& parameterID,
                                const juce::String& parameterName,
                                const juce::NormalisableRange<float>& valueRange,
                                float defaultFloatValue,
                                const std::function<juce::String (float)>& valueToTextFunction,
                                std::function<float (const juce::String&)>&& textToValueFunction, bool supportsModulation, const juce::ValueTree& v)
#if JUCE_VERSION < 0x070000
    : juce::AudioParameterFloat (
        parameterID,
        parameterName,
        valueRange,
        defaultFloatValue,
        juce::String(),
        AudioProcessorParameter::genericParameter,
        valueToTextFunction == nullptr
            ? std::function<juce::String (float v, int)>()
            : [valueToTextFunction] (float v, int)
            { return valueToTextFunction (v); },
        std::move (textToValueFunction)),
#else
    : juce::AudioParameterFloat (
        parameterID,
        parameterName,
        valueRange,
        defaultFloatValue,
        juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction (
                [valueToTextFunction] (float v, int)
                { return valueToTextFunction (v); })
            .withValueFromStringFunction (std::move (textToValueFunction))),
#endif
      unsnappedDefault (valueRange.convertTo0to1 (defaultFloatValue)),
//      normalisableRange (valueRange),
      supportsModulation(supportsModulation),modulatable_param(v)
{
    myStringFromValFunction = valueToTextFunction;
    myValueFromStringFunction = textToValueFunction;

}
//TODO : update modulation to look at / store entire buffer coming from processblock
//could implement as float* or AudioBuffer&/*
void FloatParameter::applyMonophonicModulation (double modulationValue)
{
    modulationAmount = (float) modulationValue;
}
//TODO : update getCurrentValue to take index of currentsample when accessing parameter.
// this would index into a modulationAmount array or AudioBuffer
float FloatParameter::getCurrentValue() const noexcept
{
    return range.convertFrom0to1 (juce::jlimit (0.0f, 1.0f, range.convertTo0to1 (get()) + modulationAmount));
}

} // namespace chowdsp
