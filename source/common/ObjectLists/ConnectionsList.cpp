//
// Created by Davis Polito on 7/15/25.
//
#include "ConnectionsList.h"

#include "chowdsp_listeners/chowdsp_listeners.h"

namespace bitklavier {
    void ConnectionList::deleteObject(Connection *c) {
        synth.removeConnection(c->connection);
        for (auto listener: listeners_) {
            listener->removeConnection(c);
        }
    }

    void ConnectionList::newObjectAdded(Connection *c) {
        synth.addConnection(c->connection);
        for (auto listener: listeners_) {
            listener->connectionAdded(c);
        }
    }

    Connection *ConnectionList::createNewObject(const juce::ValueTree &v) {
        auto* connection = new Connection(v);
        return connection;
    }

    void ConnectionList::valueTreeRedirected(juce::ValueTree &) {
        deleteAllObjects();
        rebuildObjects();
        for(auto object : objects)
        {
            newObjectAdded(object);
        }
        for (auto listener : listeners_) {
            listener->connectionListChanged();
        }
    }

    void ConnectionList::valueTreeParentChanged(juce::ValueTree &) {
    }

    void ConnectionList::valueTreePropertyChanged(juce::ValueTree &v, const juce::Identifier &i) {

    }
    void ConnectionList::appendChild (const juce::ValueTree& child, juce::UndoManager* undoManager)
    {
        undoManager->beginNewTransaction();
        this->parent.appendChild(child,undoManager);

    }

    void ConnectionList::removeChild (juce::ValueTree& child, juce::UndoManager* undoManager)
    {
        undoManager->beginNewTransaction();
        this->parent.removeChild(child,undoManager);
    }
}
