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

#include "ModulationConnection.h"
namespace bitklavier {
    class ModulationProcessor;
    using AudioGraphIOProcessor = juce::AudioProcessorGraph::AudioGraphIOProcessor;
    using Node = juce::AudioProcessorGraph::Node;
  class SoundEngine   {
    public:
      static constexpr int kDefaultOversamplingAmount = 2;
      static constexpr int kDefaultSampleRate = 44100;

      SoundEngine();
      virtual ~SoundEngine();


//      void process(int num_samples, juce::AudioSampleBuffer& buffer);

      void releaseResources() {processorGraph->releaseResources();}
      void resetEngine() { prepareToPlay(curr_sample_rate, buffer_size);}
      void prepareToPlay(double sampleRate, int samplesPerBlock)
      {
          setSampleRate(sampleRate);
          setBufferSize(samplesPerBlock);
          processorGraph->prepareToPlay (sampleRate, samplesPerBlock);
          initialiseGraph();
      }

      int getDefaultSampleRate() { return kDefaultSampleRate; }

      int getSampleRate()
      {
          return curr_sample_rate;
      }

      void setSampleRate(int sampleRate)
      {
          curr_sample_rate = sampleRate;
      }

      void setBufferSize(int bufferSize)
      {
          buffer_size = bufferSize;
      }

      int getBufferSize()
      {
          return buffer_size;
      }

      void connectMidiNodes()
      {
          processorGraph->addConnection ({ { midiInputNode->nodeID,  juce::AudioProcessorGraph::midiChannelIndex },
                                          { midiOutputNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex } });
      }
      void initialiseGraph()
      {
          processorGraph->clear();
          lastUID = juce::AudioProcessorGraph::NodeID(0);
          audioOutputNode = processorGraph->addNode (std::make_unique<AudioGraphIOProcessor> (AudioGraphIOProcessor::audioOutputNode),getNextUID());
          midiInputNode   = processorGraph->addNode (std::make_unique<AudioGraphIOProcessor> (AudioGraphIOProcessor::midiInputNode),getNextUID());
          midiOutputNode  = processorGraph->addNode (std::make_unique<AudioGraphIOProcessor> (AudioGraphIOProcessor::midiOutputNode),getNextUID());

          connectMidiNodes();
      }

      juce::AudioProcessorGraph::NodeID lastUID;

      juce::AudioProcessorGraph::NodeID getNextUID() noexcept
      {
          return juce::AudioProcessorGraph::NodeID (++(lastUID.uid));
      }

      Node::Ptr addNode (std::unique_ptr<juce::AudioProcessor> newProcessor, juce::AudioProcessorGraph::NodeID id)
      {
          Node::Ptr node;
          if(id.uid != 0) {
              lastUID = id;
              node = processorGraph->addNode(std::move(newProcessor), id);
          }
          else
              node = processorGraph->addNode(std::move(newProcessor), getNextUID());

          auto processor = node->getProcessor();
          if (processor->getMainBusNumOutputChannels() > 0)
          {
              processorGraph->addConnection({{node->nodeID,0 }, {audioOutputNode->nodeID, 0}});
              processorGraph->addConnection({{node->nodeID,1 }, {audioOutputNode->nodeID, 1}});
          }

          return node;
      }

      Node::Ptr addNode(std::unique_ptr<bitklavier::ModulationProcessor> modProcessor, juce::AudioProcessorGraph::NodeID id);



        ModulationConnectionBank& getModulationBank() { return modulation_bank_; }
      void processAudioAndMidi(juce::AudioBuffer<float>& audio_buffer, juce::MidiBuffer& midi_buffer)
      {
          processorGraph->processBlock(audio_buffer, midi_buffer);
      }
     void setInputsOutputs(int newNumIns, int newNumOuts)
     {
          processorGraph->setPlayConfigDetails(newNumIns, newNumOuts, curr_sample_rate, buffer_size);
     }
        juce::AudioProcessorGraph::Node * getNodeForId(juce::AudioProcessorGraph::NodeID id)
     {
         return processorGraph->getNodeForId(id);
     }
     void addConnection(juce::AudioProcessorGraph::Connection& connection)
     {
          processorGraph->addConnection(connection);
     }
     bool isConnected(juce::AudioProcessorGraph::Connection& connection)
     {
          return processorGraph->isConnected(connection);
     }
     bool isConnected(juce::AudioProcessorGraph::NodeID src, juce::AudioProcessorGraph::NodeID dest)
     {
            return processorGraph->isConnected(src,dest);
     }

     void addChangeListener(juce::ChangeListener* listener)
     {
          processorGraph->addChangeListener(listener);
     }
    private:
      void setOversamplingAmount(int oversampling_amount, int sample_rate);
      int last_oversampling_amount_;
      int last_sample_rate_;
      int buffer_size;
      int curr_sample_rate;

        std::unique_ptr<juce::AudioProcessorGraph>  processorGraph;

        Node::Ptr audioOutputNode;
        Node::Ptr midiInputNode;
        Node::Ptr midiOutputNode;
        ModulationConnectionBank modulation_bank_;
      JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SoundEngine)
  };
} // namespace vital

