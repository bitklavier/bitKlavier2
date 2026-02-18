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

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

#include "CompressorProcessor.h"
#include "DirectProcessor.h"
#include "EQProcessor.h"
#include "GainProcessor.h"
#include "ModulationConnection.h"
#include "ModulationProcessor.h"
#include "NostalgicProcessor.h"
#include "ResonanceProcessor.h"
#include "SynchronicProcessor.h"
#include "TuningProcessor.h"
#include "midi_manager.h"
#include "synth_base.h"
#include <vector>

namespace bitklavier
{
    class ModulationProcessor;
    using AudioGraphIOProcessor = juce::AudioProcessorGraph::AudioGraphIOProcessor;
    using Node = juce::AudioProcessorGraph::Node;

    class SoundEngine
    {
    public:
        static constexpr int kDefaultOversamplingAmount = 2;
        static constexpr int kDefaultSampleRate = 44100;

        SoundEngine(SynthBase& , juce::ValueTree &);
        virtual ~SoundEngine();

        //      void process(int num_samples, juce::AudioSampleBuffer& buffer);

        void releaseResources() { processorGraph->releaseResources(); }
        void resetEngine() { prepareToPlay (curr_sample_rate, buffer_size); }
        void prepareToPlay (double sampleRate, int samplesPerBlock)
        {
            setSampleRate (sampleRate);
            setBufferSize (samplesPerBlock);
            processorGraph->prepareToPlay (sampleRate, samplesPerBlock);
            gainProcessor->prepareToPlay (sampleRate, samplesPerBlock);
            eqProcessor->prepareToPlay (sampleRate, samplesPerBlock);
            compressorProcessor->prepareToPlay (sampleRate, samplesPerBlock);
        }

        int getDefaultSampleRate() { return kDefaultSampleRate; }

        int getSampleRate()
        {
            return curr_sample_rate;
        }

        void setSampleRate (int sampleRate)
        {
            curr_sample_rate = sampleRate;
        }

        void setBufferSize (int bufferSize)
        {
            buffer_size = bufferSize;
        }

        int getBufferSize()
        {
            return buffer_size;
        }

        void connectMidiNodes()
        {
            processorGraph->addConnection ({ { midiInputNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex },
                { midiOutputNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex } });
        }

        void initialiseGraph()
        {
            processorGraph->clear();
            lastUID = juce::AudioProcessorGraph::NodeID (0);
            audioOutputNode = processorGraph->addNode (std::make_unique<AudioGraphIOProcessor> (AudioGraphIOProcessor::audioOutputNode), getNextUID());
            midiInputNode = processorGraph->addNode (std::make_unique<AudioGraphIOProcessor> (AudioGraphIOProcessor::midiInputNode), getNextUID());
            midiOutputNode = processorGraph->addNode (std::make_unique<AudioGraphIOProcessor> (AudioGraphIOProcessor::midiOutputNode), getNextUID());

            connectMidiNodes();

        }
        void addDefaultChain(SynthBase& parent, juce::ValueTree& tree);

        // Inject UI-generated MIDI to all KeymapProcessors in the graph
        void postUINoteOn  (int midiNote, float velocity01, int channel = 1);
        void postUINoteOff (int midiNote, float velocity01 = 0.0f, int channel = 1);

        // UI live MIDI visualisation registration: forwards to all KeymapProcessors' MidiManagers
        void addMidiLiveListener (MidiManager::LiveMidiListener* l);
        void removeMidiLiveListener (MidiManager::LiveMidiListener* l);

        // Register all stored UI listeners onto a specific MidiManager (used by processors on construction)
        void registerLiveListenersTo (MidiManager* midiMgr);

        juce::AudioProcessorGraph::NodeID lastUID;

        // Registry of UI live MIDI listeners to attach to future MidiManagers
        std::vector<MidiManager::LiveMidiListener*> live_ui_listeners_;

        juce::AudioProcessorGraph::NodeID getNextUID() noexcept
        {
            return juce::AudioProcessorGraph::NodeID (++(lastUID.uid));
        }

        Node::Ptr addNode (std::unique_ptr<juce::AudioProcessor> newProcessor, juce::AudioProcessorGraph::NodeID id)
        {
            Node::Ptr node;

            // 1. Add the node to the graph using the provided ID or generate a new one
            if (id.uid != 0)
            {
                lastUID = id;
                node = processorGraph->addNode (std::move (newProcessor), id);
            }
            else
            {
                node = processorGraph->addNode (std::move (newProcessor), getNextUID());
            }

            auto processor = node->getProcessor();

            // 2. Identification: Should this processor's audio be automatically wired to the main output?
            // We skip processors that are strictly for modulation (like ModulationProcessor)
            // because their "Main Bus" might contain non-audio signals.
            bool isModulationProcessor = (dynamic_cast<bitklavier::ModulationProcessor*> (processor) != nullptr);

            if (!isModulationProcessor && processor->getMainBusNumOutputChannels() > 0)
            {
                /*
                 * RATIONALE:
                 * We want to connect the 'Main Output Bus' of this processor to the 'Main Output' of the graph.
                 * Using absolute channel indices 0 and 1 is dangerous for multi-bus processors
                 * because those indices might point to modulation or sidechain buses if the
                 * 'Main' bus isn't the first one or if it isn't stereo.
                 */

                // Get the main output bus (usually index 0)
                if (auto* mainBus = processor->getBus (false, 0))
                {
                    int numChannels = mainBus->getNumberOfChannels();

                    // Connect each channel of the Main Bus to the corresponding channel of the audioOutputNode
                    for (int ch = 0; ch < numChannels; ++ch)
                    {
                        // getChannelIndexInProcessBlockBuffer converts the bus-relative channel index
                        // to the absolute index required by the AudioProcessorGraph::Connection.
                        int absoluteSourceChannel = processor->getChannelIndexInProcessBlockBuffer (false, 0, ch);

                        // Usually we only have 2 channels of output (L/R)
                        if (ch < 2)
                        {
                            processorGraph->addConnection ({
                                { node->nodeID, absoluteSourceChannel },
                                { audioOutputNode->nodeID, ch }
                            });
                        }
                    }

                    DBG ("SoundEngine: Connected " + juce::String (numChannels) +
                         " channels from " + processor->getName() + " to main output.");
                }
            }

            return node;
        }


        // Node::Ptr addNode (std::unique_ptr<juce::AudioProcessor> newProcessor, juce::AudioProcessorGraph::NodeID id)
        // {
        //     Node::Ptr node;
        //     if (id.uid != 0)
        //     {
        //         lastUID = id;
        //         node = processorGraph->addNode (std::move (newProcessor), id);
        //     }
        //     else
        //         node = processorGraph->addNode (std::move (newProcessor), getNextUID());
        //
        //     auto processor = node->getProcessor();
        //     if (processor->getMainBusNumOutputChannels() > 0)
        //     {
        //         DBG("adddmainoutconneciton");
        //         processorGraph->addConnection ({ { node->nodeID, 0 }, { audioOutputNode->nodeID, 0 } });
        //         processorGraph->addConnection ({ { node->nodeID, 1 }, { audioOutputNode->nodeID, 1 } });
        //     }
        //
        //     return node;
        // }

        juce::AudioProcessorGraph::Node::Ptr removeNode (juce::AudioProcessorGraph::NodeID id)
        {
            if(processorGraph)
                return processorGraph->removeNode (id);
            else
                return nullptr;
        }

        Node::Ptr addNode (std::unique_ptr<ModulationProcessor> modProcessor, juce::AudioProcessorGraph::NodeID id);

        void setActivePiano (const juce::ValueTree& v);

        ModulationConnectionBank& getModulationBank() { return modulation_bank_; }
        StateConnectionBank& getStateBank() { return state_bank_; }
        ParamOffsetBank& getParamOffsetBank() {return param_offset_bank_; }

        /**
         * Updates the processors of all relevant nodes in the graph when
         * parameters in the Gallery Settings have been changed (marked "dirty")
         * - Currently just for A440 and tempo multiplier settings
         */
        void updateChangedGalleryState ()
        {
            if (a4Dirty.exchange (false, std::memory_order_acq_rel))
            {
                const double newA4 = pendingA4Hz.load (std::memory_order_relaxed);

                // Walk all nodes and update their processors
                auto nodes = processorGraph->getNodes();
                for (auto* node : nodes)
                {
                    if (auto* kp = dynamic_cast<DirectProcessor*>(node->getProcessor()))
                    {
                        kp->setA4Frequency(newA4);
                    }
                    else if (auto* kp = dynamic_cast<SynchronicProcessor*>(node->getProcessor()))
                    {
                        kp->setA4Frequency(newA4);
                    }
                    else if (auto* kp = dynamic_cast<NostalgicProcessor*>(node->getProcessor()))
                    {
                        kp->setA4Frequency(newA4);
                    }
                    else if (auto* kp = dynamic_cast<ResonanceProcessor*>(node->getProcessor()))
                    {
                        kp->setA4Frequency(newA4);
                    }
                    else if (auto* kp = dynamic_cast<TuningProcessor*>(node->getProcessor()))
                    {
                        kp->setA4Frequency(newA4);
                    }
                }
            }

            if (tempoMultiplierDirty.exchange (false, std::memory_order_acq_rel))
            {
                const double newTM = pendingTempoMultiplier.load (std::memory_order_relaxed);
                auto nodes = processorGraph->getNodes();
                for (auto* node : nodes)
                {
                    if (auto* kp = dynamic_cast<TempoProcessor*>(node->getProcessor()))
                    {
                        kp->setGlobalTempoMultiplier(newTM);
                    }
                }
            }
        }

        void processAudioAndMidi (juce::AudioBuffer<float>& audio_buffer, juce::MidiBuffer& midi_buffer)
        {
            // update all the processors with any changes to params in Gallery Settings
            updateChangedGalleryState ();

            //DBG ("------------------BEGIN BLOCK-------------------");
            processorGraph->processBlock (audio_buffer, midi_buffer);
        }

        void setInputsOutputs (int newNumIns, int newNumOuts)
        {
            processorGraph->setPlayConfigDetails (newNumIns, newNumOuts, curr_sample_rate, buffer_size);
        }

        juce::AudioProcessorGraph::Node* getNodeForId (juce::AudioProcessorGraph::NodeID id)
        {
            return processorGraph->getNodeForId (id);
        }

        static void dbgPrintConnectionsForNode (const juce::AudioProcessorGraph& graph,
                                        juce::AudioProcessorGraph::NodeID nodeID)
        {
            const auto connections = graph.getConnections();

            DBG ("=== Connections for node " +  juce::String(nodeID.uid ) + " ===");

            for (const auto& c : connections)
            {
                const bool isSource = (c.source.nodeID == nodeID);
                const bool isDest   = (c.destination.nodeID == nodeID);

                if (!isSource && !isDest)
                    continue;

                auto chanToString = [] (juce::AudioProcessorGraph::NodeAndChannel nc)
                {
                    if (nc.channelIndex == juce::AudioProcessorGraph::midiChannelIndex)
                        return juce::String ("MIDI");

                    return juce::String ("ch ") + juce::String (nc.channelIndex);
                };

                DBG (juce::String (isSource ? "OUT " : "IN  ")
                     + " node " + juce::String ( c.source.nodeID.uid)
                     + " [" + chanToString (c.source) + "]  ->  "
                     + "node " + juce::String ( c.destination.nodeID.uid)
                     + " [" + chanToString (c.destination) + "]");
            }
        }

        // bool addConnection (juce::AudioProcessorGraph::Connection& connection)
        // {
        //     if(processorGraph == nullptr)
        //         return false;
        //     dbgPrintConnectionsForNode(*processorGraph, connection.source.nodeID);
        //     if(connection.source.channelIndex == 0 or connection.source.channelIndex == 1
        //         && connection.destination.nodeID != audioOutputNode->nodeID) {
        //         processorGraph->removeConnection({{connection.source.nodeID, 0},{audioOutputNode->nodeID,0}});
        //         processorGraph->removeConnection({{connection.source.nodeID, 1},{audioOutputNode->nodeID,1}});
        //         DBG("[Graph] Removed previous stereo connections from node " + juce::String(connection.source.nodeID.uid)
        //             + " to audio output node " + juce::String(audioOutputNode->nodeID.uid));
        //     }
        //
        //     if (!connection.source.isMIDI())
        //         processorGraph->addConnection({{connection.source.nodeID, connection.source.channelIndex+1},
        //             {connection.destination.nodeID,connection.destination.channelIndex+1}});
        //
        //     return processorGraph->addConnection (connection);
        // }

        bool addConnection (juce::AudioProcessorGraph::Connection& connection)
        {
            if (processorGraph == nullptr)
                return false;

            auto* srcNode = processorGraph->getNodeForId (connection.source.nodeID);
            auto* dstNode = processorGraph->getNodeForId (connection.destination.nodeID);

            if (srcNode == nullptr || dstNode == nullptr)
                return false;

            auto* srcProc = srcNode->getProcessor();
            auto* dstProc = dstNode->getProcessor();

            // --- 1. SMART MIDI AUTO-CONNECTION ---
            // If this is an audio connection, and both nodes support MIDI,
            // ensure the MIDI bus is also connected.
            if (!connection.source.isMIDI() && dstProc->acceptsMidi() && srcProc->producesMidi())
            {
                juce::AudioProcessorGraph::Connection midiConn {
                    { connection.source.nodeID, juce::AudioProcessorGraph::midiChannelIndex },
                    { connection.destination.nodeID, juce::AudioProcessorGraph::midiChannelIndex }
                };

                // Corrected isConnected call using the Connection struct
                if (!processorGraph->isConnected (midiConn))
                {
                    processorGraph->addConnection (midiConn);
                }
            }

            // --- 2. SMART STEREO PAIRING ---
            // We only pair if the channel belongs to the "Main" output bus (index 0).

            if (!connection.source.isMIDI())
            {
                // Find if this channel is part of the main output bus
                int mainBusNumChannels = 0;
                if (auto* mainBus = srcProc->getBus (false, 0))
                    mainBusNumChannels = mainBus->getNumberOfChannels();

                // If the main bus is stereo (or more) and we are connecting the first channel (0),
                // automatically connect the second channel (1).
                if (mainBusNumChannels > 1 && connection.source.channelIndex == 0)
                {
                     processorGraph->addConnection ({
                        { connection.source.nodeID, 1 },
                        { connection.destination.nodeID, 1 }
                    });
                }
            }

            // --- 3. OUTPUT CLEANUP ---
            // If we are custom-wiring main audio, remove the default auto-wires to output.
            if ((connection.source.channelIndex == 0 || connection.source.channelIndex == 1)
                && connection.destination.nodeID != audioOutputNode->nodeID)
            {
                processorGraph->removeConnection ({{connection.source.nodeID, 0}, {audioOutputNode->nodeID, 0}});
                processorGraph->removeConnection ({{connection.source.nodeID, 1}, {audioOutputNode->nodeID, 1}});
            }

            // --- 4. FINAL PRIMARY CONNECTION ---
            return processorGraph->addConnection (connection);
        }

        // void removeConnection (const juce::AudioProcessorGraph::Connection& connection)
        // {
        //     if(processorGraph == nullptr)
        //         return;
        //
        //     if (!connection.source.isMIDI())
        //         processorGraph->removeConnection({{connection.source.nodeID, connection.source.channelIndex+1},
        //            {connection.destination.nodeID,connection.destination.channelIndex+1}});
        //
        //     processorGraph->removeConnection (connection);
        //
        //     if(connection.source.channelIndex == 0 or connection.source.channelIndex == 1
        //      && connection.destination.nodeID != audioOutputNode->nodeID) {
        //         if (!processorGraph->isAnInputTo(connection.source.nodeID,audioOutputNode->nodeID)) {
        //             processorGraph->addConnection ({ { connection.source.nodeID, 0 }, { audioOutputNode->nodeID, 0 } });
        //             processorGraph->addConnection ({ { connection.source.nodeID, 1 }, { audioOutputNode->nodeID, 1 } });
        //             DBG ("[Graph] Reconnected node "
        //          + juce::String(connection.source.nodeID.uid) + "  to audio output node "
        //          + juce::String(audioOutputNode->nodeID.uid));
        //         }
        //      }
        // }

        void removeConnection (const juce::AudioProcessorGraph::Connection& connection)
        {
            if (processorGraph == nullptr)
                return;

            // 1. Remove the primary connection as requested
            processorGraph->removeConnection (connection);

            // 2. SYMMETRICAL STEREO CLEANUP
            // If we are removing the first channel (0) of a Main Bus,
            // we should also remove the automatic second channel (1) connection.
            if (!connection.source.isMIDI())
            {
                auto* srcNode = processorGraph->getNodeForId (connection.source.nodeID);
                if (srcNode != nullptr)
                {
                    auto* srcProc = srcNode->getProcessor();
                    int mainBusNumChannels = 0;
                    if (auto* mainBus = srcProc->getBus (false, 0))
                        mainBusNumChannels = mainBus->getNumberOfChannels();

                    if (mainBusNumChannels > 1 && connection.source.channelIndex == 0)
                    {
                        processorGraph->removeConnection ({
                            { connection.source.nodeID, 1 },
                            { connection.destination.nodeID, 1 }
                        });
                    }
                }
            }

            // 3. RESTORE DEFAULT OUTPUT CONNECTIONS
            // If the node's main audio (channels 0 or 1) is no longer connected
            // to any other internal node, wire it back to the final speakers.
            if (connection.source.channelIndex == 0 || connection.source.channelIndex == 1)
            {
                // Only restore if the node has no other outgoing connections
                // leading to the final audio output node.
                if (!processorGraph->isAnInputTo (connection.source.nodeID, audioOutputNode->nodeID))
                {
                    processorGraph->addConnection ({{connection.source.nodeID, 0}, {audioOutputNode->nodeID, 0}});
                    processorGraph->addConnection ({{connection.source.nodeID, 1}, {audioOutputNode->nodeID, 1}});
                }
            }

            // NOTE: We generally do NOT automatically remove the MIDI connection here.
            // Since MIDI is shared, multiple audio connections might be relying on it.
            // It's safer to let the MIDI connection be managed explicitly or cleaned
            // up when the node itself is removed.
        }

        bool isConnected (juce::AudioProcessorGraph::Connection& connection)
        {
            if(processorGraph)
                return processorGraph->isConnected (connection);
            return false;
        }

        bool isConnected (juce::AudioProcessorGraph::NodeID src, juce::AudioProcessorGraph::NodeID dest)
        {
            if(processorGraph)
                return processorGraph->isConnected (src, dest);
            return false;
        }

        void addChangeListener (juce::ChangeListener* listener)
        {
            if(processorGraph)
                processorGraph->addChangeListener (listener);
        }

        void requestResetAllContinuousModsRT();

        void allNotesOff();
        void shutdown();

        CompressorProcessor* getCompressorProcessor()
        {
            return compressorProcessor.get();
        };
        EQProcessor* getEQProcessor()
        {
            return eqProcessor.get();
        };
        GainProcessor* getMainVolumeProcessor()
        {
            return gainProcessor.get();
        };

        void requestA4Update (double newA4) noexcept
        {
            pendingA4Hz.store (newA4, std::memory_order_relaxed);
            a4Dirty.store (true, std::memory_order_release);
        }

        void requestTempoMultiplierUpdate (double newTm) noexcept
        {
            pendingTempoMultiplier.store (newTm, std::memory_order_relaxed);
            tempoMultiplierDirty.store (true, std::memory_order_release);
        }


    private:
        void setOversamplingAmount (int oversampling_amount, int sample_rate);
        int last_oversampling_amount_;
        int last_sample_rate_;
        int buffer_size;
        int curr_sample_rate;

        std::atomic<double> pendingA4Hz { 440.0 };
        std::atomic<bool>   a4Dirty { false };

        std::atomic<double> pendingTempoMultiplier { 1.0 };
        std::atomic<bool>   tempoMultiplierDirty { false };

        std::unique_ptr<juce::AudioProcessorGraph> processorGraph;

        Node::Ptr audioOutputNode;
        Node::Ptr midiInputNode;
        Node::Ptr midiOutputNode;
        ModulationConnectionBank modulation_bank_;
        StateConnectionBank state_bank_;
        ParamOffsetBank param_offset_bank_;
        std::unique_ptr<GainProcessor> gainProcessor ;
        std::unique_ptr<CompressorProcessor> compressorProcessor ;
        std::unique_ptr<EQProcessor> eqProcessor ;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SoundEngine)
    };
} // namespace vital
