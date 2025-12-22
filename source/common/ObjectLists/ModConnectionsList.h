//
// Created by Davis Polito on 7/15/25.
//

#ifndef ModConnectionSLIST_H
#define ModConnectionSLIST_H
#include <juce_audio_processors/juce_audio_processors.h>
#include "Identifiers.h"
#include "tracktion_ValueTreeUtilities.h"
#include "synth_base.h"
namespace bitklavier {
    class ModConnection {
    public:
        ModConnection(const juce::ValueTree &v) : state(v) {
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
    class ModConnectionList : public tracktion::engine::ValueTreeObjectList<ModConnection> {
    public:
        ModConnectionList(SynthBase& parent, const juce::ValueTree& v) : ValueTreeObjectList(v),synth(parent) {
        jassert(v.hasType(IDs::MODCONNECTIONS));
            rebuildObjects();
            for (auto object: objects) {
                ModConnectionList::newObjectAdded(object);
            }
        }
        ~ModConnectionList() {deleteAllGui();
            freeObjects();}
        class Listener {
        public:
            virtual ~Listener() {}
            virtual void modConnectionListChanged() = 0;
            virtual void modConnectionAdded(ModConnection*) = 0;
            virtual void removeModConnection(ModConnection*) = 0;
        };
        bool isSuitableType (const juce::ValueTree& v) const override
        {
            return v.hasType(IDs::RESETCONNECTION) || v.hasType(IDs::MODCONNECTION) || v.hasType(IDs::TUNINGCONNECTION) || v.hasType (IDs::TEMPOCONNECTION) || v.hasType (IDs::SYNCHRONICCONNECTION);
        }
        void addListener (Listener* l) { listeners_.push_back (l); }
        void clearListeners(){listeners_.clear();}

        void removeListener (Listener* l) {std::erase(listeners_,l);}
        ModConnection* createNewObject(const juce::ValueTree&) override;
        void deleteObject (ModConnection*) override;
        void newObjectAdded (ModConnection*) override;
        void objectRemoved (ModConnection*) override {};     //resized(); }
        void objectOrderChanged() override              { }//resized(); }
        void valueTreeParentChanged (juce::ValueTree&) override;
        void valueTreeRedirected (juce::ValueTree&) override ;
        void valueTreePropertyChanged (juce::ValueTree& v, const juce::Identifier& i) override;

        void deleteAllGui() {
            for (auto obj: objects)
                for (auto listener: listeners_)
                    listener->removeModConnection(obj) ;
        }
        void rebuildAllGui() {
            for (auto obj: objects)
                for (auto listener: listeners_)
                    listener->modConnectionAdded(obj);
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



#endif //ModConnectionSLIST_H
