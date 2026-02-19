#ifndef VALUETREE_IDS_REMAPPER_H
#define VALUETREE_IDS_REMAPPER_H

#include <juce_data_structures/juce_data_structures.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "Identifiers.h"
#include <map>

namespace bitklavier
{
    /**
     * Deeply remaps UUIDs and NodeIDs within a ValueTree based on provided maps.
     * Also handles remapping of compound UUID strings in src/dest properties.
     */
    static inline void deepRemapIDs (juce::ValueTree& vt,
                                     std::map<juce::String, juce::String>& uuidMap,
                                     const std::map<juce::AudioProcessorGraph::NodeID, juce::AudioProcessorGraph::NodeID>& nodeIdMap)
    {
        // 1) Remap uuid property
        if (vt.hasProperty (IDs::uuid))
        {
            auto oldUuid = vt.getProperty (IDs::uuid).toString();
            if (auto it = uuidMap.find (oldUuid); it != uuidMap.end())
            {
                vt.setProperty (IDs::uuid, it->second, nullptr);
            }
            else if (vt.hasType(IDs::modulationproc) || vt.hasType(IDs::ModulationConnection))
            {
                auto newUuid = juce::Uuid().toString();
                uuidMap[oldUuid] = newUuid;
                vt.setProperty (IDs::uuid, newUuid, nullptr);
            }
        }

        // 2) Remap nodeID property
        if (vt.hasProperty (IDs::nodeID))
        {
            auto oldNodeId = juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar (vt.getProperty (IDs::nodeID));
            if (auto it = nodeIdMap.find (oldNodeId); it != nodeIdMap.end())
                vt.setProperty (IDs::nodeID, juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::toVar (it->second), nullptr);
        }

        // 3) Remap src/dest properties
        if (vt.hasProperty (IDs::src))
        {
            auto srcVar = vt.getProperty (IDs::src);
            auto oldId = juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar (srcVar);
            if (auto it = nodeIdMap.find (oldId); it != nodeIdMap.end()) // It's a remapped NodeID
            {
                vt.setProperty (IDs::src, juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::toVar (it->second), nullptr);
            }
            else if (srcVar.isString()) // Potentially a UUID-based compound string
            {
                auto s = srcVar.toString();
                for (auto const& [oldU, newU] : uuidMap)
                    if (s.contains (oldU))
                        s = s.replace (oldU, newU);
                vt.setProperty (IDs::src, s, nullptr);
            }
        }

        if (vt.hasProperty (IDs::dest))
        {
            auto destVar = vt.getProperty (IDs::dest);
            auto oldId = juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar (destVar);
            if (auto it = nodeIdMap.find (oldId); it != nodeIdMap.end()) // It's a remapped NodeID
            {
                vt.setProperty (IDs::dest, juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::toVar (it->second), nullptr);
            }
            else if (destVar.isString()) // Potentially a UUID-based compound string
            {
                auto d = destVar.toString();
                for (auto const& [oldU, newU] : uuidMap)
                    if (d.contains (oldU))
                        d = d.replace (oldU, newU);
                vt.setProperty (IDs::dest, d, nullptr);
            }
        }

        // 4) Recursively process children
        for (int i = 0; i < vt.getNumChildren(); ++i)
        {
            auto child = vt.getChild (i);
            deepRemapIDs (child, uuidMap, nodeIdMap);
        }
    }
}

#endif // VALUETREE_IDS_REMAPPER_H
