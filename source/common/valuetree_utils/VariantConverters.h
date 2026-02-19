//
// Created by Davis Polito on 9/23/24.
//

#ifndef BITKLAVIER2_VARIANTCONVERTERS_H
#define BITKLAVIER2_VARIANTCONVERTERS_H
#include <juce_audio_processors/juce_audio_processors.h>
namespace juce {
    template<>
    struct juce::VariantConverter<juce::AudioProcessorGraph::NodeID> final
    {
        using Type = juce::String;

        static juce::AudioProcessorGraph::NodeID fromVar(const juce::var& v)
        {
            return juce::AudioProcessorGraph::NodeID (static_cast<uint32_t> (static_cast<int64> (v)));
        }

        static juce::var toVar(const juce::AudioProcessorGraph::NodeID& id)
        {
            return juce::String(id.uid);
        }
    };
}

#endif //BITKLAVIER2_VARIANTCONVERTERS_H
