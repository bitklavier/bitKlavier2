// Copyright (C) 2022-2025 Dan Trueman
// SPDX-License-Identifier: GPL-3.0-or-later

//
// Created by Davis Polito on 6/11/25.
//

#include "PreparationList.h"
#include "DirectProcessor.h"
#include "BlendronicProcessor.h"
#include "KeymapProcessor.h"
#include "ModulationProcessor.h"
#include "PianoSwitchProcessor.h"
#include "CommentProcessor.h"
#include "VSTModulationBridge.h"
#include "../UserPreferences.h"

PreparationList::PreparationList(SynthBase &parent, const juce::ValueTree &v, juce::UndoManager* um) : tracktion::engine::ValueTreeObjectList<
    PluginInstanceWrapper>(v), synth(parent) , um(um){
    jassert(v.hasType(IDs::PREPARATIONS));
      rebuildObjects();
    for (auto object: objects) {
        PreparationList::newObjectAdded(object);
    }
}
bool PreparationList::isSuitableType(const juce::ValueTree& v)const {
    {
        return synth.prepFactory.contains(v.getType().toString().toStdString()) || v.hasType(IDs::vst);
    }
}
PluginInstanceWrapper *PreparationList::createNewObject(const juce::ValueTree &v) {

    juce::AudioProcessor *rawPtr;
    juce::AudioProcessorGraph::Node::Ptr node_ptr;
    juce::ValueTree state = v;

    if (temporary_instance == nullptr && static_cast<int>(state.getProperty(IDs::type)) <
        bitklavier::BKPreparationType::PreparationTypeVST) {
       auto processor = synth.prepFactory.create(v.getType().toString().toStdString(), std::any(std::tie(synth,v, um)));
        rawPtr = processor.get();
        processor->prepareToPlay(synth.getSampleRate(), synth.getBufferSize());
        //looking at ProcessorGraph i actually don't think their is any need to try to wrap this in thread safety because
        //the graph rebuild is inherently gonna trigger some async blocking
        // DBG("add to " + v.getParent().getParent().getProperty(IDs::name).toString());
        // DBG("create new object with uuid" + state.getProperty(IDs::uuid).toString());
        node_ptr = synth.addProcessor(std::move(processor),
                                      juce::AudioProcessorGraph::NodeID(
                                          juce::Uuid(state.getProperty(IDs::uuid).toString()).getTimeLow()));
    } else if (temporary_instance == nullptr && static_cast<int>(state.getProperty(IDs::type)) ==
               bitklavier::BKPreparationType::PreparationTypeVST) {
        juce::String err;

        juce::PluginDescription pd;
        pd.loadFromXml(*v.getChildWithName(IDs::PLUGIN).createXml());

        auto& formatManager = synth.user_prefs->userPreferences->formatManager;

        auto* format = formatManager.getFormat (0); // Default to first format if we can't find by name
        for (auto* f : formatManager.getFormats())
        {
            if (f->getName() == pd.pluginFormatName)
            {
                format = f;
                break;
            }
        }

        if (format != nullptr && format->requiresUnblockedMessageThreadDuringCreation (pd) && ! juce::MessageManager::getInstance()->isThisTheMessageThread())
        {
            juce::WaitableEvent finished;
            juce::MessageManager::callAsync ([&]
            {
                temporary_instance = formatManager.createPluginInstance (pd, synth.getSampleRate(), synth.getBufferSize(), err);
                finished.signal();
            });
            finished.wait (10000);
        }
        else
        {
            temporary_instance = formatManager.createPluginInstance (pd, synth.getSampleRate(), synth.getBufferSize(), err);
        }

        if (temporary_instance == nullptr) {
            auto options = juce::MessageBoxOptions::makeOptionsOk(juce::MessageBoxIconType::WarningIcon,
                                                                  TRANS("Couldn't create plugin"),
                                                                  err);
            //TODO show error
        }
        if (v.getChildWithName("STATE").isValid()) {
            juce::MemoryBlock m;
            m.fromBase64Encoding(v.getChildWithName("STATE").getProperty("base64").toString());
            temporary_instance->setStateInformation(m.getData(), (int) m.getSize());
        }
        // createUuidProperty(state);
        rawPtr = temporary_instance.get();
        temporary_instance->prepareToPlay(synth.getSampleRate(), synth.getBufferSize());

        // ------------------------------------------------------------------
        // Create a VSTModulationBridge for this plugin.
        // The bridge sits upstream of the VST in the graph and applies
        // modulation values from the ModulationProcessor to the VST's params.
        // Its ValueTree is stored as a child of the VST's state ValueTree so
        // that connectModulation can locate it via a recursive search.
        // ------------------------------------------------------------------

        // Retrieve (or assign) a stable bridge UUID stored on the VST state.
        if (!state.hasProperty(IDs::bridgeUuid))
            state.setProperty(IDs::bridgeUuid, juce::Uuid().toString(), nullptr);
        const juce::String bridgeUuidStr = state.getProperty(IDs::bridgeUuid).toString();

        // Get or create the bridge ValueTree as a child of the VST state VT.
        juce::ValueTree bridgeVt = state.getOrCreateChildWithName(IDs::vstbridge, nullptr);
        bridgeVt.setProperty(IDs::uuid, bridgeUuidStr, nullptr);

        const auto bridgeNodeID = juce::AudioProcessorGraph::NodeID(
            juce::Uuid(bridgeUuidStr).getTimeLow());

        auto* rawVSTPtr = temporary_instance.get();
        auto bridge = std::make_unique<VSTModulationBridge>(
            rawVSTPtr, bridgeVt, synth);
        bridge->prepareToPlay(synth.getSampleRate(), synth.getBufferSize());
        bridge->setupModulatableParams();

        auto bridgeNodePtr = synth.addProcessor(std::move(bridge), bridgeNodeID);
        bridgeVt.setProperty(IDs::nodeID,
            juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::toVar(bridgeNodeID),
            nullptr);

        // Add the VST plugin node itself.
        const auto vstNodeID = juce::AudioProcessorGraph::NodeID(
            juce::Uuid(state.getProperty(IDs::uuid).toString()).getTimeLow());
        node_ptr = synth.addProcessor(std::move(temporary_instance), vstNodeID);

        // Enforce Bridge → VST ordering via a MIDI ordering edge so that the
        // bridge's processBlock (which calls param->setValue) always runs before
        // the VST plugin's processBlock within each audio callback.
        if (bridgeNodePtr != nullptr && node_ptr != nullptr)
        {
            juce::AudioProcessorGraph::Connection midiOrder {
                { bridgeNodeID, juce::AudioProcessorGraph::midiChannelIndex },
                { vstNodeID,    juce::AudioProcessorGraph::midiChannelIndex }
            };
            synth.addConnection(midiOrder);
        }
    } else {
        createUuidProperty(state);
        rawPtr = temporary_instance.get();
        temporary_instance->prepareToPlay(synth.getSampleRate(), synth.getBufferSize());

        if (static_cast<int>(state.getProperty(IDs::type)) ==
            bitklavier::BKPreparationType::PreparationTypeVST)
        {
            // Bridge creation for async-loaded VST (new plugin added via UI).
            if (!state.hasProperty(IDs::bridgeUuid))
                state.setProperty(IDs::bridgeUuid, juce::Uuid().toString(), nullptr);
            const juce::String bridgeUuidStr = state.getProperty(IDs::bridgeUuid).toString();

            juce::ValueTree bridgeVt = state.getOrCreateChildWithName(IDs::vstbridge, nullptr);
            bridgeVt.setProperty(IDs::uuid, bridgeUuidStr, nullptr);

            const auto bridgeNodeID = juce::AudioProcessorGraph::NodeID(
                juce::Uuid(bridgeUuidStr).getTimeLow());

            auto* rawVSTPtr = temporary_instance.get();
            auto bridge = std::make_unique<VSTModulationBridge>(rawVSTPtr, bridgeVt, synth);
            bridge->prepareToPlay(synth.getSampleRate(), synth.getBufferSize());
            bridge->setupModulatableParams();

            auto bridgeNodePtr = synth.addProcessor(std::move(bridge), bridgeNodeID);
            bridgeVt.setProperty(IDs::nodeID,
                juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::toVar(bridgeNodeID),
                nullptr);

            const auto vstNodeID = juce::AudioProcessorGraph::NodeID(
                juce::Uuid(state.getProperty(IDs::uuid).toString()).getTimeLow());
            node_ptr = synth.addProcessor(std::move(temporary_instance), vstNodeID);

            if (bridgeNodePtr != nullptr && node_ptr != nullptr)
            {
                juce::AudioProcessorGraph::Connection midiOrder {
                    { bridgeNodeID, juce::AudioProcessorGraph::midiChannelIndex },
                    { vstNodeID,    juce::AudioProcessorGraph::midiChannelIndex }
                };
                synth.addConnection(midiOrder);
            }
        }
        else
        {
            node_ptr = synth.addProcessor(std::move(temporary_instance),
                                          juce::AudioProcessorGraph::NodeID(
                                              juce::Uuid(state.getProperty(IDs::uuid).toString()).getTimeLow()));
        }
    }
    if (node_ptr) {
        node_ptr->properties.set(
            "x", juce::VariantConverter<juce::Point<int> >::fromVar(v.getProperty(IDs::x_y)).getX());
        node_ptr->properties.set(
            "y", juce::VariantConverter<juce::Point<int> >::fromVar(v.getProperty(IDs::x_y)).getY());
        node_ptr->properties.set("type", v.getProperty(IDs::type));
        //sendChangeMessage();
    }

    auto* wrapper = new PluginInstanceWrapper(rawPtr, state, node_ptr->nodeID);

    // If a bridge was created for this VST, store its NodeID for cleanup.
    if (state.hasProperty(IDs::bridgeUuid))
    {
        const juce::String buuid = state.getProperty(IDs::bridgeUuid).toString();
        wrapper->bridgeNodeID = juce::AudioProcessorGraph::NodeID(
            juce::Uuid(buuid).getTimeLow());
    }

    return wrapper;
}

void PreparationList::deleteObject(PluginInstanceWrapper *at) {

    for (auto listener: listeners_)
        listener->removeModule(at);
    // find and delete cables and modulation lines associated with this preparation section
    synth.deleteConnectionsWithId(at->node_id);
    synth.removeProcessor(at->node_id);

    // If a VSTModulationBridge was created for this VST plugin, remove it too.
    if (at->bridgeNodeID.uid != 0)
    {
        synth.deleteConnectionsWithId(at->bridgeNodeID);
        synth.removeProcessor(at->bridgeNodeID);
    }

    pianoSwitchProcessors.erase(std::remove(pianoSwitchProcessors.begin(), pianoSwitchProcessors.end(), at), pianoSwitchProcessors.end());
    delete at;
}

void PreparationList::valueTreeRedirected(juce::ValueTree &) {
    deleteAllObjects();
    rebuildObjects();
    for (auto object: objects) {
        newObjectAdded(object);
    }
}

void PreparationList::newObjectAdded(PluginInstanceWrapper *instance_wrapper) {
    if (auto *piano_switch = dynamic_cast<PianoSwitchProcessor*>(instance_wrapper->proc)) {
        pianoSwitchProcessors.push_back(instance_wrapper);
        prependPianoChangeProcessorToAll(instance_wrapper);
    } else {
        prependAllPianoChangeProcessorsTo(instance_wrapper);
    }

    for (auto listener: listeners_)
        listener->moduleAdded(instance_wrapper);
}

void PreparationList::deleteAllGui() {
    for (auto obj: objects)
        for (auto listener: listeners_)
            listener->removeModule(obj);
}

void PreparationList::rebuildAllGui()
{
    DBG("PreparationList::rebuildAllGui()");

    for (auto obj: objects)
        for (auto listener: listeners_)
            listener->moduleAdded(obj);
    for (const auto & vt : parent) {
        if(vt.getType() == IDs::linkedPrep) {
            auto galleryTop = parent.getRoot();
            /*
             * todo: remove name from this search, so it's not dependent on something the user might change
             */
            auto lpiano = galleryTop.getChildWithProperty(IDs::name, vt.getProperty(IDs::linkedPianoName));
            auto lprep = lpiano.getChildWithName(IDs::PREPARATIONS).getChildWithProperty (IDs::nodeID, vt.getProperty(IDs::nodeID));
            DBG("PreparationList::rebuildAllGui(), found linked prep with nodeID = " << juce::String(lprep.getProperty(IDs::nodeID)));

            // need to find the PluginInstanceWrapper* for this lprep.
            PluginInstanceWrapper* linkedObj = nullptr;
            for (auto& preparationList : synth.preparationLists)
            {
                if (preparationList->getValueTree().getParent() == lpiano)
                {
                    for (auto obj : preparationList->objects)
                    {
                        if (obj->state.getProperty(IDs::nodeID) == lprep.getProperty(IDs::nodeID))
                        {
                            linkedObj = obj;
                            break;
                        }
                    }
                }
                if (linkedObj != nullptr) break;
            }

            if (linkedObj != nullptr)
            {
                DBG("PreparationList::rebuildAllGui(), found linked OBJECT with nodeID = " << juce::String(linkedObj->state.getProperty(IDs::nodeID)));
                for (auto listener : listeners_)
                {
                    // Check if this module is already added to the listener (ConstructionSite)
                    // ConstructionSite implements moduleAdded by creating a new PreparationSection.
                    // If we call it multiple times for the same linkedObj, it might create duplicates.
                    // However, rebuildAllGui is usually called when we need to refresh the WHOLE GUI.
                    listener->moduleAdded(linkedObj);
                }
            }

            // for (auto listener: listeners_)
            //     listener->linkedPiano();
        }
    }
}


void PreparationList::valueTreeParentChanged(juce::ValueTree &) {
}

void PreparationList::addPlugin(const juce::PluginDescription &desc, const juce::ValueTree &v) {
    synth.user_prefs->userPreferences->formatManager.createPluginInstanceAsync(desc,
        synth.getSampleRate(),
        synth.getBufferSize(),
        [this, v](std::unique_ptr<juce::AudioPluginInstance> instance, const juce::String &error) {
            addPluginCallback(std::move(instance), error, v);
        });
}

void PreparationList::addPluginCallback(std::unique_ptr<juce::AudioPluginInstance> instance,
                                        const juce::String &error,
                                        const juce::ValueTree &v) {
    if (instance == nullptr) {
        auto options = juce::MessageBoxOptions::makeOptionsOk(juce::MessageBoxIconType::WarningIcon,
                                                              TRANS("Couldn't create plugin"),
                                                              error);
        //messageBox = juce::AlertWindow::showScopedAsync (options, nullptr);
    } else {
        instance->enableAllBuses();
        temporary_instance = std::move(instance);
        parent.addChild(v, -1, nullptr);
    }
}

void PreparationList::setValueTree(const juce::ValueTree &v) {
    parent = v;
}

//this gets called on creation for any preparation that is not a pianoswitch
void PreparationList::prependAllPianoChangeProcessorsTo(const PluginInstanceWrapper *dst) {
    jassert(dynamic_cast<PianoSwitchProcessor*>(dst->proc)==nullptr);
    if(dynamic_cast<KeymapProcessor*>(dst->proc))
        return;
    if(dynamic_cast<bitklavier::CommentProcessor*>(dst->proc))
        return;
    for (auto& piano_switch_array : synth.preparationLists) {
        for (auto piano_switch : piano_switch_array->pianoSwitchProcessors) {
            // synth.addModulationConnection(piano_switch->node_id, dst->node_id);
            if(!synth.addModulationConnection(piano_switch->node_id,dst->node_id))
                jassertfalse;
        }
    }

}

//this gets called on creation for a piano switch
void PreparationList::prependPianoChangeProcessorToAll(const PluginInstanceWrapper *piano_switch) {
    jassert(dynamic_cast<PianoSwitchProcessor*>(piano_switch->proc));
    for (auto& piano_switch_array : synth.preparationLists) {
        for (auto processor : piano_switch_array->objects) {
            if (dynamic_cast<PianoSwitchProcessor*>(processor->proc) == nullptr && dynamic_cast<KeymapProcessor*>(processor->proc) == nullptr && dynamic_cast<bitklavier::CommentProcessor*>(processor->proc) == nullptr)
                if(!synth.addModulationConnection(piano_switch->node_id,processor->node_id))
                    jassertfalse;
        }
    }

    // During construction, this PreparationList is not yet in synth.preparationLists (the
    // emplace_back that owns us hasn't returned yet), so the loop above misses all same-piano
    // processors.  Add those connections here when the list isn't registered yet.
    bool isAlreadyInList = false;
    for (const auto& pl : synth.preparationLists)
        if (pl.get() == this) { isAlreadyInList = true; break; }

    if (!isAlreadyInList)
    {
        for (auto processor : objects)
        {
            if (processor == piano_switch) continue;
            if (dynamic_cast<PianoSwitchProcessor*>(processor->proc) == nullptr &&
                dynamic_cast<KeymapProcessor*>(processor->proc) == nullptr &&
                dynamic_cast<bitklavier::CommentProcessor*>(processor->proc) == nullptr)
                if (!synth.addModulationConnection(piano_switch->node_id, processor->node_id))
                    jassertfalse;
        }
    }
}

