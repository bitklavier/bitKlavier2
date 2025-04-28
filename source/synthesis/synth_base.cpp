/* Copyright 2013-2019 Matt Tytel
 *
 * vital is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * vital is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with vital.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "synth_base.h"
#include "synth_gui_interface.h"
#include "melatonin_audio_sparklines/melatonin_audio_sparklines.h"

#include "startup.h"
#include "sound_engine.h"

#include "Identifiers.h"
#include "Synthesiser/Sample.h"
#include "load_save.h"
#include "RampModulator.h"
#include "valuetree_utils/VariantConverters.h"
#include "ModulationProcessor.h"
#include "StateModulator.h"

SynthBase::SynthBase(juce::AudioDeviceManager * deviceManager) : expired_(false), manager(deviceManager) {

  self_reference_ = std::make_shared<SynthBase*>();
  *self_reference_ = this;

  engine_ = std::make_unique<bitklavier::SoundEngine>();

  keyboard_state_ = std::make_unique<juce::MidiKeyboardState>();
  juce::ValueTree v;
  midi_manager_ = std::make_unique<MidiManager>( keyboard_state_.get(),manager, v );

  Startup::doStartupChecks();
  tree = juce::ValueTree(IDs::GALLERY);
  tree.setProperty(IDs::mainSampleSet,"Piano (Default)",nullptr);
  tree.addListener(this);
  //use valuetree rather than const valuetree bcus the std::ant cast ends upwith a juce::valuetree through cop
  modulator_factory.registerType<RampModulatorProcessor, juce::ValueTree>("ramp");
  modulator_factory.registerType<StateModulatorProcessor, juce::ValueTree>("state");
    mod_connections_.reserve(bitklavier::kMaxModulationConnections);
    state_connections_.reserve(bitklavier::kMaxStateConnections);
}

SynthBase::~SynthBase() {
  tree.removeListener(this);
}
//void SynthBase::pitchWheelMidiChanged(float value) {
//  ValueChangedCallback* callback = new ValueChangedCallback(self_reference_, "pitch_wheel", value);
//  callback->post();
//}
//
//void SynthBase::modWheelMidiChanged(float value) {
//  ValueChangedCallback* callback = new ValueChangedCallback(self_reference_, "mod_wheel", value);
//  callback->post();
//}
//
//
//
//void SynthBase::presetChangedThroughMidi(juce::File preset) {
//  SynthGuiInterface* gui_interface = getGuiInterface();
//  if (gui_interface) {
//    gui_interface->updateFullGui();
//    gui_interface->notifyFresh();
//  }
//}



void SynthBase::setMpeEnabled(bool enabled) {
  midi_manager_->setMpeEnabled(enabled);
}

juce::AudioProcessorGraph::Node::Ptr SynthBase::addProcessor(std::unique_ptr<juce::AudioProcessor> processor , juce::AudioProcessorGraph::NodeID id)
{
    processor->prepareToPlay(engine_->getSampleRate(), engine_->getBufferSize());
    return engine_->addNode(std::move(processor) , id);
}
bool SynthBase::isConnected(juce::AudioProcessorGraph::NodeID src, juce::AudioProcessorGraph::NodeID dest) {

    engine_->isConnected(src,dest);
}
juce::AudioProcessorGraph::Node * SynthBase::getNodeForId(juce::AudioProcessorGraph::NodeID id)
{
return engine_->getNodeForId(id);
}

void SynthBase::addConnection(juce::AudioProcessorGraph::Connection &connect)
{
    engine_->addConnection(connect);
}
void SynthBase::removeConnection(const juce::AudioProcessorGraph::Connection &connect) {
    engine_->removeConnection(connect);
}
bool SynthBase::loadFromValueTree(const juce::ValueTree& state)
{
    pauseProcessing(true);
    //engine_->allSoundsOff();
    tree.copyPropertiesAndChildrenFrom(state, nullptr);

    pauseProcessing(false);
    if (tree.isValid())
        return true;
    return false;
}

bool SynthBase::loadFromFile(juce::File preset, std::string& error) {
    if (!preset.exists())
        return false;

    auto xml = juce::parseXML(preset);
    if(xml == nullptr)
    {
        error = "Error loading preset";
        return false;
    }
    auto parsed_value_tree = juce::ValueTree::fromXml(*xml);
    if(!parsed_value_tree.isValid()) {

        error = "Error converting XML to juce::ValueTree";
        return false;
    }
    SynthGuiInterface* gui_interface = getGuiInterface();
    if (gui_interface) {
        this->mod_connections_.clear();
        this->state_connections_.clear();
        this->mod_connections_.reserve(bitklavier::kMaxModulationConnections);
        this->state_connections_.reserve(bitklavier::kMaxStateConnections);
        this->engine_->getModulationBank().reset();
        this->engine_->getStateBank().reset();
      gui_interface->updateFullGui();
    }
    if(!loadFromValueTree(parsed_value_tree))
    {
        error = "Error Initializing juce::ValueTree";
        return false;
    }

    //setPresetName(preset.getFileNameWithoutExtension());
    if (gui_interface) {
        gui_interface->updateFullGui();
        gui_interface->notifyFresh();
    }

    return true;
}

void SynthBase::processAudioAndMidi(juce::AudioBuffer<float>& audio_buffer, juce::MidiBuffer& midi_buffer) //, int channels, int samples, int offset, int start_sample = 0, int end_sample = 0)
{

    if (expired_)
        return;
    AudioThreadAction action;
    while (processorInitQueue.try_dequeue (action))
        action();

    engine_->processAudioAndMidi(audio_buffer, midi_buffer);

    //melatonin::printSparkline(audio_buffer);


}
void SynthBase::addModulationConnection(juce::AudioProcessorGraph::NodeID source, juce::AudioProcessorGraph::NodeID dest) {
   auto* sourceNode = getNodeForId(source);
   auto* destNode   = getNodeForId(dest);
   destNode->getProcessor()->getBus(true,1)->enable(true); //should always be modulation bus
    sourceNode->getProcessor()->getBus(false,1)->enable(true); //should always be modulation bus
   auto dest_index = destNode->getProcessor()->getChannelIndexInProcessBlockBuffer(true,1,0);
    auto source_index = sourceNode->getProcessor()->getChannelIndexInProcessBlockBuffer(false,1,0);

   juce::AudioProcessorGraph::Connection connection {{source, source_index}, {dest, dest_index}};
   engine_->addConnection(connection);
}
void SynthBase::writeAudio(juce::AudioSampleBuffer* buffer, int channels, int samples, int offset) {
  //const float* engine_output = (const float*)engine_->output(0)->buffer;
  /* get output of engine here */
  for (int channel = 0; channel < channels; ++channel) {
    float* channel_data = buffer->getWritePointer(channel, offset);
    //this line actually sends audio to the JUCE AudioSamplerBuffer to get audio out of the plugin
    for (int i = 0; i < samples; ++i) {
      //channel_data[i] = engine_output[float::kSize * i + channel];
      _ASSERT(std::isfinite(channel_data[i]));
    }
  }
 /*this line would send audio out to draw and get info from */
  //updateMemoryOutput(samples, engine_->output(0)->buffer);
}






int SynthBase::getSampleRate() {
  return engine_->getSampleRate();
}

bool SynthBase::isMidiMapped(const std::string& name) {
  return midi_manager_->isMidiMapped(name);
}



juce::ValueTree& SynthBase::getValueTree()
{
  return tree;
}

juce::UndoManager& SynthBase::getUndoManager()
{
  return um;
}


//modulations
std::vector<bitklavier::StateConnection*> SynthBase::getSourceStateConnections(const std::string& source)const {
    std::vector<bitklavier::StateConnection*> connections;
    for (auto& connection : state_connections_) {
        if (connection->source_name == source)
            connections.push_back(connection);
    }
    return connections;
}
std::vector<bitklavier::StateConnection*> SynthBase::getDestinationStateConnections(const std::string& destination)const {
    std::vector<bitklavier::StateConnection*> connections;
    for (auto& connection : state_connections_) {
        if (connection->destination_name == destination)
            connections.push_back(connection);
    }
    return connections;
}

std::vector<bitklavier::ModulationConnection*> SynthBase::getSourceConnections(const std::string& source)const {
    std::vector<bitklavier::ModulationConnection*> connections;
    for (auto& connection : mod_connections_) {
        if (connection->source_name == source)
            connections.push_back(connection);
    }
    return connections;
}
std::vector<bitklavier::ModulationConnection*> SynthBase::getDestinationConnections(const std::string& destination)const {
    std::vector<bitklavier::ModulationConnection*> connections;
    for (auto& connection : mod_connections_) {
        if (connection->destination_name == destination)
            connections.push_back(connection);
    }
    return connections;
}

bitklavier::ModulationConnection* SynthBase::getConnection(const std::string& source, const std::string& destination) const{
    for (auto& connection : mod_connections_) {
        if (connection->source_name == source && connection->destination_name == destination)
            return connection;
    }
    return nullptr;
}
bitklavier::StateConnection* SynthBase::getStateConnection(const std::string& source, const std::string& destination) const{
    for (auto& connection : state_connections_) {
        if (connection->source_name == source && connection->destination_name == destination)
            return connection;
    }
    return nullptr;
}



int SynthBase::getNumModulations(const std::string& destination) {
    int connections = 0;
    for (bitklavier::ModulationConnection* connection : mod_connections_) {
        if (connection->destination_name == destination)
            connections++;
    }
    return connections;
}

bitklavier::ModulationConnectionBank& SynthBase::getModulationBank() {
    return engine_->getModulationBank();
}
bitklavier::StateConnectionBank& SynthBase::getStateBank() {
    return engine_->getStateBank();
}

bool SynthBase::isSourceConnected(const std::string & source) {
    for (auto* connection : mod_connections_) {
        if (connection->source_name == source)
            return true;
    }
    return false;
}

void SynthBase::connectModulation(bitklavier::ModulationConnection *connection) {
   std::stringstream ss(connection->source_name);
   std::string uuid;
   std::getline(ss,uuid,'_');

   auto mod_src = tree.getChildWithName(IDs::PIANO).getChildWithName(IDs::PREPARATIONS).getChildWithProperty(IDs::uuid,juce::String(uuid));
   std::stringstream dst_stream(connection->destination_name);
   std::string dst_uuid;
   std::getline(dst_stream,dst_uuid,'_');
   std::string src_modulator_uuid_and_name;
   std::getline(ss,src_modulator_uuid_and_name,'_');
   auto pos = src_modulator_uuid_and_name.find_first_of("-");
   std::string juse_uuid = src_modulator_uuid_and_name.substr(pos+1,src_modulator_uuid_and_name.size());
   DBG(juse_uuid);
//   auto it = std::find_if(src_modulator_uuid_and_name.begin(),src_modulator_uuid_and_name.end(),::isdigit);
//   src_modulator_uuid_and_name.erase(src_modulator_uuid_and_name.begin(),it);
   DBG(src_modulator_uuid_and_name);

    auto mod_dst = tree.getChildWithName(IDs::PIANO).getChildWithName(IDs::PREPARATIONS).getChildWithProperty(IDs::uuid,juce::String(dst_uuid));


    auto mod_connections = tree.getChildWithName(IDs::PIANO).getChildWithName(IDs::MODCONNECTIONS);

    auto mod_connection =    mod_connections.getChildWithProperty(IDs::dest,  mod_dst.getProperty(IDs::nodeID));
//    connection->state.removeFromParent();


    std::string dst_param;
    std::getline(dst_stream,dst_param,'_');
    auto source_node = engine_->getNodeForId(juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar(mod_src.getProperty(IDs::nodeID)));
    auto dest_node = engine_->getNodeForId(juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar(mod_dst.getProperty(IDs::nodeID)));

    auto parameter_tree = mod_dst.getChildWithProperty(IDs::parameter,juce::String(dst_param));
    auto param_index = parameter_tree.getProperty(IDs::channel,-1);
    connection->parent_processor = dynamic_cast<bitklavier::ModulationProcessor*>(source_node->getProcessor());
    //determine where this would actually output in the modulationprocessor
    //if two seperate mods in modproc would modulate the same paramater for whatever reason they will map to the same
    // bus output
    //////i dont think any of this is threadsaf
    ///e
    connection->modulation_output_bus_index = connection->parent_processor->getNewModulationOutputIndex(*connection);
    connection->processor = connection->parent_processor->getModulatorBase(juse_uuid);
    connection->parent_processor->addModulationConnection(connection);


//    connection->parent_processor->modulation_connections_.push_back(connection);
    DBG("mod output bus index" + juce::String(connection->modulation_output_bus_index));
    auto source_index = source_node->getProcessor()->getChannelIndexInProcessBlockBuffer(false,1,0);//1 is mod
    auto dest_index = dest_node->getProcessor()->getChannelIndexInProcessBlockBuffer(true,1,param_index);
    //this is safe since we know that every source will be a modulationprocessor

//    juce::AudioProcessorGraph::Connection connection {{source_node->getId(), source_index}, {dest_node->getId(), dest_index}};
//    engine_->addConnection(connection);
    if(!parameter_tree.isValid() || !mod_src.isValid())
    {
        connection->destination_name = "";
        connection->source_name = "";
    }
    else if (mod_connections_.count(connection) == 0){
        mod_connections_.push_back(connection);
        connection->connection_ = {{source_node->nodeID, source_index}, {dest_node->nodeID, dest_index}};
        mod_connection.appendChild(connection->state, nullptr);
        getGuiInterface()->tryEnqueueProcessorInitQueue([this,connection](){
            engine_->addConnection(connection->connection_);
        });
    }

}
bool SynthBase::connectModulation(const juce::ValueTree &v) {
    bitklavier::ModulationConnection* connection = getConnection(v.getProperty(IDs::src).toString().toStdString(),v.getProperty(IDs::dest).toString().toStdString());
    bool create = connection == nullptr;
    if (create)
    {
        connection = getModulationBank().createConnection (v.getProperty(IDs::src).toString().toStdString(),v.getProperty(IDs::dest).toString().toStdString());
        connection->state = v;
    }
    if (connection)
        connectModulation(connection);
    return create;
}

bool SynthBase::connectModulation(const std::string &source, const std::string &destination) {
    bitklavier::ModulationConnection* connection = getConnection(source, destination);
    bool create = connection == nullptr;
    if (create)
    {
        connection = getModulationBank().createConnection (source, destination);
//        tree.appendChild(connection->state, nullptr);
    }
    if (connection)
        connectModulation(connection);
    return create;
}

void SynthBase::disconnectModulation(const std::string &source, const std::string &destination) {
    bitklavier::ModulationConnection* connection = getConnection(source, destination);
    if (connection)
        disconnectModulation(connection);
}

void SynthBase::disconnectStateModulation(const std::string &source, const std::string &destination) {
    bitklavier::StateConnection* connection = getStateConnection(source,destination);
    if (connection)
        disconnectModulation(connection);
}

void SynthBase::disconnectModulation(bitklavier::StateConnection *connection) {
    if(state_connections_.count(connection) == 0)
        return;
    connection->source_name = "";
    connection->destination_name = "";
    state_connections_.remove(connection);
    getGuiInterface()->tryEnqueueProcessorInitQueue([this,connection](){
        engine_->removeConnection(connection->connection_);
        connection->connection_ = {};
        connection->parent_processor->removeModulationConnection(connection);
    });
    connection->state.getParent().removeChild(connection->state, nullptr);
}
void SynthBase::disconnectModulation(bitklavier::ModulationConnection *connection) {
    if (mod_connections_.count(connection) == 0)
        return;
    connection->source_name = "";
    connection->destination_name = "";
    mod_connections_.remove(connection);
    getGuiInterface()->tryEnqueueProcessorInitQueue([this,connection](){
        engine_->removeConnection(connection->connection_);
        connection->connection_ = {};
        connection->parent_processor->removeModulationConnection(connection);
    });
    connection->state.getParent().removeChild(connection->state, nullptr);

}
void SynthBase::connectStateModulation(bitklavier::StateConnection *connection) {
    std::stringstream ss(connection->source_name);
    std::string uuid;
    std::getline(ss,uuid,'_');

    auto mod_src = tree.getChildWithName(IDs::PIANO).getChildWithName(IDs::PREPARATIONS).getChildWithProperty(IDs::uuid,juce::String(uuid));
    std::stringstream dst_stream(connection->destination_name);
    std::string dst_uuid;
    std::getline(dst_stream,dst_uuid,'_');
    std::string src_modulator_uuid_and_name;
    std::getline(ss,src_modulator_uuid_and_name,'_');
    auto pos = src_modulator_uuid_and_name.find_first_of("-");
    std::string juse_uuid = src_modulator_uuid_and_name.substr(pos+1,src_modulator_uuid_and_name.size());
    DBG(juse_uuid);
//   auto it = std::find_if(src_modulator_uuid_and_name.begin(),src_modulator_uuid_and_name.end(),::isdigit);
//   src_modulator_uuid_and_name.erase(src_modulator_uuid_and_name.begin(),it);
    DBG(src_modulator_uuid_and_name);

    auto mod_dst = tree.getChildWithName(IDs::PIANO).getChildWithName(IDs::PREPARATIONS).getChildWithProperty(IDs::uuid,juce::String(dst_uuid));


    auto mod_connections = tree.getChildWithName(IDs::PIANO).getChildWithName(IDs::MODCONNECTIONS);

    auto state_connection =    mod_connections.getChildWithProperty(IDs::dest,  mod_dst.getProperty(IDs::nodeID));
//    connection->state.removeFromParent();


    std::string dst_param;
    std::getline(dst_stream,dst_param,'_');
    auto source_node = engine_->getNodeForId(juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar(mod_src.getProperty(IDs::nodeID)));
    auto dest_node = engine_->getNodeForId(juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar(mod_dst.getProperty(IDs::nodeID)));

//    auto parameter_tree = mod_dst.getChildWithProperty(IDs::parameter,juce::String(dst_param));
//    auto param_index = parameter_tree.getProperty(IDs::channel,-1);
    connection->parent_processor = dynamic_cast<bitklavier::ModulationProcessor*>(source_node->getProcessor());
    //determine where this would actually output in the modulationprocessor
    //if two seperate mods in modproc would modulate the same paramater for whatever reason they will map to the same
    // bus output
    //////i dont think any of this is threadsafe
    connection->modulation_output_bus_index = connection->parent_processor->getNewModulationOutputIndex(*connection);
    connection->processor = connection->parent_processor->getModulatorBase(juse_uuid);
    connection->processor->addListener(connection);
    connection->parent_processor->addModulationConnection(connection);


//    connection->parent_processor->modulation_connections_.push_back(connection);
    DBG("mod output bus index" + juce::String(connection->modulation_output_bus_index));
    auto source_index = source_node->getProcessor()->getChannelIndexInProcessBlockBuffer(false,1,0);//1 is mod
//    auto dest_index = dest_node->getProcessor()->getChannelIndexInProcessBlockBuffer(true,1,param_index);
    //this is safe since we know that every source will be a modulationprocessor

        state_connections_.push_back(connection);
        connection->connection_ = {{source_node->nodeID, 0}, {dest_node->nodeID, 0}};
        state_connection.appendChild(connection->state, nullptr);
        getGuiInterface()->tryEnqueueProcessorInitQueue([this,connection](){

            engine_->addConnection(connection->connection_);
        });


}
bool SynthBase::connectStateModulation(const std::string &source, const std::string &destination) {
    bitklavier::StateConnection* connection = getStateConnection(source, destination);
    bool create = connection == nullptr;
    if (create)
    {
        connection = getStateBank().createConnection (source, destination);
//        tree.appendChild(connection->state, nullptr);
    }
    if (connection)
        connectStateModulation(connection);
    return create;
}