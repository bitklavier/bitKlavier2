//
// Created by Davis Polito on 9/20/24.
//

#ifndef BITKLAVIER2_PREPARATIONSTATEIMPL_H
#define BITKLAVIER2_PREPARATIONSTATEIMPL_H
#include "chowdsp_plugin_state/chowdsp_plugin_state.h"
#include "bk_XMLSerializer.h"
namespace bitklavier
{
/**
 * Template type to hold a plugin's state.
 *
 * @tparam ParameterState       Struct containing all of the plugin's parameters as chowdsp::OptionalPointer's.
 * @tparam NonParameterState    Struct containing all of the plugin's non-parameter state as StateValue objects.
 * @tparam Serializer           A type that implements chowdsp::BaseSerializer (JSONSerializer by default)
 */
    template <typename ParameterState, typename NonParameterState = chowdsp::NonParamState, typename Serializer = bitklavier::XMLSerializer>
    class PreparationStateImpl : public chowdsp::PluginState
    {
        static_assert (std::is_base_of_v<chowdsp::ParamHolder, ParameterState>, "ParameterState must be a chowdsp::ParamHolder!");
        static_assert (std::is_base_of_v<chowdsp::NonParamState, NonParameterState>, "NonParameterState must be a chowdsp::NonParamState!");

    public:
        /** Constructs a plugin state with no processor */
        explicit PreparationStateImpl (juce::UndoManager* um = nullptr);

        /** Constructs the state and adds all the state parameters to the given processor */
        explicit PreparationStateImpl (juce::AudioProcessor& processor, juce::UndoManager* um = nullptr);

        /** Serializes the plugin state to the given juce::MemoryBlock */
        void serialize (juce::MemoryBlock& data) const override;

        /** Deserializes the plugin state from the given juce::MemoryBlock */
        void deserialize (const juce::MemoryBlock& data) override;

        /** Serializer */
        template <typename>
        static typename Serializer::SerializedType serialize (const PreparationStateImpl& object);

        /** Deserializer */
        template <typename>
        static void deserialize (typename Serializer::DeserializedType serial, PreparationStateImpl& object);

        /** Returns the plugin non-parameter state */
        [[nodiscard]] chowdsp::NonParamState& getNonParameters() override;

        /** Returns the plugin non-parameter state */
        [[nodiscard]] const chowdsp::NonParamState& getNonParameters() const override;

        ParameterState params;
        NonParameterState nonParams;

    private:
        chowdsp::Version pluginStateVersion {};

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PreparationStateImpl)
    };

} // namespace bitklavier
//#include "PreparationStateImpl.h"
namespace bitklavier
{
    template <typename ParameterState, typename NonParameterState, typename Serializer>
    PreparationStateImpl<ParameterState, NonParameterState, Serializer>::PreparationStateImpl (juce::UndoManager* um)
    {
        initialise (params, nullptr, um);
    }

    template <typename ParameterState, typename NonParameterState, typename Serializer>
    PreparationStateImpl<ParameterState, NonParameterState, Serializer>::PreparationStateImpl (juce::AudioProcessor& processor, juce::UndoManager* um)
    {
        initialise (params, &processor, um);
    }

    template <typename ParameterState, typename NonParameterState, typename Serializer>
    void PreparationStateImpl<ParameterState, NonParameterState, Serializer>::serialize (juce::MemoryBlock& data) const
    {

        chowdsp::Serialization::serialize<Serializer> (*this, data);
    }

    template <typename ParameterState, typename NonParameterState, typename Serializer>
    void PreparationStateImpl<ParameterState, NonParameterState, Serializer>::deserialize (const juce::MemoryBlock& data)
    {
        callOnMainThread (
                [this, data]
                {
                    chowdsp::Serialization::deserialize<Serializer> (data, *this);

                    params.applyVersionStreaming (pluginStateVersion);
                    if (nonParams.versionStreamingCallback != nullptr)
                        nonParams.versionStreamingCallback (pluginStateVersion);

                    getParameterListeners().updateBroadcastersFromMessageThread();

                    if (undoManager != nullptr)
                        undoManager->clearUndoHistory();
                });
    }

/** Serializer */
    template <typename ParameterState, typename NonParameterState, typename Serializer>
    template <typename>
    typename Serializer::SerializedType PreparationStateImpl<ParameterState, NonParameterState, Serializer>::serialize (const PreparationStateImpl& object)
    {
//        auto serial = Serializer::createBaseElement("PROCESSOR");

#if defined JucePlugin_VersionString
//    Serializer::template serialize<Serializer> (currentPluginVersion, serial);
#endif

        //Serializer::addChildElement (serial, Serializer::template serialize<Serializer, chowdsp::NonParamState> (object.nonParams));
        auto serial = Serializer::template serialize<Serializer, ParameterState> (object.params);
        return serial;
    }

/** Deserializer */
    template <typename ParameterState, typename NonParameterState, typename Serializer>
    template <typename>
    void PreparationStateImpl<ParameterState, NonParameterState, Serializer>::deserialize (typename Serializer::DeserializedType serial, PreparationStateImpl& object)
    {
        enum
        {
//#if defined JucePlugin_VersionString
//        versionChildIndex,
//#endif
            nonParamStateChildIndex,
            paramStateChildIndex,
            expectedNumChildElements,
        };

        //Serializer::template deserialize<Serializer, chowdsp::NonParamState> (Serializer::getChildElement (serial, "nonparam"), object.nonParams);
        // if ( Serializer::getChildElement (serial, object.params.getName()))
            Serializer::template deserialize<Serializer, ParameterState> (serial, object.params);
    }

    template <typename ParameterState, typename NonParameterState, typename Serializer>
    chowdsp::NonParamState& PreparationStateImpl<ParameterState, NonParameterState, Serializer>::getNonParameters()
    {
        return nonParams;
    }

    template <typename ParameterState, typename NonParameterState, typename Serializer>
    const chowdsp::NonParamState& PreparationStateImpl<ParameterState, NonParameterState, Serializer>::getNonParameters() const
    {
        return nonParams;
    }
} // namespace chowds
//#include "PreparationStateImpl.cpp"

#endif //BITKLAVIER2_PREPARATIONSTATEIMPL_H
