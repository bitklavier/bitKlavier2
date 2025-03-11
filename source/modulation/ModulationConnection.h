//
// Created by Davis Polito on 12/10/24.
//

#ifndef BITKLAVIER_MODULATIONCONNECTION_H
#define BITKLAVIER_MODULATIONCONNECTION_H
#include <juce_data_structures/juce_data_structures.h>
#include "Identifiers.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <chowdsp_parameters/chowdsp_parameters.h>
#include "StateModulator.h"
class ModulatorBase;
namespace bitklavier {
class ModulationProcessor;
    struct ModulationConnection {
        ModulationConnection(const std::string& from, const std::string& to, int index)
            : source_name(from), destination_name(to),state(IDs::ModulationConnection)
        {

            createUuidProperty(state);
            uuid = state.getProperty(IDs::uuid);
            index_in_all_mods = index;
            scalingValue_ = 0.5f;
            setDestination(to);
            setSource(from);

        }
        ~ModulationConnection()
        {
            //count--;
        }
        static bool isModulationSourceDefaultBipolar(const std::string& source);
        void setSource(const std::string& from)
        {
            state.setProperty(IDs::src, juce::String(from), nullptr);
        }

        void setDestination(const std::string& uuid_to)
        {
            state.setProperty(IDs::dest, juce::String(uuid_to), nullptr);
        }

        void setModulationAmount(float amt)
        {
            state.setProperty(IDs::modAmt, amt, nullptr);
        }
        void setPolarity(bool isBipolar)
        {
            state.setProperty(IDs::isBipolar, isBipolar, nullptr);
        }
        void resetConnection(const std::string& from, const std::string& to) {
            source_name = from;
            destination_name = to;
            setSource(source_name);
            setDestination(destination_name);
            if (source_name == "" || destination_name == "")
            {
                state.getParent().removeChild(state,nullptr);
            }
        }

        float getCurrentBaseValue()
        {

                return scalingValue_.load();
        }
        void setScalingValue(float val)
        {
                if (isBipolar())
                    scalingValue_.store(val *0.5f);
                else
                    scalingValue_.store(val);

            setModulationAmount(val);
        }

        void setBypass(bool bypass) {}
        void setStereo(bool stereo) {}
        bool isBipolar() const { return bipolar_; }
        bool isBypass() const {return bypass_; }
        bool isStereo() const {return stereo_; }
        bool setDefaultBipolar (bool val)
        {
            defaultBipolar = val;
            setBipolar(val);
        }
        void setBipolar(bool bipolar) {
            bipolar_ = bipolar;
        }

        std::string source_name;
        std::string destination_name;        //must be named state to be picked up by valuetreeobjectlist - dont know
        // if i'll be using this for that or not
        juce::ValueTree state;
        int index_in_all_mods;
        int index_in_mapping;
        juce::Uuid uuid;
        bool bipolar_;
        bool bypass_;
        bool stereo_;
        bool defaultBipolar;
        juce::AudioProcessorGraph::Connection connection_;
        ModulationProcessor* parent_processor;
        ModulatorBase* processor;
        int modulation_output_bus_index;
    private:
        std::atomic<float> scalingValue_;
//        std::atomic<float>* bipolarOffset;



    };


        typedef struct mapping_change
        {
            bool disconnecting;
            ModulationConnection* connection;
            std::string destination;
            std::string source;
            int dest_param_index;
            int source_param_uuid;
        }mapping_change;


    class ModulationConnectionBank {
    public:
        ModulationConnectionBank();
        ~ModulationConnectionBank();
        ModulationConnection* createConnection(const std::string& from, const std::string& to);

        ModulationConnection* atIndex(int index) { return all_connections_[index].get(); }
        size_t numConnections() { return all_connections_.size(); }

    private:

        std::vector<std::unique_ptr<ModulationConnection>> all_connections_;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulationConnectionBank)
    };

struct StateConnection : public ModulatorBase::Listener{
        StateConnection(const std::string& from, const std::string& to, int index)
                : source_name(from), destination_name(to),state(IDs::ModulationConnection)
        {
            createUuidProperty(state);
            uuid = state.getProperty(IDs::uuid);
            index_in_all_mods = index;
            scalingValue_ = 1.f;
            setSource(source_name);
            setDestination(destination_name);

        }
        ~StateConnection()
        {
            //count--;
        }
        void setSource(const std::string& from)
        {
            state.setProperty(IDs::src, juce::String(from), nullptr);
        }

        void setDestination(const std::string& uuid_to)
        {
            state.setProperty(IDs::dest, juce::String(uuid_to), nullptr);
        }
        void setModulationAmount(float amt)
        {
            state.setProperty(IDs::modAmt, amt, nullptr);
        }
        void setChangeBuffer(bitklavier::ParameterChangeBuffer* buf) {
            changeBuffer = buf;
        }
        void resetConnection(const std::string& from, const std::string& to) {
            source_name = from;
            destination_name = to;
            setSource(source_name);
            setDestination(destination_name);
            if (source_name == "" || destination_name == "")
            {
                state.getParent().removeChild(state,nullptr);
            }
            changeBuffer = nullptr;
        }

        float getCurrentBaseValue()
        {
            return scalingValue_.load();
        }
        void setScalingValue(float val)
        {
                scalingValue_.store(val);
            DBG(juce::String(val));
        }
        void modulationTriggered()
        {
            changeBuffer->changeState.push_back(std::pair<int,juce::ValueTree>(0,change));
        }

        std::string source_name;
        std::string destination_name;        //must be named state to be picked up by valuetreeobjectlist - dont know
        // if i'll be using this for that or not
        juce::ValueTree state;
        juce::ValueTree change;
        int index_in_all_mods;
        int index_in_mapping;
        juce::Uuid uuid;
        bool defaultBipolar;
        juce::AudioProcessorGraph::Connection connection_;
        ModulationProcessor* parent_processor;
        ModulatorBase* processor;
        int modulation_output_bus_index;
        bitklavier::ParameterChangeBuffer* changeBuffer = nullptr;
    private:
        std::atomic<float> scalingValue_;
//        std::atomic<float>* bipolarOffset;



    };


    class StateConnectionBank {
    public:
        StateConnectionBank();
        ~StateConnectionBank();
        StateConnection* createConnection(const std::string& from, const std::string& to);

        StateConnection* atIndex(int index) { return all_connections_[index].get(); }
        size_t numConnections() { return all_connections_.size(); }
        void addParam(std::pair<std::string,bitklavier::ParameterChangeBuffer*>&&);

    private:
        std::map<std::string,bitklavier::ParameterChangeBuffer*> parameter_map;
        std::vector<std::unique_ptr<StateConnection>> all_connections_;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StateConnectionBank)
    };
}

#endif //BITKLAVIER_MODULATIONCONNECTION_H
