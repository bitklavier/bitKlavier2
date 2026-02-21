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

#include "sound_engine.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include "KeymapProcessor.h"
#include "ModulationProcessor.h"
#include <algorithm>

namespace bitklavier {

    SoundEngine::SoundEngine(SynthBase& parent, juce::ValueTree& tree) :
    /*voice_handler_(nullptr),*/
            last_oversampling_amount_(-1), last_sample_rate_(-1),
            processorGraph(std::make_unique<juce::AudioProcessorGraph>())
        {
        initialiseGraph();
        processorGraph->enableAllBuses();
    }

    SoundEngine::~SoundEngine() {}
    void  SoundEngine::shutdown() {
        allNotesOff();
        processorGraph->clear();
        processorGraph->rebuild();
        processorGraph.reset();
    }

    void SoundEngine::setOversamplingAmount(int oversampling_amount, int sample_rate) {
        static constexpr int kBaseSampleRate = 44100;

        int oversample = oversampling_amount;
        int sample_rate_mult = sample_rate / kBaseSampleRate;
        while (sample_rate_mult > 1 && oversample > 1) {
            sample_rate_mult >>= 1;
            oversample >>= 1;
        }
        //voice_handler_->setOversampleAmount(oversample);

        last_oversampling_amount_ = oversampling_amount;
        last_sample_rate_ = sample_rate;
    }

//    Node::Ptr SoundEngine::addNode(std::unique_ptr<bitklavier::ModulationProcessor> modProcessor,
//                                   juce::AudioProcessorGraph::NodeID id) {
//    }

    void SoundEngine::setActivePiano(const juce::ValueTree &v) {
        DBG("***********SoundEngine::setActivePiano SWITCH PIANOS********************");
        DBG("*********curr piano is " + v.getProperty(IDs::name).toString() + " ***********************");
        jassert(v.hasType(IDs::PIANO));
        auto nodes = processorGraph->getNodes();
        //skip input output nodes
        const auto & curr_piano = v.getChildWithName(IDs::PREPARATIONS);

        /**
         * start with the 4th node.
         *  - the first three are initialized in initialiseGraph() in sound_engine.h
         *      - node 0: audio output to DAW
         *      - nodes 1-2: MIDI in and out from/to DAW
         *      - eventually we will want a 4th, to take audio in from DAW
         */
        for (int i = 3; i < nodes.size(); i++){
            auto node = nodes[i];
            auto tree = curr_piano.getChildWithProperty(IDs::nodeID,
                juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::toVar(node->nodeID));
            if (tree.isValid()) {
                node->setBypassed(false);
            } else {
                node->setBypassed(true);
            }
        }
    }

    void bitklavier::SoundEngine::requestResetAllContinuousModsRT()
    {
        DBG("SoundEngine::requestResetAllContinuousModsRT()");
        auto nodes = processorGraph->getNodes();
        for (auto* node : nodes)
            if (auto* mp = dynamic_cast<bitklavier::ModulationProcessor*>(node->getProcessor()))
                mp->requestResetAllContinuousModsRT();
    }

    void SoundEngine::allNotesOff() {
        auto nodes = processorGraph->getNodes();
        for (auto node : nodes) {
            if (auto* a = dynamic_cast<KeymapProcessor*>(node->getProcessor()))
                { a->allNotesOff();}

        }
    }

    void SoundEngine::postUINoteOn (int midiNote, float velocity01, int channel)
    {
        const auto ts = juce::Time::getMillisecondCounterHiRes() * 0.001;
        juce::MidiMessage msg = juce::MidiMessage::noteOn (channel,
                                                           midiNote,
                                                           (juce::uint8) juce::jlimit (0, 127, (int) std::lround (velocity01 * 127.0f)));
        msg.setTimeStamp (ts);

        auto nodes = processorGraph->getNodes();
        for (auto* node : nodes)
            if (auto* kp = dynamic_cast<KeymapProcessor*> (node->getProcessor()))
                kp->postExternalMidi (msg);
    }

    void SoundEngine::postUINoteOff (int midiNote, float velocity01, int channel)
    {
        const auto ts = juce::Time::getMillisecondCounterHiRes() * 0.001;
        juce::MidiMessage msg = juce::MidiMessage::noteOff (channel,
                                                            midiNote,
                                                            (juce::uint8) juce::jlimit (0, 127, (int) std::lround (velocity01 * 127.0f)));
        msg.setTimeStamp (ts);

        auto nodes = processorGraph->getNodes();
        for (auto* node : nodes)
            if (auto* kp = dynamic_cast<KeymapProcessor*> (node->getProcessor()))
                kp->postExternalMidi (msg);
    }

    void SoundEngine::injectHostMidi (const juce::MidiBuffer& midiMessages)
    {
        if (midiMessages.isEmpty())
            return;

        auto nodes = processorGraph->getNodes();
        for (auto* node : nodes)
        {
            if (auto* kp = dynamic_cast<KeymapProcessor*> (node->getProcessor()))
            {
                for (const auto metadata : midiMessages)
                    kp->postExternalMidi (metadata.getMessage());
            }
        }
    }

    void SoundEngine::addMidiLiveListener (MidiManager::LiveMidiListener* l)
    {
        // Store for future processors and attach to all current ones
        if (l)
        {
            // prevent duplicates
            if (std::find(live_ui_listeners_.begin(), live_ui_listeners_.end(), l) == live_ui_listeners_.end())
                live_ui_listeners_.push_back(l);

            auto nodes = processorGraph->getNodes();
            for (auto* node : nodes)
                if (auto* kp = dynamic_cast<KeymapProcessor*> (node->getProcessor()))
                    if (kp->_midi)
                        kp->_midi->addLiveMidiListener (l);
        }
    }

    void SoundEngine::removeMidiLiveListener (MidiManager::LiveMidiListener* l)
    {
        if(!processorGraph)
            return;
        auto nodes = processorGraph->getNodes();
        for (auto* node : nodes)
            if (auto* kp = dynamic_cast<KeymapProcessor*> (node->getProcessor()))
                if (kp->_midi)
                    kp->_midi->removeLiveMidiListener (l);

        // remove from registry
        live_ui_listeners_.erase(std::remove(live_ui_listeners_.begin(), live_ui_listeners_.end(), l), live_ui_listeners_.end());
    }

    void SoundEngine::registerLiveListenersTo (MidiManager* midiMgr)
    {
        if (!midiMgr) return;
        for (auto* l : live_ui_listeners_)
            midiMgr->addLiveMidiListener(l);
    }

    void SoundEngine::restoreDefaultOutputConnections()
    {
        if (processorGraph == nullptr)
            return;

        auto nodes = processorGraph->getNodes();
        for (auto* node : nodes)
        {
            if (node->nodeID == audioOutputNode->nodeID || 
                node->nodeID == midiInputNode->nodeID || 
                node->nodeID == midiOutputNode->nodeID)
                continue;

            auto* processor = node->getProcessor();
            if (processor == nullptr)
                continue;

            // Skip modulation processors for auto-audio-output
            if (dynamic_cast<bitklavier::ModulationProcessor*> (processor) != nullptr)
                continue;

            if (processor->getMainBusNumOutputChannels() > 0)
            {
                // Check if this node has ANY outgoing audio connections.
                // If it doesn't, it might be a final node that needs to be connected to the output.
                bool hasAudioOutput = false;
                const auto connections = processorGraph->getConnections();
                for (const auto& c : connections)
                {
                    if (c.source.nodeID == node->nodeID && !c.source.isMIDI())
                    {
                        hasAudioOutput = true;
                        break;
                    }
                }

                if (!hasAudioOutput)
                {
                    if (auto* mainBus = processor->getBus (false, 0))
                    {
                        int numChannels = mainBus->getNumberOfChannels();
                        for (int ch = 0; ch < numChannels && ch < 2; ++ch)
                        {
                            int absIdx = processor->getChannelIndexInProcessBlockBuffer (false, 0, ch);
                            processorGraph->addConnection ({{node->nodeID, absIdx}, {audioOutputNode->nodeID, ch}});
                        }
                        
                        // DBG ("SoundEngine: Restored default output for " + processor->getName());
                    }
                }
            }
        }
    }

    void SoundEngine::addDefaultChain(SynthBase& parent, juce::ValueTree& tree) {
        if (gainProcessor || compressorProcessor || eqProcessor)
            return;
        juce::ValueTree buseq (IDs::BUSEQ);
        juce::ValueTree buscompressor (IDs::BUSCOMPRESSOR);
        tree.appendChild (buseq, nullptr);
        tree.appendChild (buscompressor, nullptr);
        buseq.setProperty(IDs::type, bitklavier::BKPreparationType::PreparationTypeEQ, nullptr);
        buscompressor.setProperty(IDs::type, bitklavier::BKPreparationType::PreparationTypeCompressor, nullptr);
        // gainProcessor = std::make_unique<GainProcessor>(parent,tree);
        // compressorProcessor = std::make_unique<CompressorProcessor>(parent,buseq);
        // eqProcessor = std::make_unique<EQProcessor>(parent,buscompressor);
        gainProcessor       = std::make_unique<GainProcessor>(parent, tree, &parent.getUndoManager());
        compressorProcessor = std::make_unique<CompressorProcessor>(parent, buseq, &parent.getUndoManager());
        eqProcessor         = std::make_unique<EQProcessor>(parent, buscompressor, &parent.getUndoManager());
    }
} // namespace bitklavier
