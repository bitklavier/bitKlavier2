//
// Created by Davis Polito on 1/30/25.
//

#include "ModulationProcessor.h"
#include "ModulatorBase.h"
#include "ModulationConnection.h"
void bitklavier::ModulationProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    buffer.clear();


    for (auto msg : midiMessages)
    {
        if (msg.getMessage().isNoteOn())
        {
            for (auto mod : modulators_)
                mod->triggerModulation();

        }

    }
    for (int i = 0; i <modulators_.size();i++)
    {

        if (modulators_[i] == nullptr)
            continue;
        //process the modulation into a scratch buffer.
        modulators_[i]->getNextAudioBlock(tmp_buffers[i], midiMessages);
        if (modulators_[i]->type == ModulatorType::AUDIO)
        {
            for (auto connection : mod_routing[i].mod_connections)
            {
                buffer.copyFrom(connection->modulation_output_bus_index, 0,tmp_buffers[i].getReadPointer(0,0),buffer.getNumSamples(), connection->getCurrentBaseValue());
            }
        }

        //mod_routing[i]
        //output the modulation to the correct output buffers with scaling
    }
    //melatonin::printSparkline(buffer);
}

void bitklavier::ModulationProcessor::addModulator(ModulatorBase* mod) {
//update to search for any null locations
    mod->parent_ = this;
    modulators_.push_back(mod);
    tmp_buffers.emplace_back(1,blockSize_);
    mod_routing.emplace_back();
}

void bitklavier::ModulationProcessor::removeModulator(ModulatorBase* mod) {
    auto it  = std::find(modulators_.begin(), modulators_.end(), mod);
    if(it != modulators_.end()) {
        int index = std::distance(modulators_.begin(), it);
        //set to null so that deallocation doesn't happen on audio thread
        modulators_[index] = nullptr;
        tmp_buffers[index] = {};
        mod_routing[index] = {};
    }
}

void bitklavier::ModulationProcessor::addModulationConnection(ModulationConnection* connection){
    all_modulation_connections_.push_back(connection);



    auto it = std::find(modulators_.begin(), modulators_.end(), connection->processor);
    size_t index = std::distance(modulators_.begin(), it);

    mod_routing[index].mod_connections.push_back(connection);

}

void bitklavier::ModulationProcessor::removeModulationConnection(ModulationConnection* connection){
    auto end = std::remove(all_modulation_connections_.begin(), all_modulation_connections_.end(), connection);
    all_modulation_connections_.erase(end, all_modulation_connections_.end());

    auto it = std::find(modulators_.begin(), modulators_.end(), connection->processor);
    size_t index = std::distance(modulators_.begin(), it);
    auto it_ = std::remove(mod_routing[index].mod_connections.begin(), mod_routing[index].mod_connections.end(), connection);
    mod_routing[index].mod_connections.erase(it_, mod_routing[index].mod_connections.end());

}
void bitklavier::ModulationProcessor::addModulationConnection(StateConnection* connection){
    all_state_connections_.push_back(connection);



   // auto it = std::find(modulators_.begin(), modulators_.end(), connection->processor);
    //size_t index = std::distance(modulators_.begin(), it);

    //mod_routing[index].mod_connections.push_back(connection);

}

void bitklavier::ModulationProcessor::removeModulationConnection(StateConnection* connection)
{
  auto end = std::remove(all_state_connections_.begin(), all_state_connections_.end(), connection);
  all_state_connections_.erase(end, all_state_connections_.end());

};
int bitklavier::ModulationProcessor::createNewModIndex()
{

   getBus(false,1)->setNumberOfChannels(getBus(false,1)->getNumberOfChannels()+1);
   return (getBus(false,1)->getNumberOfChannels()-1)-1; //(old number of channels) -1 to get 0 based
}

int bitklavier::ModulationProcessor::getNewModulationOutputIndex(const bitklavier::ModulationConnection& connection)
{
   for(auto _connection : all_modulation_connections_) {
       if (connection.destination_name == _connection->destination_name)
       {
           return  _connection->modulation_output_bus_index;
       }
   }

        return createNewModIndex();
}
int bitklavier::ModulationProcessor::getNewModulationOutputIndex(const bitklavier::StateConnection& connection)
{
    for(auto _connection : all_state_connections_) {
        if (connection.destination_name == _connection->destination_name)
        {
            return  _connection->modulation_output_bus_index;
        }
    }

    return -1;
}

ModulatorBase* bitklavier::ModulationProcessor::getModulatorBase(std::string& uuid)
{
    for (auto mod : modulators_)
    {
        if(mod->state.getProperty(IDs::uuid).toString() == juce::String(uuid))
        {
            return mod;
        }
    }
    return nullptr;

}