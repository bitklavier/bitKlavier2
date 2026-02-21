//
// Created by Davis Polito on 7/15/25.
//

#ifndef CONNECTIONSLIST_H
#define CONNECTIONSLIST_H
#include <juce_audio_processors/juce_audio_processors.h>
// For juce::ListenerList
#include <juce_core/juce_core.h>
#include "Identifiers.h"
#include "tracktion_ValueTreeUtilities.h"
#include "synth_base.h"
#include "synth_gui_interface.h"
#include "FullInterface.h"
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
            rebuildObjects();
            for (auto object: objects) {
                ConnectionList::newObjectAdded(object);
            }
        }
        ~ConnectionList()
        {
            freeObjects();
            listeners_.clear();
        }
        class Listener {
        public:
            virtual ~Listener() {}
            virtual void connectionListChanged() = 0;
            virtual void connectionAdded(Connection*) = 0;
            virtual void removeConnection(Connection*) = 0;
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
            friend class ConnectionList;
            Subscription(ConnectionList* list, Listener* l) : list_(list), listener_(l) {}
            void moveFrom(Subscription& other) {
                list_ = other.list_;
                listener_ = other.listener_;
                other.list_ = nullptr;
                other.listener_ = nullptr;
            }
            ConnectionList* list_ { nullptr };
            Listener* listener_ { nullptr };
        };
        bool isSuitableType (const juce::ValueTree& v) const override
        {
            return v.hasType (IDs::CONNECTION) || v.hasType(IDs::MODCONNECTION);
        }
        void addListener (Listener* l) { listeners_.add(l); }
        void removeListener (Listener* l) { listeners_.remove(l); }
        void clearListeners() { listeners_.clear(); }
        // Preferred: RAII subscribe
        Subscription subscribe(Listener* l) { addListener(l); return Subscription(this, l); }
        Connection* createNewObject(const juce::ValueTree&) override;
        void deleteObject (Connection*) override;
        void newObjectAdded (Connection*) override;
        void objectRemoved (Connection*) override {};     //resized(); }
        void objectOrderChanged() override              { }//resized(); }
        void valueTreeParentChanged (juce::ValueTree&) override;
        void valueTreeRedirected (juce::ValueTree&) override ;
        void valueTreePropertyChanged (juce::ValueTree& v, const juce::Identifier& i) override;

        void deleteAllGui() {
            auto interface = synth.getGuiInterface() != nullptr ? synth.getGuiInterface() : nullptr;
            auto full = interface != nullptr ? interface->getGui() : nullptr;

            if (full != nullptr && full->open_gl_.context.isAttached() && full->open_gl_.context.isActive())
            {
                full->open_gl_.context.executeOnGLThread([this](juce::OpenGLContext &) {
                    for (auto obj: objects)
                        listeners_.call([&](Listener& l){ l.removeConnection(obj); });
                }, true);
            }
            else
            {
                for (auto obj: objects)
                    listeners_.call([&](Listener& l){ l.removeConnection(obj); });
            }
        }
        void rebuildAllGui() {
            for (auto obj: objects)
                listeners_.call([&](Listener& l){ l.connectionAdded(obj); });
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



#endif //CONNECTIONSLIST_H
