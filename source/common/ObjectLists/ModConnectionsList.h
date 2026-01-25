//
// Created by Davis Polito on 7/15/25.
//

#ifndef ModConnectionSLIST_H
#define ModConnectionSLIST_H
#include <juce_audio_processors/juce_audio_processors.h>
// For juce::ListenerList
#include <juce_core/juce_core.h>
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
        ~ModConnectionList() {
            deleteAllGui();
            freeObjects();
            listeners_.clear();
        }
        class Listener {
        public:
            virtual ~Listener() {}
            virtual void modConnectionListChanged() = 0;
            virtual void modConnectionAdded(ModConnection*) = 0;
            virtual void removeModConnection(ModConnection*) = 0;
        };
        // RAII subscription token that auto-unsubscribes on destruction/reset
        class Subscription {
        public:
            Subscription() = default;
            ~Subscription() { release(); }
            Subscription(Subscription&& other) noexcept { moveFrom(other); }
            Subscription& operator=(Subscription&& other) noexcept { if (this != &other) { release(); moveFrom(other); } return *this; }
            Subscription(const Subscription&) = delete;
            Subscription& operator=(const Subscription&) = delete;

            void release() {
                if (list_ != nullptr && listener_ != nullptr) {
                    list_->removeListener(listener_);
                }
                list_ = nullptr;
                listener_ = nullptr;
            }
        private:
            friend class ModConnectionList;
            Subscription(ModConnectionList* list, Listener* l) : list_(list), listener_(l) {}
            void moveFrom(Subscription& other) {
                list_ = other.list_;
                listener_ = other.listener_;
                other.list_ = nullptr;
                other.listener_ = nullptr;
            }
            ModConnectionList* list_ { nullptr };
            Listener* listener_ { nullptr };
        };
        bool isSuitableType (const juce::ValueTree& v) const override
        {
            // Include both legacy container nodes (RESET/MOD/TUNING/...) and the flat
            // per-connection nodes (ModulationConnection) so saved state/continuous
            // mods are reconstructed on load.
            return v.hasType (IDs::RESETCONNECTION)
                || v.hasType (IDs::MODCONNECTION)
                || v.hasType (IDs::TUNINGCONNECTION)
                || v.hasType (IDs::TEMPOCONNECTION)
                || v.hasType (IDs::SYNCHRONICCONNECTION)
                || v.hasType (IDs::ModulationConnection);
        }
        void addListener (Listener* l) { listeners_.add(l); }
        void removeListener (Listener* l) { listeners_.remove(l); }
        void clearListeners(){ listeners_.clear(); }
        // Preferred: RAII subscribe
        Subscription subscribe(Listener* l) { addListener(l); return Subscription(this, l); }
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
                listeners_.call([&](Listener& l){ l.removeModConnection(obj); });
        }
        void rebuildAllGui() {
            for (auto obj: objects)
                listeners_.call([&](Listener& l){ l.modConnectionAdded(obj); });
        }
        const juce::ValueTree& getValueTree() const {
            return parent;
        }
        void appendChild (const juce::ValueTree& child, juce::UndoManager* undoManager);
        void removeChild (juce::ValueTree& child, juce::UndoManager* undoManager);
    private:
        juce::ListenerList<Listener> listeners_;
        SynthBase& synth;
    };
}



#endif //ModConnectionSLIST_H
