//
// Created by Davis Polito on 7/15/25.
//

#ifndef CONNECTIONSLIST_H
#define CONNECTIONSLIST_H
#include <juce_audio_processors/juce_audio_processors.h>
#include "Identifiers.h"
#include "tracktion_ValueTreeUtilities.h"
#include "synth_base.h"
namespace bitklavier {
    class Connection {
    public:
        Connection(const juce::ValueTree &v) : state(v) {
            connection.source = {
                juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar(v.getProperty(IDs::src)),
                v.getProperty(IDs::srcIdx)
            };
            connection.destination = {
                juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar(v.getProperty(IDs::dest)),
                v.getProperty(IDs::destIdx)
            };
            src_id.referTo(state, IDs::src, nullptr);
            dest_id.referTo(state, IDs::dest, nullptr);
        }

        juce::AudioProcessorGraph::Connection connection;
        juce::ValueTree state;
        juce::CachedValue<juce::AudioProcessorGraph::NodeID> src_id;
        juce::CachedValue<juce::AudioProcessorGraph::NodeID> dest_id;
    };
    class ConnectionList : public tracktion::engine::ValueTreeObjectList<Connection> {
    public:
        ConnectionList(SynthBase& parent, const juce::ValueTree& v) : ValueTreeObjectList(v),synth(parent) {
            jassert(v.hasType(IDs::CONNECTIONS) || v.hasType(IDs::MODCONNECTIONS));
        }
        ~ConnectionList() {freeObjects();}
        class Listener {
        public:
            virtual ~Listener() {}
            virtual void connectionListChanged() = 0;
            virtual void connectionAdded(Connection*) = 0;
            virtual void removeConnection(Connection*) = 0;
        };
        bool isSuitableType (const juce::ValueTree& v) const override
        {
            return v.hasType (IDs::CONNECTION) || v.hasType(IDs::MODCONNECTION);
        }
        void addListener (Listener* l) { listeners_.push_back (l); }

        void removeListener (Listener* l) {listeners_.erase(
                    std::remove(listeners_.begin(), listeners_.end(), l),
                    listeners_.end());
        }
        Connection* createNewObject(const juce::ValueTree&) override;
        void deleteObject (Connection*) override;
        void newObjectAdded (Connection*) override;
        void objectRemoved (Connection*) override {};     //resized(); }
        void objectOrderChanged() override              { }//resized(); }
        void valueTreeParentChanged (juce::ValueTree&) override;
        void valueTreeRedirected (juce::ValueTree&) override ;
        void valueTreePropertyChanged (juce::ValueTree& v, const juce::Identifier& i) override;

        void deleteAllGui() {
            for (auto obj: objects)
                for (auto listener: listeners_)
                    listener->removeConnection(obj) ;
        }
        void rebuildAllGui() {
            for (auto obj: objects)
                for (auto listener: listeners_)
                    listener->connectionAdded(obj);
        }
        const juce::ValueTree& getValueTree() const {
            return parent;
        }
        void appendChild (const juce::ValueTree& child, juce::UndoManager* undoManager);
        void removeChild (juce::ValueTree& child, juce::UndoManager* undoManager);
    private:
        std::vector<Listener*> listeners_;
        SynthBase& synth;
    };
}



#endif //CONNECTIONSLIST_H
