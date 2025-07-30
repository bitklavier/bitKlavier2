//
// Created by Davis Polito on 1/30/25.
//

#include "ModulationProcessor.h"
#include "ModulatorBase.h"
#include "ModulationConnection.h"
#include "ModulationList.h"
bitklavier::ModulationProcessor::ModulationProcessor(const juce::ValueTree& vt,SynthBase& parent) :
       juce::AudioProcessor(BusesProperties().withInput("disabled",juce::AudioChannelSet::mono(),false)
       .withOutput("disabled",juce::AudioChannelSet::mono(),false)
       .withOutput("Modulation",juce::AudioChannelSet::discreteChannels(1),true)
       .withInput( "Modulation",juce::AudioChannelSet::discreteChannels(1),true)
       .withInput("Reset",juce::AudioChannelSet::discreteChannels(1),true)), state(vt),
mod_list(std::make_unique<ModulationList>(state,&parent,this))
{
    createUuidProperty(state);
}
std::unique_ptr<juce::AudioProcessor> bitklavier::ModulationProcessor::create(SynthBase &parent, const juce::ValueTree &v) {

        return std::make_unique<ModulationProcessor>(v,parent);

}

void bitklavier::ModulationProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    // DBG("mod");
    auto reset_in = getBusBuffer(buffer,true,2);
    auto sample = buffer.getSample(0,0);
    auto sample1 = buffer.getSample(1,0);
    auto channel   = getChannelIndexInProcessBlockBuffer(true, 1, 0);

    if (reset_in.getSample(0,0))
    {
        if (reset_in.getSample(0,0) == 1)
        {
            DBG("ModulationProcessor::processBlock received reset " + juce::String(reset_in.getSample(0,0)));
            for (auto mod : modulators_)
                mod->triggerReset();
        }
    }


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
    // melatonin::printSparkline(buffer);
    //melatonin::printSparkline(buffer);
}

void bitklavier::ModulationProcessor::addModulator(ModulatorBase* mod) {
//update to search for any null locations
    mod->parent_ = this;
    // modulators_.push_back(mod);
    // Replace nullptrs with new instances
    // Add modulator to first nullptr slot or append at the end
    auto it = std::find(modulators_.begin(), modulators_.end(), nullptr);
    std::size_t index;

    if (it != modulators_.end()) {
        index = std::distance(modulators_.begin(), it);
        *it = mod; // replace nullptr with new modulator
    } else {
        index = modulators_.size();
        modulators_.push_back(mod); // add to end
    }

    // Ensure the auxiliary vectors are sized correctly
    if (tmp_buffers.size() <= index) {
        tmp_buffers.resize(index + 1);
    }
    tmp_buffers[index] = juce::AudioBuffer<float>(1, blockSize_); // or whatever buffer type

    if (mod_routing.size() <= index) {
        mod_routing.resize(index + 1);
    }
    mod_routing[index] = {}; // default construct routing entry
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
    modulators_[index]->connections_.push_back(connection->state);
}

void bitklavier::ModulationProcessor::removeModulationConnection(ModulationConnection* connection){
    auto end = std::remove(all_modulation_connections_.begin(), all_modulation_connections_.end(), connection);
    all_modulation_connections_.erase(end, all_modulation_connections_.end());

    auto it = std::find(modulators_.begin(), modulators_.end(), connection->processor);

    if (it != modulators_.end())
    {
        size_t index = std::distance(modulators_.begin(), it);
        if (mod_routing[index].mod_connections.size() != 0)
        {
            auto it_ = std::remove(mod_routing[index].mod_connections.begin(), mod_routing[index].mod_connections.end(), connection);
            mod_routing[index].mod_connections.erase(it_, mod_routing[index].mod_connections.end());
        }
    }
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