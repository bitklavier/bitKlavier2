//
// Created by Davis Polito on 7/15/25.
//
#include "ModConnectionsList.h"

#include "chowdsp_listeners/chowdsp_listeners.h"

namespace bitklavier {
    void ModConnectionList::deleteObject(ModConnection *c) {
        synth.removeConnection(c->connection);
        for (auto listener: listeners_) {
            listener->removeModConnection(c);
        }
    }

    void ModConnectionList::newObjectAdded(ModConnection *c) {
       // synth.addModConnection(c->ModConnection);
        for (const auto& v : c->state) {
            if (v.hasType(IDs::ModulationConnection)) {
                synth.connectModulation(v);
            }
        }
        if (c->state.hasType(IDs::TUNINGCONNECTION))
            synth.connectTuning(c->state);
        // }
        if (c->state.hasType(IDs::RESETCONNECTION))
            synth.connectReset(c->state);
        for (auto listener: listeners_) {
            listener->modConnectionAdded(c);
        } 
    }

    ModConnection *ModConnectionList::createNewObject(const juce::ValueTree &v) {
        auto* modConnection = new ModConnection(v);
        return modConnection;
    }

    void ModConnectionList::valueTreeRedirected(juce::ValueTree &) {
        deleteAllObjects();
        rebuildObjects();
        for(auto object : objects)
        {
            newObjectAdded(object);
        }
        for (auto listener : listeners_) {
            listener->modConnectionListChanged();
        }
    }

    void ModConnectionList::valueTreeParentChanged(juce::ValueTree &) {
    }

    void ModConnectionList::valueTreePropertyChanged(juce::ValueTree &v, const juce::Identifier &i) {

    }
    void ModConnectionList::appendChild (const juce::ValueTree& child, juce::UndoManager* undoManager)
    {
        undoManager->beginNewTransaction();
        this->parent.appendChild(child,undoManager);

    }

    void ModConnectionList::removeChild (juce::ValueTree& child, juce::UndoManager* undoManager)
    {
        undoManager->beginNewTransaction();
        this->parent.removeChild(child,undoManager);
    }
}
