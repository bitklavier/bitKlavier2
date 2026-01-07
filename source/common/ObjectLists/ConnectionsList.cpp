//
// Created by Davis Polito on 7/15/25.
//
#include "ConnectionsList.h"

#include "chowdsp_listeners/chowdsp_listeners.h"

namespace bitklavier {
    void ConnectionList::deleteObject(Connection *c) {
        synth.removeConnection(c->connection);
        listeners_.call([&](Listener& l){ l.removeConnection(c); });
    }

    void ConnectionList::newObjectAdded(Connection *c) {
        synth.addConnection(c->connection);
        listeners_.call([&](Listener& l){ l.connectionAdded(c); });
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
        listeners_.call([](Listener& l){ l.connectionListChanged(); });
    }

    void ConnectionList::valueTreeParentChanged(juce::ValueTree &) {
    }

    void ConnectionList::valueTreePropertyChanged(juce::ValueTree &v, const juce::Identifier &i) {

    }
    void ConnectionList::appendChild (const juce::ValueTree& child, juce::UndoManager* undoManager)
    {
        if (undoManager != nullptr) undoManager->beginNewTransaction();
        this->parent.appendChild(child,undoManager);
    }

    void ConnectionList::removeChild (juce::ValueTree& child, juce::UndoManager* undoManager)
    {
        if (undoManager != nullptr) undoManager->beginNewTransaction();
        this->parent.removeChild(child,undoManager);
    }
}
