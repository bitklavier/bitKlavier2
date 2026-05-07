#pragma once

#if JUCE_MODULE_AVAILABLE_chowdsp_clap_extensions
#include <chowdsp_clap_extensions/chowdsp_clap_extensions.h>
#endif
#include "Identifiers.h"

namespace bitklavier
{
struct ParameterChangeBuffer
{
    ParameterChangeBuffer()
    {
        changeState.reserve (10);
    }

    // Lightweight synchronisation for cross-thread access to changeState.
    juce::SpinLock changeLock;
    std::vector<std::pair<int, juce::ValueTree>> changeState = {};
    juce::ValueTree defaultState;

    // Try to push without blocking; drop if contended to keep audio thread safe.
    bool tryPush (int index, const juce::ValueTree& vt)
    {
        juce::SpinLock::ScopedTryLockType lock { changeLock };
        if (! lock.isLocked())
            return false;
        changeState.emplace_back (index, vt);
        return true;
    }
};

// orig
// struct ParameterChangeBuffer
// {
//     ParameterChangeBuffer()
//     {
//         changeState.reserve (10);
//     }
//
//     std::vector<std::pair<int, juce::ValueTree>> changeState = {};
//     juce::ValueTree defaultState;
// };

class StateChangeableParameter
{
public:
    virtual ~StateChangeableParameter() = default;
    virtual void processStateChanges() {}
    bitklavier::ParameterChangeBuffer stateChanges;
    void push_change (std::pair<int, juce::ValueTree>&& x)
    {
        // Use the non-blocking push; if it fails due to contention, drop the event.
        stateChanges.tryPush (x.first, x.second);
    }
};
} // namespace bitklavier
namespace chowdsp
{
#if ! JUCE_MODULE_AVAILABLE_chowdsp_clap_extensions
namespace ParamUtils
{
    /** Mixin for parameters that recognize some form of modulation. */
    struct ModParameterMixin
    {
        virtual ~ModParameterMixin() = default;

        /** Returns true if this parameter supports monophonic modulation. */
        virtual bool supportsMonophonicModulation() { return false; }

        /** Returns true if this parameter supports polyphonic modulation. */
        virtual bool supportsPolyphonicModulation() { return false; }

        /** Base function for applying monophonic modulation to a parameter. */
        [[maybe_unused]] virtual void applyMonophonicModulation (double /*value*/)
        {
        }

        /** Base function for applying polyphonic modulation to a parameter. */
        [[maybe_unused]] virtual void applyPolyphonicModulation (int32_t /*note_id*/, int16_t /*port_index*/, int16_t /*channel*/, int16_t /*key*/, double /*value*/)
        {
        }
    };
} // namespace ParamUtils
#endif

#ifndef DOXYGEN
#if JUCE_VERSION < 0x070000
// JUCE 6 doesn't have the new juce::ParameterID class, so this is a little workaround.
using ParameterID = juce::String;
#else
using ParameterID = juce::ParameterID;
#endif
#endif

/** Wrapper of juce::AudioParameterFloat that supports monophonic modulation. */
class FloatParameter : public juce::AudioParameterFloat,
                       public ParamUtils::ModParameterMixin
{
public:
    FloatParameter (const ParameterID& parameterID,
                    const juce::String& parameterName,
                    const juce::NormalisableRange<float>& valueRange,
                    float defaultValue,
                    const std::function<juce::String (float)>& valueToTextFunction,
                    std::function<float (const juce::String&)>&& textToValueFunction,
                    bool supportsModulation = false,
                    const juce::ValueTree& v = {});

    using Ptr = OptionalPointer<FloatParameter>;

    /**
     * Sets the parameter value.
     * This will result in a call @c setValueNotifyingHost, so make sure that's what you want.
     * Especially if calling this from the audio thread!
     */
    void setParameterValue (float newValue) { AudioParameterFloat::operator= (newValue); }

    /** Returns the default value for the parameter. */
    float getDefaultValue() const override { return unsnappedDefault; }

    /** TRUE! */
    bool supportsMonophonicModulation() override { return supportsModulation; }

    /** Applies monphonic modulation to this parameter. */
    void applyMonophonicModulation (double value) override;

    /** Returns the current parameter value accounting for any modulation that is currently applied. */
    float getCurrentValue() const noexcept;
    int getIntValue() { return static_cast<int> (*this); }

    /** Returns the current parameter value accounting for any modulation that is currently applied. */
    operator float() const noexcept { return getCurrentValue(); } // NOSONAR, NOLINT(google-explicit-constructor): we want to be able to do implicit conversion here
    explicit operator int() const noexcept { return std::round (getCurrentValue()); }
    /** Print debug info. */
    void printDebug() const
    {
        DBG (paramID + " : " + juce::String (get()));
    }
    float getModAmt() const { return convertFrom0to1 (modulationAmount); }
    bitklavier::ParameterChangeBuffer stateChanges;
    std::function<juce::String (float)> getStringFromValueFunction() const
    {
        return myStringFromValFunction;
    }
    std::function<float (const juce::String&)> getValueFromStringFunction() const
    {
        return myValueFromStringFunction;
    }

    void setRangeToValueTree (const juce::ValueTree& vt)
    {
        jassert (vt.isValid());
        modulatable_param = vt;
        float start = static_cast<float> (vt.getProperty (IDs::start));
        float end = static_cast<float> (vt.getProperty (IDs::end));
        float skew = static_cast<float> (vt.getProperty (IDs::skew));
        range = juce::NormalisableRange<float> { start, end, range.interval, skew };
    }
    juce::ValueTree& getModParam()
    {
        return modulatable_param;
    }
    void setParamOffsetIndex (int val)
    {
        param_offset_index = val;
    }
    int getParamOffsetIndex()
    {
        return param_offset_index;
    }

private:
    int param_offset_index = -1;
    juce::ValueTree modulatable_param;
    std::function<juce::String (float)> myStringFromValFunction;
    std::function<float (const juce::String&)> myValueFromStringFunction;
    const float unsnappedDefault;
    //    const juce::NormalisableRange<float> normalisableRange;

    const bool supportsModulation;
    float modulationAmount = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FloatParameter)
};

/** Wrapper of juce::AudioParameterChoice that does not support modulation. */
class ChoiceParameter : public juce::AudioParameterChoice,
                        public ParamUtils::ModParameterMixin,
                        public bitklavier::StateChangeableParameter
{
public:
    ChoiceParameter (const ParameterID& parameterID, const juce::String& parameterName, const juce::StringArray& parameterChoices, int defaultItemIndex)
        : juce::AudioParameterChoice (parameterID, parameterName, parameterChoices, defaultItemIndex),
          defaultChoiceIndex (defaultItemIndex)
    {
    }
    void printDebug() const
    {
        DBG (paramID + " : " + juce::String (getIndex())); // Using getIndex() for ChoiceParameter
    }

    using Ptr = OptionalPointer<ChoiceParameter>;

    /** Returns the default value for the parameter. */
    int getDefaultIndex() const noexcept { return defaultChoiceIndex; }

    /**
     * Sets the parameter value.
     * This will result in a call @c setValueNotifyingHost, so make sure that's what you want.
     * Especially if calling this from the audio thread!
     */
    void setParameterValue (int newValue) { AudioParameterChoice::operator= (newValue); }

    // bitklavier::ParameterChangeBuffer stateChanges;
    void setRangeToValueTree (const juce::ValueTree& vt) {}
    juce::ValueTree& getModParam()
    {
        return modulatable_param;
    }
    void setParamOffsetIndex (int val)
    {
        param_offset_index = val;
    }
    int getParamOffsetIndex()
    {
        return param_offset_index;
    }
    float getCurrentValue() const noexcept { return 0.f; }

private:
    int param_offset_index = -1;
    juce::ValueTree modulatable_param;

    const int defaultChoiceIndex = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChoiceParameter)
};

/**
 * A Choice parameter based off of an enum class type.
 *
 * By default, underscores in enum names will be replaced with spaces.
 * For custom behaviour, replace the charMap argument with a custom
 * character map.
 */
template <typename EnumType>
class EnumChoiceParameter : public ChoiceParameter
{
public:
    // Ideally we could use any fully specified enum, but since there's no way to enforce
    // that, let's stick to "flags" enums.
    static_assert (magic_enum::detail::is_flags_v<EnumType>, "In order to enforce consistent serialization/deserialization, enum types should be constructed as flags.");

    EnumChoiceParameter (const ParameterID& parameterID,
                         const juce::String& parameterName,
                         EnumType defaultChoice,
                         const std::initializer_list<std::pair<char, char>>& charMap = { { '_', ' ' } })
        : ChoiceParameter (
              parameterID,
              parameterName,
              EnumHelpers::createStringArray<EnumType> (charMap),
              static_cast<int> (*magic_enum::enum_index (defaultChoice)))
    {
    }

    EnumType get() const noexcept
    {
        return magic_enum::enum_value<EnumType> ((size_t) getIndex());
    }
    void printDebug() const
    {
        DBG (paramID + " : " + juce::String (static_cast<int> (get())));
    }
    void processStateChanges() override
    {
        for (const auto& [index, change] : stateChanges.changeState)
        {
            auto val = change.getProperty (paramID);
            if (! val.isUndefined())
            {
                int n = val;
                EnumType t = static_cast<EnumType> (1 << n);
                setParameterValue (t);
            }
        }
        stateChanges.changeState.clear();
    }

    /**
     * Sets the parameter value.
     * This will result in a call @c setValueNotifyingHost, so make sure that's what you want.
     * Especially if calling this from the audio thread!
     */
    void setParameterValue (EnumType newValue) { AudioParameterChoice::operator= (static_cast<int> (*magic_enum::enum_index (newValue))); }

    using Ptr = OptionalPointer<EnumChoiceParameter>;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EnumChoiceParameter)
};

/** Wrapper of juce::AudioParameterBool that does not support modulation. */
class BoolParameter : public juce::AudioParameterBool,
                      public ParamUtils::ModParameterMixin,
                      public bitklavier::StateChangeableParameter
{
public:
    BoolParameter (const ParameterID& parameterID, const juce::String& parameterName, bool defaultBoolValue)
        : juce::AudioParameterBool (parameterID, parameterName, defaultBoolValue)
    {
    }
    void printDebug() const
    {
        DBG (paramID + " : " + juce::String (static_cast<int> (get())));
    }
    using Ptr = OptionalPointer<BoolParameter>;

    /**
     * Sets the parameter value.
     * This will result in a call @c setValueNotifyingHost, so make sure that's what you want.
     * Especially if calling this from the audio thread!
     */
    void setParameterValue (bool newValue) { AudioParameterBool::operator= (newValue); }
    void processStateChanges() override
    {
        static juce::var nullVar;
        for (const auto& [index, change] : stateChanges.changeState)
        {
            auto val = change.getProperty (paramID);
            if (val != nullVar)
            {
                setParameterValue (val);
            }
        }
        stateChanges.changeState.clear();
    }
    void setRangeToValueTree (const juce::ValueTree& vt) {}
    juce::ValueTree& getModParam()
    {
        return modulatable_param;
    }
    void setParamOffsetIndex (int val)
    {
        param_offset_index = val;
    }
    int getParamOffsetIndex()
    {
        return param_offset_index;
    }
    float getCurrentValue() const noexcept { return 0.f; }

private:
    int param_offset_index = -1;
    juce::ValueTree modulatable_param;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BoolParameter)
};

/** A float parameter which specifically stores a percentage value. */
class PercentParameter : public FloatParameter
{
public:
    PercentParameter (const ParameterID& parameterID,
                      const juce::String& paramName,
                      float defaultValue = 0.5f,
                      bool isBipolar = false,
                      const juce::ValueTree& v = {})
        : FloatParameter (parameterID,
                          paramName,
                          juce::NormalisableRange { isBipolar ? -1.0f : 0.0f, 1.0f },
                          defaultValue,
                          &ParamUtils::percentValToString,
                          &ParamUtils::stringToPercentVal,
                          true,
                          v)
    {
    }

    using Ptr = OptionalPointer<PercentParameter>;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PercentParameter)
};

/** A float parameter which specifically stores a gain value in Decibels. */
class GainDBParameter : public FloatParameter
{
public:
    GainDBParameter (const ParameterID& parameterID,
                     const juce::String& paramName,
                     const juce::NormalisableRange<float>& paramRange,
                     float defaultValue,
                     bool mod = false,
                     const juce::ValueTree& v = {})
        : FloatParameter (parameterID,
                          paramName,
                          paramRange,
                          defaultValue,
                          &ParamUtils::gainValToString,
                          &ParamUtils::stringToGainVal,
                          true,
                          v)
    {
    }

    using Ptr = OptionalPointer<GainDBParameter>;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GainDBParameter)
};

/** A float parameter which specifically stores a frequency value in Hertz. */
class FreqHzParameter : public FloatParameter
{
public:
    FreqHzParameter (const ParameterID& parameterID,
                     const juce::String& paramName,
                     const juce::NormalisableRange<float>& paramRange,
                     float defaultValue,
                     const juce::ValueTree& v = {})
        : FloatParameter (parameterID,
                          paramName,
                          paramRange,
                          defaultValue,
                          &ParamUtils::freqValToString,
                          &ParamUtils::stringToFreqVal,
                          true,
                          v)
    {
    }

    using Ptr = OptionalPointer<FreqHzParameter>;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FreqHzParameter)
};

/** A float parameter which specifically stores a time value in milliseconds. */
class TimeMsParameter : public FloatParameter
{
public:
    TimeMsParameter (const ParameterID& parameterID,
                     const juce::String& paramName,
                     const juce::NormalisableRange<float>& paramRange,
                     float defaultValue,
                     bool mod = false,
                     const juce::ValueTree& v = {})
        : FloatParameter (parameterID,
                          paramName,
                          paramRange,
                          defaultValue,
                          &ParamUtils::timeMsValToString,
                          &ParamUtils::stringToTimeMsVal,
                          mod,
                          v)
    {
    }

    using Ptr = OptionalPointer<TimeMsParameter>;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TimeMsParameter)
};

/** A float parameter which specifically stores a ratio value. */
class RatioParameter : public FloatParameter
{
public:
    RatioParameter (const ParameterID& parameterID,
                    const juce::String& paramName,
                    const juce::NormalisableRange<float>& paramRange,
                    float defaultValue,
                    const juce::ValueTree& v = {})
        : FloatParameter (parameterID,
                          paramName,
                          paramRange,
                          defaultValue,
                          &ParamUtils::ratioValToString,
                          &ParamUtils::stringToRatioVal,
                          false,
                          v)
    {
    }

    using Ptr = OptionalPointer<RatioParameter>;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RatioParameter)
};

/** A float parameter which specifically stores a semitones value. */

class SemitonesParameter : public FloatParameter
{
public:
    JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wcomma")
    SemitonesParameter (const ParameterID& parameterID,
                        const juce::String& paramName,
                        juce::NormalisableRange<float> paramRange,
                        float defaultValue,
                        bool snapToInt = false,
                        const juce::ValueTree& v = {})
        : FloatParameter (
              parameterID,
              paramName,
              (paramRange.interval = snapToInt ? 1.0f : paramRange.interval, paramRange),
              defaultValue,
              [snapToInt] (float val)
              { return ParamUtils::semitonesValToString (val, snapToInt); },
              &ParamUtils::stringToSemitonesVal,
              false,
              v)
    {
    }
    JUCE_END_IGNORE_WARNINGS_GCC_LIKE

    using Ptr = OptionalPointer<SemitonesParameter>;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SemitonesParameter)
};

} // namespace chowdsp
