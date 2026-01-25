//
// Created by Davis Polito on 7/15/25.
//
#include "ModConnectionsList.h"

#include "chowdsp_listeners/chowdsp_listeners.h"

namespace bitklavier {
    void ModConnectionList::deleteObject(ModConnection *c) {
        synth.removeConnection(c->connection);
        listeners_.call([&](Listener& l){ l.removeModConnection(c); });
    }

    void ModConnectionList::newObjectAdded(ModConnection *c) {
        // Flat layout only: connect per-connection nodes directly.
        if (c->state.hasType(IDs::ModulationConnection))
            synth.connectModulation(c->state);
        if (c->state.hasType(IDs::TUNINGCONNECTION))
            synth.connectTuning(c->state);
        if (c->state.hasType(IDs::RESETCONNECTION))
            synth.connectReset(c->state);
        if (c->state.hasType (IDs::TEMPOCONNECTION))
            synth.connectTempo(c->state);
        if (c->state.hasType (IDs::SYNCHRONICCONNECTION))
            synth.connectSynchronic(c->state);
        listeners_.call([&](Listener& l){ l.modConnectionAdded(c); });
    }

    // void ModConnectionList::newObjectAdded(ModConnection *c) {
    //    // synth.addModConnection(c->ModConnection);
    //     for (const auto& v : c->state) {
    //         if (v.hasType(IDs::ModulationConnection)) {
    //             synth.connectModulation(v);
    //         }
    //     }
    //     if (c->state.hasType(IDs::TUNINGCONNECTION))
    //         synth.connectTuning(c->state);
    //     // }
    //     if (c->state.hasType(IDs::RESETCONNECTION))
    //     {
    //         // Only keep/show a reset connection if the backend accepted it.
    //         // If connectReset returns false (e.g., Reset -> Tuning), remove the just-added tree and skip UI notification.
    //         if (! synth.connectReset(c->state))
    //         {
    //             DBG ("ModConnectionList: rejected RESETCONNECTION, removing from tree");
    //             removeChild (c->state, &synth.getUndoManager());
    //             return;
    //         }
    //     }
    //     if (c->state.hasType (IDs::TEMPOCONNECTION))
    //         synth.connectTempo(c->state);
    //     if (c->state.hasType (IDs::SYNCHRONICCONNECTION))
    //         synth.connectSynchronic(c->state);
    //     listeners_.call([&](Listener& l){ l.modConnectionAdded(c); });
    // }

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
        listeners_.call([](Listener& l){ l.modConnectionListChanged(); });
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
        // Don't iterate the ValueTree while removing from it – that can invalidate
        // the iterator and lead to use-after-free inside JUCE's ValueTree iterator.
        // Instead, find the index first, then remove by index.
        const int index = this->parent.indexOf (child);
        if (index >= 0)
        {
            undoManager->beginNewTransaction();
            this->parent.removeChild (index, undoManager);
        }
    }

    // void ModConnectionList::removeChild (juce::ValueTree& child, juce::UndoManager* undoManager)
    // {
    //     for(const auto& _child: this->parent) {
    //         if(_child == child) {
    //             undoManager->beginNewTransaction();
    //             this->parent.removeChild(child,undoManager);
    //         }
    //     }
    // }
}
