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

#include "CompressorProcessor.h"
#include "EQProcessor.h"
#include "SampleLoadManager.h"
#include "FullInterface.h"
#include "GainProcessor.h"
#include "melatonin_audio_sparklines/melatonin_audio_sparklines.h"
#include "synth_gui_interface.h"
#include "sound_engine.h"
#include "startup.h"
#include "../common/ObjectLists/PreparationList.h"
#include "Identifiers.h"
#include "LFOModulator.h"
#include "ModulationProcessor.h"
#include "ObjectLists/ConnectionsList.h"
#include "ObjectLists/ModConnectionsList.h"
#include "PluginBase.h"
#include "RampModulator.h"
#include "StateModulator.h"
#include "Synthesiser/Sample.h"
#include "TempoProcessor.h"
#include "TuningProcessor.h"
#include "SynchronicProcessor.h"
#include "chowdsp_sources/chowdsp_sources.h"
#include "valuetree_utils/VariantConverters.h"

SynthBase::SynthBase (juce::AudioDeviceManager* deviceManager) : expired_ (false), manager (deviceManager),user_prefs (new UserPreferencesWrapper()),
                                                                        sampleLoadManager (
                                                                            new SampleLoadManager (this,
                                                                                user_prefs
                                                                                ))
{
    self_reference_ = std::make_shared<SynthBase*>();
    *self_reference_ = this;

    keyboard_state_ = std::make_unique<juce::MidiKeyboardState>();
    midi_manager_ = std::make_unique<MidiManager> (keyboard_state_.get(), manager, juce::ValueTree{});

    Startup::doStartupChecks();
    tree = juce::ValueTree (IDs::GALLERY);
    sampleLoadManager->setValueTree(tree);

    /*
     * todo: soundset should be saved with piano, not gallery
     *        - same with instruments within soundfont
     *        - can we do that by simply doing `piano.setProperty (IDs::soundset, "Default", nullptr);1 ??
     *        - and/or I suppose `sampleLoadManager->setValueTree(piano);`
     *       but postfx (EQ and Compressor on 2bus) should be saved with gallery
     *        - we don't want to have to set/save them for individual pianos within a gallery
     *              - if someone wants to do that, they can do it manually by routing the main outs of all the preps to whatever they want
     *        - would this be `tree.appendChild (posteq, nullptr)` and `tree.appendChild (postcompressor, nullptr)` ??
     *       also General Settings, saved with Gallery
     */
    tree.setProperty (IDs::soundset, "Default", nullptr);
    juce::ValueTree piano (IDs::PIANO);
    juce::ValueTree preparations (IDs::PREPARATIONS);
    juce::ValueTree connections (IDs::CONNECTIONS);
    juce::ValueTree modconnections (IDs::MODCONNECTIONS);
    juce::ValueTree buseq (IDs::BUSEQ);
    juce::ValueTree buscompressor (IDs::BUSCOMPRESSOR);

    piano.appendChild (preparations, nullptr);
    piano.appendChild (connections, nullptr);
    piano.appendChild (modconnections, nullptr);
    piano.setProperty (IDs::isActive, 1, nullptr);
    piano.setProperty (IDs::name, "default", nullptr);
    buseq.setProperty(IDs::type, bitklavier::BKPreparationType::PreparationTypeEQ, nullptr);
    buscompressor.setProperty(IDs::type, bitklavier::BKPreparationType::PreparationTypeCompressor, nullptr);
    tree.appendChild (piano, nullptr);
    tree.addListener (this);
    tree.appendChild (buseq, nullptr);
    tree.appendChild (buscompressor, nullptr);

    //use valuetree rather than const valuetree bcus the std::ant cast ends upwith a juce::valuetree through cop
    modulator_factory.registerType<RampModulatorProcessor, juce::ValueTree> ("ramp");
    modulator_factory.registerType<LFOModulatorProcessor, juce::ValueTree> ("lfo");
    modulator_factory.registerType<StateModulatorProcessor, juce::ValueTree> ("state");
    mod_connections_.reserve (bitklavier::kMaxModulationConnections);
    state_connections_.reserve (bitklavier::kMaxStateConnections);
    preparationLists.emplace_back (
        std::make_unique<PreparationList> (
            *this, tree.getChildWithName (IDs::PIANO).getChildWithName (IDs::PREPARATIONS)));
    connectionLists.emplace_back (
        std::make_unique<bitklavier::ConnectionList> (
            *this, tree.getChildWithName (IDs::PIANO).getChildWithName (IDs::CONNECTIONS)));
    mod_connection_lists_.emplace_back (
        std::make_unique<bitklavier::ModConnectionList> (
            *this, tree.getChildWithName (IDs::PIANO).getChildWithName (IDs::MODCONNECTIONS)));
    engine_ = std::make_unique<bitklavier::SoundEngine>();
    gainProcessor = std::make_unique<GainProcessor>(*this,tree);
    compressorProcessor = std::make_unique<CompressorProcessor>(*this,buscompressor);
    eqProcessor = std::make_unique<EQProcessor>(*this,buseq);
}

SynthBase::~SynthBase()
{
    tree.removeListener (this);
    um.clearUndoHistory();
}

std::map<juce::String, juce::ReferenceCountedArray<BKSynthesiserSound > *>*SynthBase::getSamples() {
    for (const auto &[key, val] : sampleLoadManager->samplerSoundset)
            DBG(key);
    return &sampleLoadManager->samplerSoundset;
}

void SynthBase::deleteConnectionsWithId (juce::AudioProcessorGraph::NodeID delete_id)
{
    DBG ("delete connectionswithid");

    //cable connections i.e. audio/midi
    // if (connectionLists?
    auto* connectionList = getActiveConnectionList();
    auto size = connectionList->size();
    auto vt = connectionList->getValueTree();
    for (int i = 0; i < size;)
    {
        auto connection = vt.getChild (i);
        if (juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar (connection.getProperty (IDs::src)) == delete_id || juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar (connection.getProperty (IDs::dest)) == delete_id)
        {
            DBG ("remove connection");
            connectionList->removeChild (connection, &getUndoManager());
        }
        else
        {
            i++;
        }
    }
    //modulation connections, i.e. tuning, mod, reset
    auto* modConnectionList = getActiveModConnectionList();
    // if(modConnectionList) {
    auto size_ = modConnectionList->size();
    auto vt_ = modConnectionList->getValueTree();
    for (int i = 0; i < size;)
    {
        auto connection = vt_.getChild (i);
        if (juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar (connection.getProperty (IDs::src)) == delete_id || juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar (connection.getProperty (IDs::dest)) == delete_id)
        {
            DBG ("remove mod connection");
            modConnectionList->removeChild (connection, &getUndoManager());
        }
        else
        {
            i++;
        }
    }
    // }
}

void SynthBase::clearAllGuiListeners() {
    for (auto &connectionList: connectionLists)
        connectionList->clearListeners();
    for (auto &list : preparationLists)
        list->clearListeners();
    for (auto &list : mod_connection_lists_)
        list->clearListeners();
}

void SynthBase::valueTreeChildAdded (juce::ValueTree& parentTree,
    juce::ValueTree& childWhichHasBeenAdded)
{
    if (childWhichHasBeenAdded.hasType (IDs::PIANO))
    {
        DBG ("added piano");
        preparationLists.emplace_back (
            std::make_unique<PreparationList> (
                *this, childWhichHasBeenAdded.getOrCreateChildWithName (IDs::PREPARATIONS, nullptr)));
        connectionLists.emplace_back (std::make_unique<bitklavier::ConnectionList> (
            *this, childWhichHasBeenAdded.getOrCreateChildWithName (IDs::CONNECTIONS, nullptr)));
        mod_connection_lists_.emplace_back (std::make_unique<bitklavier::ModConnectionList> (
            *this, childWhichHasBeenAdded.getOrCreateChildWithName (IDs::MODCONNECTIONS, nullptr)));
        if (childWhichHasBeenAdded.getProperty (IDs::isActive))
        {
            if (getGuiInterface())
            {
                getGuiInterface()->setActivePiano (childWhichHasBeenAdded);
            }
        }
        // else
        // {
        //     setActivePiano (childWhichHasBeenAdded,SwitchTriggerThread::MessageThread);
        // }
    }
    if (childWhichHasBeenAdded.hasType (IDs::ModulationConnection))
    {
        if (connectModulation (childWhichHasBeenAdded))
            getGuiInterface()->notifyModulationsChanged();
    }
}

void SynthBase::valueTreeChildRemoved (juce::ValueTree& parentTree,
    juce::ValueTree& childWhichHasBeenRemoved,
    int indexFromWhichChildWasRemoved)
{
    if (childWhichHasBeenRemoved.hasType (IDs::ModulationConnection))
    {
        if (disconnectModulation (childWhichHasBeenRemoved))
            getGuiInterface()->notifyModulationsChanged();
    }
}

void SynthBase::valueTreePropertyChanged (juce::ValueTree& treeWhosePropertyHasChanged,
    const juce::Identifier& property)
{
    if (property == IDs::isActive && treeWhosePropertyHasChanged.hasType (IDs::PIANO) && static_cast<int> (treeWhosePropertyHasChanged.getProperty (IDs::isActive)) == 1)
    {
        if (getGuiInterface())
        {
            getGuiInterface()->setActivePiano (treeWhosePropertyHasChanged);
        }
        // else
        // {
        //     setActivePiano (treeWhosePropertyHasChanged);
        // }
    }
}

PreparationList* SynthBase::getActivePreparationList()
{
    for (auto& preparation : preparationLists)
    {
        if (preparation->getValueTree().getParent().getProperty (IDs::isActive))
            return preparation.get();
    }
    jassertfalse;
    return nullptr;
}

bitklavier::ConnectionList* SynthBase::getActiveConnectionList()
{
    for (auto& connection : connectionLists)
    {
        if (connection->getValueTree().getParent().getProperty (IDs::isActive))
            return connection.get();
    }
    jassertfalse;
    return nullptr;
}

bitklavier::ModConnectionList* SynthBase::getActiveModConnectionList()
{
    for (auto& connection : mod_connection_lists_)
    {
        if (connection->getValueTree().getParent().getProperty (IDs::isActive))
            return connection.get();
    }
    jassertfalse;
    return nullptr;
}

void SynthBase::setActivePiano (const juce::ValueTree& v, SwitchTriggerThread thread)
{
    //DBG ("setActivePiano");
    activePiano = v;
    switch_trigger_thread = thread;
    if (thread == SwitchTriggerThread::MessageThread)
    {
        processorInitQueue.try_enqueue ([this] { engine_->setActivePiano (activePiano); });
    }
    else
    {
        engine_->setActivePiano (activePiano);
    }

    // tree.removeListener (this);
    // tree.setProperty (IDs::isActive, 0, nullptr);
    // tree = v;
    // tree.addListener (this);
}

void SynthBase::addTuningConnection (juce::AudioProcessorGraph::NodeID src, juce::AudioProcessorGraph::NodeID dest)
{
    auto* sourceNode = getNodeForId (src);
    auto* destNode = getNodeForId (dest);
    dynamic_cast<bitklavier::InternalProcessor*> (destNode->getProcessor())->setTuning (dynamic_cast<TuningProcessor*> (sourceNode->getProcessor()));
    addModulationConnection (src, dest);
}

void SynthBase::connectTuning (const juce::ValueTree& v)
{
    auto srcid = juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar (v.getProperty (IDs::src));
    auto dstid = juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar (v.getProperty (IDs::dest));
    addTuningConnection (srcid, dstid);
}

void SynthBase::addTempoConnection (juce::AudioProcessorGraph::NodeID src, juce::AudioProcessorGraph::NodeID dest)
{
    auto* sourceNode = getNodeForId (src);
    auto* destNode = getNodeForId (dest);
    dynamic_cast<bitklavier::InternalProcessor*> (destNode->getProcessor())->setTempo (dynamic_cast<TempoProcessor*> (sourceNode->getProcessor()));
    addModulationConnection (src, dest);
}

void SynthBase::connectTempo (const juce::ValueTree& v)
{
    auto srcid = juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar (v.getProperty (IDs::src));
    auto dstid = juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar (v.getProperty (IDs::dest));
    addTempoConnection (srcid, dstid);
}

void SynthBase::addSynchronicConnection (juce::AudioProcessorGraph::NodeID src, juce::AudioProcessorGraph::NodeID dest)
{
    auto* sourceNode = getNodeForId (src);
    auto* destNode = getNodeForId (dest);
    dynamic_cast<bitklavier::InternalProcessor*> (destNode->getProcessor())->setSynchronic (dynamic_cast<SynchronicProcessor*> (sourceNode->getProcessor()));
    addModulationConnection (src, dest);
}

void SynthBase::connectSynchronic (const juce::ValueTree& v)
{
    auto srcid = juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar (v.getProperty (IDs::src));
    auto dstid = juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar (v.getProperty (IDs::dest));
    addSynchronicConnection (srcid, dstid);
}

void SynthBase::setMpeEnabled (bool enabled)
{
    midi_manager_->setMpeEnabled (enabled);
}

juce::AudioProcessorGraph::Node::Ptr SynthBase::addProcessor (std::unique_ptr<juce::AudioProcessor> processor,
    juce::AudioProcessorGraph::NodeID id)
{
    // if ( auto * pianoSwitch = dynamic_cast<PianoSwitchProcessor*>(processor.get())) {
    //
    // }
    return engine_->addNode (std::move (processor), id);
}

juce::AudioProcessorGraph::Node::Ptr SynthBase::removeProcessor (juce::AudioProcessorGraph::NodeID id)
{
    if(engine_)
        return engine_->removeNode (id);
    else
        return nullptr;
}

bool SynthBase::isConnected (juce::AudioProcessorGraph::NodeID src, juce::AudioProcessorGraph::NodeID dest)
{
    engine_->isConnected (src, dest);
}

juce::AudioProcessorGraph::Node* SynthBase::getNodeForId (juce::AudioProcessorGraph::NodeID id)
{
    return engine_->getNodeForId (id);
}

void SynthBase::addConnection (juce::AudioProcessorGraph::Connection& connect)
{
    engine_->addConnection (connect);
}

void SynthBase::removeConnection (const juce::AudioProcessorGraph::Connection& connect)
{
    engine_->removeConnection (connect);
}

bool SynthBase::loadFromValueTree (const juce::ValueTree& state)
{
    //engine_->allSoundsOff();
    pauseProcessing(true);
    tree.copyPropertiesAndChildrenFrom (state, nullptr);

    pauseProcessing (false);
    if (tree.isValid())
        return true;
    return false;
}

void SynthBase::clearAllBackend() {

    this->mod_connections_.clear();
    this->state_connections_.clear();
    this->mod_connections_.reserve (bitklavier::kMaxModulationConnections);
    this->state_connections_.reserve (bitklavier::kMaxStateConnections);
    this->engine_->getModulationBank().reset();
    this->engine_->getStateBank().reset();
    preparationLists.clear();
    mod_connection_lists_.clear();
    connectionLists.clear();
    this->engine_->initialiseGraph();
}
struct SoundsetRef
{
    juce::String   name;
    juce::ValueTree target; // node to write IDs::soundset into (or used for later completion)
};

static void collectSoundsetRefsRecursive (const juce::ValueTree& node,
                                         juce::Array<SoundsetRef>& out)
{
    if (!node.isValid())
        return;

    if (node.hasProperty (IDs::soundset))
    {
        auto ss = node.getProperty (IDs::soundset).toString();
        if (ss.isNotEmpty() && ss != IDs::syncglobal.toString() )
        {
            out.add ({ ss, node });
        }
    }

    for (int i = 0; i < node.getNumChildren(); ++i)
        collectSoundsetRefsRecursive (node.getChild(i), out);
}
// bool SynthBase::loadFromFile (juce::File preset, std::string& error)
// {
//     if (!preset.exists())
//         return false;
//
//     auto xml = juce::parseXML (preset);
//     if (xml == nullptr)
//     {
//         error = "Error loading preset";
//         return false;
//     }
//     auto parsed_value_tree = juce::ValueTree::fromXml (*xml);
//     if (!parsed_value_tree.isValid())
//     {
//         error = "Error converting XML to juce::ValueTree";
//         return false;
//     }
//
//     clearAllBackend();
//     SynthGuiInterface* gui_interface = getGuiInterface();
//     if (gui_interface)
//     {
//         gui_interface->removeAllGuiListeners();
//     }
//     engine_->resetEngine();
//     if (!loadFromValueTree (parsed_value_tree))
//     {
//         error = "Error Initializing juce::ValueTree";
//         return false;
//     }
//
//     //setPresetName(preset.getFileNameWithoutExtension());
//     if (gui_interface)
//     {
//         gui_interface->updateFullGui();
//         gui_interface->notifyFresh();
//     }
//
//     // load samples
//     juce::Array<SoundsetRef> refs;
//     collectSoundsetRefsRecursive (parsed_value_tree, refs);
//
//     // Dedup by soundset name (but still allow multiple target nodes to be updated)
//     juce::StringArray uniqueSoundsets;
//     uniqueSoundsets.ensureStorageAllocated (refs.size());
//
//     for (auto& r : refs)
//         uniqueSoundsets.addIfNotAlreadyThere (r.name);
//
//     // Kick off loads FIRST
//     if (sampleLoadManager != nullptr)
//     {
//         for (auto& r : refs)
//             sampleLoadManager->loadSamples (r.name, r.target);
//
//         // Optional: make sure each referencing node has the property normalized
//         // (useful if you want to enforce Gallery/global routing at read-time)
//         // for (auto& r : refs)
//         // {
//         //     // If your loadSamples(ss, target) sets IDs::soundset appropriately when already-loaded,
//         //     // you can call that instead of direct setProperty.
//         //     r.target.setProperty (IDs::soundset, r.name, nullptr);
//         // }
//     }
//     return true;
// }
bool SynthBase::loadFromFile ( juce::File preset, std::string& error)
{
    if (!preset.exists())
        return false;

    auto xml = juce::parseXML (preset);
    if (xml == nullptr)
    {
        error = "Error loading preset";
        return false;
    }

    auto parsed = juce::ValueTree::fromXml (*xml);
    if (!parsed.isValid())
    {
        error = "Error converting XML to juce::ValueTree";
        return false;
    }

    // ---------- PREPASS: collect all soundset references ----------
    juce::Array<SoundsetRef> soundsetRefs;
    collectSoundsetRefsRecursive (parsed, soundsetRefs);

    bool needsAsyncLoads = false;

    // ---------- Queue sample loads ----------
    if (sampleLoadManager != nullptr)
    {
        for (auto& ref : soundsetRefs)
        {
            if (! sampleLoadManager->isSoundsetLoaded (ref.name))
            {
                needsAsyncLoads = true;


                sampleLoadManager->loadSamples (ref.name, ref.target);
            }
        }
    }


    // ---------- Reset backend BEFORE applying preset ----------

    pauseProcessing(true);
    clearAllBackend();
    pauseProcessing(false);
    if (auto* gui = getGuiInterface())
        gui->removeAllGuiListeners();


    engine_->resetEngine();

    // ---------- Defer preset application if samples are loading ----------
    if (needsAsyncLoads)
    {
        pendingPresetTree = parsed;
        presetPending.store (true);

        startSampleLoading(); // spinner / disable audio etc.
        return true;
    }

    // ---------- Otherwise apply immediately ----------
    if (! loadFromValueTree (parsed))
    {
        error = "Error Initializing juce::ValueTree";
        return false;
    }

    if (auto* gui = getGuiInterface())
    {
        gui->updateFullGui();
        gui->notifyFresh();
    }

    return true;
}
void SynthBase::startSampleLoading()
{
    samplesLoading.store (true);

    // Optional but common:
    // engine_->suspendProcessing (true);

    if(getGuiInterface() && getGuiInterface()->getGui())
        getGuiInterface()->getGui()->showLoadingSection();
}
void SynthBase::finishedSampleLoading()
{
    if (presetPending.load()) {
        presetPending.store (false);

        if (! loadFromValueTree (pendingPresetTree))
        {
            DBG ("Failed to apply preset after samples loaded");
        }
        else if (auto* gui = getGuiInterface())
        {
            gui->updateFullGui();
            gui->notifyFresh();
        }
        pendingPresetTree = juce::ValueTree{};
    }
    if(getGuiInterface() && getGuiInterface()->getGui()) {
        getGuiInterface()->getGui()->hideLoadingSection();
       getGuiInterface()->getGui()->notifyFresh();
    }
    samplesLoading.store (false);

}

void SynthBase::processAudioAndMidi (juce::AudioBuffer<float>& audio_buffer, juce::MidiBuffer& midi_buffer)
//, int channels, int samples, int offset, int start_sample = 0, int end_sample = 0)
{
    if (expired_)
        return;
    AudioThreadAction action;
    while (processorInitQueue.try_dequeue (action))
        action();

    engine_->processAudioAndMidi (audio_buffer, midi_buffer);
    gainProcessor->processBlock (audio_buffer, midi_buffer);
    eqProcessor->processBlock (audio_buffer, midi_buffer);
    compressorProcessor->processBlock (audio_buffer, midi_buffer);
    sample_index_of_switch = std::numeric_limits<int>::min();
    //melatonin::printSparkline(audio_buffer);
}

//modulation connections are used for both audio rate modulations and to order tuning/modulation/reset/and piano change
//preparations to occur before all other preparations they are connected to
// NOTE: piano change connections must be ensure to occur before all other preparations
bool SynthBase::addModulationConnection (juce::AudioProcessorGraph::NodeID source,
    juce::AudioProcessorGraph::NodeID dest)
{
    auto* sourceNode = getNodeForId (source);
    auto* destNode = getNodeForId (dest);
    destNode->getProcessor()->getBus (true, 1)->enable (true); //should always be modulation bus
    sourceNode->getProcessor()->getBus (false, 1)->enable (true); //should always be modulation bus
    auto dest_index = destNode->getProcessor()->getChannelIndexInProcessBlockBuffer (true, 1, 0);
    auto source_index = sourceNode->getProcessor()->getChannelIndexInProcessBlockBuffer (false, 1, 0);

    juce::AudioProcessorGraph::Connection connection { { source, source_index }, { dest, dest_index } };
    return engine_->addConnection (connection);
}

void SynthBase::writeAudio (juce::AudioSampleBuffer* buffer, int channels, int samples, int offset)
{
    //const float* engine_output = (const float*)engine_->output(0)->buffer;
    /* get output of engine here */
    for (int channel = 0; channel < channels; ++channel)
    {
        float* channel_data = buffer->getWritePointer (channel, offset);
        //this line actually sends audio to the JUCE AudioSamplerBuffer to get audio out of the plugin
        for (int i = 0; i < samples; ++i)
        {
            //channel_data[i] = engine_output[float::kSize * i + channel];
            _ASSERT (std::isfinite (channel_data[i]));
        }
    }
    /*this line would send audio out to draw and get info from */
    //updateMemoryOutput(samples, engine_->output(0)->buffer);
}

juce::AudioProcessorGraph::Node::Ptr SynthBase::addPlugin (std::unique_ptr<juce::AudioPluginInstance> instance,
    const juce::String& error,
    juce::Point<double> pos,
    PluginDescriptionAndPreference::UseARA useARA)
{
}

int SynthBase::getSampleRate()
{
    return engine_->getSampleRate();
}

int SynthBase::getBufferSize()
{
    return engine_->getBufferSize();
}

bool SynthBase::isMidiMapped (const std::string& name)
{
    return midi_manager_->isMidiMapped (name);
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
std::vector<bitklavier::StateConnection*> SynthBase::getSourceStateConnections (const std::string& source) const
{
    std::vector<bitklavier::StateConnection*> connections;

    for (auto connection : state_connections_)
    {
        DBG(connection->source_name);

        if (connection->source_name == source)
            connections.push_back (connection);
    }
    return connections;
}


std::vector<bitklavier::StateConnection*> SynthBase::getDestinationStateConnections (
    const std::string& destination) const
{
    std::vector<bitklavier::StateConnection*> connections;
    for (auto connection : state_connections_)
    {
        if (connection->destination_name == destination) {
            // check if already in vector
            if (std::find(connections.begin(), connections.end(), connection) == connections.end())
                connections.push_back(connection);
        }
    }
    return connections;
}

std::vector<bitklavier::ModulationConnection*> SynthBase::getSourceConnections (const std::string& source) const
{
    std::vector<bitklavier::ModulationConnection*> connections;
    for (auto connection : mod_connections_)
    {
        if (connection->source_name == source)
            connections.push_back (connection);
    }
    return connections;
}

std::vector<bitklavier::ModulationConnection*> SynthBase::getDestinationConnections (
    const std::string& destination) const
{
    std::vector<bitklavier::ModulationConnection*> connections;
    for (auto connection : mod_connections_)
    {
        if (connection->destination_name == destination) {
            // check if already in vector
            if (std::find(connections.begin(), connections.end(), connection) == connections.end())
                connections.push_back(connection);
        }
    }
    return connections;
}

bitklavier::ModulationConnection* SynthBase::getConnection (const std::string& source,
    const std::string& destination) const
{
    for (auto connection : mod_connections_)
    {
        if (connection->source_name == source && connection->destination_name == destination)
            return connection;
    }
    return nullptr;
}

bitklavier::StateConnection* SynthBase::getStateConnection (const std::string& source,
    const std::string& destination) const
{
    for (auto connection : state_connections_)
    {
        if (connection->source_name == source && connection->destination_name == destination)
            return connection;
    }
    return nullptr;
}

int SynthBase::getNumModulations (const std::string& destination)
{
    int connections = 0;
    for (bitklavier::ModulationConnection* connection : mod_connections_)
    {
        if (connection->destination_name == destination)
            connections++;
    }
    return connections;
}

bitklavier::ModulationConnectionBank& SynthBase::getModulationBank()
{
    return engine_->getModulationBank();
}

bitklavier::StateConnectionBank& SynthBase::getStateBank()
{
    return engine_->getStateBank();
}
bitklavier::ParamOffsetBank& SynthBase::getParamOffsetBank() {
    return engine_->getParamOffsetBank();
}
bool SynthBase::isSourceConnected (const std::string& source)
{
    for (auto* connection : mod_connections_)
    {
        if (connection->source_name == source)
            return true;
    }
    return false;
}

void SynthBase::connectModulation (bitklavier::ModulationConnection* connection)
{
    if (mod_connections_.count (connection) == 1)
        return;
    std::string src_uuid;
    std::string dst_uuid;
    std::string src_modulator_uuid_and_name;
    std::string dst_param;
    std::stringstream src_stream (connection->source_name);
    std::stringstream dst_stream (connection->destination_name);
    //get src info
    std::getline (src_stream, src_uuid, '_');
    std::getline (src_stream, src_modulator_uuid_and_name, '_');
    //get dest info
    std::getline (dst_stream, dst_uuid, '_');
    std::getline (dst_stream, dst_param, '_');

    auto pos = src_modulator_uuid_and_name.find_first_of ("-");
    std::string internal_modulator_uuid = src_modulator_uuid_and_name.substr (pos + 1, src_modulator_uuid_and_name.size());
    DBG (internal_modulator_uuid);
    DBG (src_modulator_uuid_and_name);

    //get actual value tree representations from string uuids
    auto mod_src = tree.getChildWithName (IDs::PIANO).getChildWithName (IDs::PREPARATIONS).getChildWithProperty (IDs::uuid, juce::String (src_uuid));
    auto mod_dst = tree.getChildWithName (IDs::PIANO).getChildWithName (IDs::PREPARATIONS).getChildWithProperty (IDs::uuid, juce::String (dst_uuid));
    auto mod_connections = tree.getChildWithName (IDs::PIANO).getChildWithName (IDs::MODCONNECTIONS);
    auto mod_connection = mod_connections.getChildWithProperty (IDs::dest, mod_dst.getProperty (IDs::nodeID));

    //get backend audio graph representation from value tree
    auto source_node = engine_->getNodeForId (juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar (mod_src.getProperty (IDs::nodeID)));
    auto dest_node = engine_->getNodeForId (juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar (mod_dst.getProperty (IDs::nodeID)));

    auto parameter_tree = mod_dst.getChildWithName (IDs::MODULATABLE_PARAMS).getChildWithProperty (IDs::parameter, juce::String (dst_param));
    connection->setParamTree (parameter_tree);
    jassert (parameter_tree.isValid()); //if you hit this then the Parameter ID is not a modulatable param listed in the value tree.
    //this means the paramid for the component does not match a modulatable param on the backend
    //this requires you to add the Parameter ID to the value tree as a MODULATABLE_PARAM see DirectProcessor Constructor for an example
    /**
     *todo: shouldn't we just ignore it then? I've hit this when just dragging a mod across the UI
    * DAVIS : no this assert is to say that you are missing a REQUIRED step in the setup of parameters.
    * That is to say you have dragged a mod across the UI and crossed some parameter that is in a half-setup state
    * mods are created anytime a parameter is dragged over a knob so that it can show the modulatable param circle
    * this isnt a requiremnt but more of a nice UI feature where you get the snap as it crosses over without having to
    * drop the modulation.
    */

    //determine where this would actually output in the modulationprocessor
    //if two seperate mods in modproc would modulate the same paramater for whatever reason they will map to the same
    // bus output
    ////i dont think any of this is threadsaf

    //populate connection class with backend information
    connection->parent_processor = dynamic_cast<bitklavier::ModulationProcessor*> (source_node->getProcessor());
    // connection->modulation_output_bus_index = connection->parent_processor->getNewModulationOutputIndex (*connection);
    connection->processor = connection->parent_processor->getModulatorBase (internal_modulator_uuid);
    connection->parent_processor->addModulationConnection (connection);

    connection->processor->addListener (connection);


    auto param_index =  parameter_tree.getParent().indexOf(parameter_tree);// parameter_tree.getProperty (IDs::channel, -1);
    auto source_index = source_node->getProcessor()->getChannelIndexInProcessBlockBuffer (false, 1, connection->modulation_output_bus_index); //1 is mod
    auto dest_index = dest_node->getProcessor()->getChannelIndexInProcessBlockBuffer (true, 1, param_index);
    if(connection->isContinuousMod) {
        auto numInputChannels = dest_node->getProcessor()->getChannelCountOfBus(true,1) / 2;
        dest_index  =  dest_node->getProcessor()->getChannelIndexInProcessBlockBuffer (true, 1, param_index + numInputChannels);
    }
    //do the final backend adding
    if (!parameter_tree.isValid() || !mod_src.isValid())
    {
        connection->destination_name = "";
        connection->source_name = "";
    }
    else if (mod_connections_.count (connection) == 0)
    {
        connection->setDestParamIndex(getParamOffsetBank().getIndexIfExists(connection->destination_name));
        mod_connections_.push_back (connection);
        connection->connection_ = { { source_node->nodeID, source_index }, { dest_node->nodeID, dest_index } };
        um.beginNewTransaction();
        mod_connection.appendChild (connection->state, &um);

        bool connectionAdded = engine_->addConnection (connection->connection_);

    }
}

bool SynthBase::connectReset (const juce::ValueTree& v)
{
    auto sourceId = juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar (v.getProperty (IDs::src, -1));
    auto destId = juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar (v.getProperty (IDs::dest, -1));
    auto source_index = engine_->getNodeForId (sourceId)->getProcessor()->getChannelIndexInProcessBlockBuffer (false, 2, 0); //2 is reset
    auto dest_index = engine_->getNodeForId (destId)->getProcessor()->getChannelIndexInProcessBlockBuffer (true, 2, 0);
    //1 is mod

    juce::AudioProcessorGraph::Connection connection_ = { { sourceId, source_index }, { destId, dest_index } };

    auto b = engine_->addConnection (connection_);
    if (b)
        DBG ("connected");
    else
        DBG ("not connected");
}

bool SynthBase::connectModulation (const juce::ValueTree& v)
{
    if (v.getProperty (IDs::isState))
    {
        bitklavier::StateConnection* connection = getStateConnection (v.getProperty (IDs::src).toString().toStdString(),
            v.getProperty (IDs::dest).toString().toStdString());
        bool create = connection == nullptr;
        if (create)
        {
            connection = getStateBank().createConnection (v.getProperty (IDs::src).toString().toStdString(), v.getProperty (IDs::dest).toString().toStdString());
            connection->state = v;
            connection->setChange (v);

            state_connections_.push_back (connection);
        }
        if (connection)
            connectStateModulation (connection);
        return create;
    }

    bitklavier::ModulationConnection* connection = getConnection (v.getProperty (IDs::src).toString().toStdString(), v.getProperty (IDs::dest).toString().toStdString());
    bool create = connection == nullptr;
    if (create)
    {
        connection = getModulationBank().createConnection (v.getProperty (IDs::src).toString().toStdString(), v.getProperty (IDs::dest).toString().toStdString());
        connection->setStateValueTree (v);

    }
    if (connection)
        connectModulation (connection);
    return create;
}

bool SynthBase::connectModulation (const std::string& source, const std::string& destination)
{
    bitklavier::ModulationConnection* connection = getConnection (source, destination);
    bool create = connection == nullptr;
    if (create)
    {
        connection = getModulationBank().createConnection (source, destination);
        //        tree.appendChild(connection->state, nullptr);
    }
    if (connection)
        connectModulation (connection);
    return create;
}

void SynthBase::disconnectModulation (const std::string& source, const std::string& destination)
{
    bitklavier::ModulationConnection* connection = getConnection (source, destination);
    if (connection)
        disconnectModulation (connection);
}

bool SynthBase::disconnectModulation (const juce::ValueTree& v)
{
    bitklavier::ModulationConnection* connection = getConnection (v.getProperty (IDs::src).toString().toStdString(),
        v.getProperty (IDs::dest).toString().toStdString());
    if (connection)
        disconnectModulation (connection);
    return connection;
}

void SynthBase::disconnectStateModulation (const std::string& source, const std::string& destination)
{
    bitklavier::StateConnection* connection = getStateConnection (source, destination);
    if (connection)
        disconnectModulation (connection);
}

void SynthBase::disconnectModulation (bitklavier::StateConnection* connection)
{
    if (state_connections_.count (connection) == 0)
        return;
    connection->source_name = "";
    connection->destination_name = "";
    DBG("state connection listener being removed");
    connection->processor->removeListener (connection);
    state_connections_.remove (connection);
    engine_->removeConnection (connection->connection_);
    getGuiInterface()->tryEnqueueProcessorInitQueue ([this, connection]() {
        connection->connection_ = {};
        connection->parent_processor->removeModulationConnection (connection);
    });
    connection->state.getParent().removeChild (connection->state, nullptr);
}

void SynthBase::disconnectModulation (bitklavier::ModulationConnection* connection)
{
    if (mod_connections_.count (connection) == 0)
        return;
    auto destination_name = connection->destination_name;
    connection->source_name = "";
    connection->destination_name = "";

    mod_connections_.remove (connection);



    connection->parent_processor->removeModulationConnection (connection, destination_name);
    connection->connection_ = {};
    um.beginNewTransaction();
    connection->state.getParent().removeChild (connection->state, nullptr);
}

void SynthBase::connectStateModulation (bitklavier::StateConnection* connection)
{
    std::stringstream ss (connection->source_name);
    std::string uuid;
    std::getline (ss, uuid, '_');

    auto mod_src = tree.getChildWithName (IDs::PIANO).getChildWithName (IDs::PREPARATIONS).getChildWithProperty (IDs::uuid, juce::String (uuid));
    std::stringstream dst_stream (connection->destination_name);
    std::string dst_uuid;
    std::getline (dst_stream, dst_uuid, '_');
    std::string src_modulator_uuid_and_name;
    std::getline (ss, src_modulator_uuid_and_name, '_');
    auto pos = src_modulator_uuid_and_name.find_first_of ("-");
    std::string juse_uuid = src_modulator_uuid_and_name.substr (pos + 1, src_modulator_uuid_and_name.size());
    //DBG (juse_uuid);
    //   auto it = std::find_if(src_modulator_uuid_and_name.begin(),src_modulator_uuid_and_name.end(),::isdigit);
    //   src_modulator_uuid_and_name.erase(src_modulator_uuid_and_name.begin(),it);
    //DBG (src_modulator_uuid_and_name);

    auto mod_dst = tree.getChildWithName (IDs::PIANO).getChildWithName (IDs::PREPARATIONS).getChildWithProperty (IDs::uuid, juce::String (dst_uuid));

    auto mod_connections = tree.getChildWithName (IDs::PIANO).getChildWithName (IDs::MODCONNECTIONS);

    auto state_connection = mod_connections.getChildWithProperty (IDs::dest, mod_dst.getProperty (IDs::nodeID));
    //    connection->state.removeFromParent();

    std::string dst_param;
    std::getline (dst_stream, dst_param, '_');
    auto source_node = engine_->getNodeForId (
        juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar (mod_src.getProperty (IDs::nodeID)));
    auto dest_node = engine_->getNodeForId (
        juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar (mod_dst.getProperty (IDs::nodeID)));

    //    auto parameter_tree = mod_dst.getChildWithProperty(IDs::parameter,juce::String(dst_param));
    //    auto param_index = parameter_tree.getProperty(IDs::channel,-1);
    connection->parent_processor = dynamic_cast<bitklavier::ModulationProcessor*> (source_node->getProcessor());
    //determine where this would actually output in the modulationprocessor
    //if two seperate mods in modproc would modulate the same paramater for whatever reason they will map to the same
    // bus output
    connection->modulation_output_bus_index = connection->parent_processor->getNewModulationOutputIndex (*connection);
    connection->processor = connection->parent_processor->getModulatorBase (juse_uuid);
    DBG("state connection listener being added");
    connection->processor->addListener (connection);
    connection->parent_processor->addModulationConnection (connection);
    //    jassert(connection->modulation_output_bus_index != -1);

    //    connection->parent_processor->modulation_connections_.push_back(connection);
    //DBG ("mod output bus index" + juce::String (connection->modulation_output_bus_index));
    auto source_index = source_node->getProcessor()->getChannelIndexInProcessBlockBuffer (false, 1, 0); //1 is mod
    //    auto dest_index = dest_node->getProcessor()->getChannelIndexInProcessBlockBuffer(true,1,param_index);
    //this is safe since we know that every source will be a modulationprocessor


    connection->connection_ = { { source_node->nodeID, 0 }, { dest_node->nodeID, 0 } };

    // getGuiInterface()->tryEnqueueProcessorInitQueue ([this, connection]() {
    engine_->addConnection (connection->connection_);
    // });
}

bool SynthBase::connectStateModulation (const std::string& source, const std::string& destination)
{
    std::stringstream ss (source);
    std::string uuid;
    std::getline (ss, uuid, '_');

    auto mod_src = tree.getChildWithName (IDs::PIANO).getChildWithName (IDs::PREPARATIONS).getChildWithProperty (IDs::uuid, juce::String (uuid));
    std::stringstream dst_stream (destination);
    std::string dst_uuid;
    std::getline (dst_stream, dst_uuid, '_');
    std::string src_modulator_uuid_and_name;
    std::getline (ss, src_modulator_uuid_and_name, '_');
    auto pos = src_modulator_uuid_and_name.find_first_of ("-");
    std::string juse_uuid = src_modulator_uuid_and_name.substr (pos + 1, src_modulator_uuid_and_name.size());
    //DBG (juse_uuid);
    //   auto it = std::find_if(src_modulator_uuid_and_name.begin(),src_modulator_uuid_and_name.end(),::isdigit);
    //   src_modulator_uuid_and_name.erase(src_modulator_uuid_and_name.begin(),it);
    //DBG (src_modulator_uuid_and_name);

    auto mod_dst = tree.getChildWithName (IDs::PIANO).getChildWithName (IDs::PREPARATIONS).getChildWithProperty (IDs::uuid, juce::String (dst_uuid));

    auto mod_connections = tree.getChildWithName (IDs::PIANO).getChildWithName (IDs::MODCONNECTIONS);

    auto state_connection = mod_connections.getChildWithProperty (IDs::dest, mod_dst.getProperty (IDs::nodeID));
    //    connection->state.removeFromParent();

    bitklavier::StateConnection* connection = getStateConnection (source, destination);
    bool create = connection == nullptr;

    if (create)
    {
        connection = getStateBank().createConnection (source, destination);
        state_connections_.push_back (connection);
        state_connection.appendChild (connection->state, nullptr);
    }
    // if (connection)
    //     connectStateModulation (connection);
    return create;
}

CompressorProcessor *SynthBase::getCompressorProcessor() {
    return compressorProcessor.get();
}

EQProcessor *SynthBase::getEQProcessor() {
    return eqProcessor.get();
}

GainProcessor *SynthBase::getMainVolumeProcessor() {
    return gainProcessor.get();
}

