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
#include "../framework/note_handler.h"
#include <juce_audio_processors/juce_audio_processors.h>


namespace bitklavier {

    using AudioGraphIOProcessor = juce::AudioProcessorGraph::AudioGraphIOProcessor;
    using Node = juce::AudioProcessorGraph::Node;
  class SoundEngine : public NoteHandler {
    public:
      static constexpr int kDefaultOversamplingAmount = 2;
      static constexpr int kDefaultSampleRate = 44100;

      SoundEngine();
      virtual ~SoundEngine();

//      void init() override;
      void processWithInput(const mono_float* audio_in, int num_samples) {
          //_ASSERT(num_samples <= output()->buffer_size);
          juce::FloatVectorOperations::disableDenormalisedNumberSupport();
      }

      void process(int num_samples, juce::AudioSampleBuffer& buffer);

      void releaseResources()
      {processorGraph->releaseResources();}
      void resetEngine()
      {
          prepareToPlay(curr_sample_rate, buffer_size);
      }
      void prepareToPlay(double sampleRate, int samplesPerBlock)
      {
          setSampleRate(sampleRate);
          // DBG("setting sample rate to: " + juce::String(sampleRate));
          setBufferSize(samplesPerBlock);

          processorGraph->prepareToPlay (sampleRate, samplesPerBlock);
          initialiseGraph();
      }

      //void correctToTime(double seconds) override;
      int getDefaultSampleRate()
      {
          return kDefaultSampleRate;
      }

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

      int getNumPressedNotes();

      int getNumActiveVoices();

      mono_float getLastActiveNote() const;

      void allSoundsOff() override;
      void allNotesOff(int sample) override;
      void allNotesOff(int sample, int channel) override;
      void allNotesOffRange(int sample, int from_channel, int to_channel);

      void noteOn(int note, mono_float velocity, int sample, int channel) override;
      void noteOff(int note, mono_float lift, int sample, int channel) override;
      void setModWheel(mono_float value, int channel);
      void setModWheelAllChannels(mono_float value);
      void setPitchWheel(mono_float value, int channel);
      void setZonedPitchWheel(mono_float value, int from_channel, int to_channel);


      //void setBpm(mono_float bpm);
      void setAftertouch(mono_float note, mono_float value, int sample, int channel);
      void setChannelAftertouch(int channel, mono_float value, int sample);
      void setChannelRangeAftertouch(int from_channel, int to_channel, mono_float value, int sample);
      void setChannelSlide(int channel, mono_float value, int sample);
      void setChannelRangeSlide(int from_channel, int to_channel, mono_float value, int sample);

      void sustainOn(int channel);
      void sustainOff(int sample, int channel);
      void sostenutoOn(int channel);
      void sostenutoOff(int sample, int channel);

      void sustainOnRange(int from_channel, int to_channel);
      void sustainOffRange(int sample, int from_channel, int to_channel);
      void sostenutoOnRange(int from_channel, int to_channel);
      void sostenutoOffRange(int sample, int from_channel, int to_channel);
      force_inline int getOversamplingAmount() const { return last_oversampling_amount_; }
      void connectAudioNodes()
      {
//          for (int channel = 0; channel < 2; ++channel)
//              mainProcessor->addConnection ({ { audioInputNode->nodeID,  channel },
//                                              { audioOutputNode->nodeID, channel } });
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
          //audioInputNode  = mainProcessor->addNode (std::make_unique<AudioGraphIOProcessor> (AudioGraphIOProcessor::audioInputNode));
          audioOutputNode = processorGraph->addNode (std::make_unique<AudioGraphIOProcessor> (AudioGraphIOProcessor::audioOutputNode),getNextUID());
          midiInputNode   = processorGraph->addNode (std::make_unique<AudioGraphIOProcessor> (AudioGraphIOProcessor::midiInputNode),getNextUID());
          midiOutputNode  = processorGraph->addNode (std::make_unique<AudioGraphIOProcessor> (AudioGraphIOProcessor::midiOutputNode),getNextUID());

          connectAudioNodes();
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
          if (processor->getTotalNumOutputChannels() > 0)
          {
//              auto busses =processor->getBusesLayout();
//              auto channelset = busses.outputBuses;
//              for (auto channels : channelset)
//              {
//                  channels.getChannelIndexForType()
//              }
              processorGraph->addConnection({{node->nodeID,0 }, {audioOutputNode->nodeID, 0}});
              processorGraph->addConnection({{node->nodeID,1 }, {audioOutputNode->nodeID, 1}});
          }

          return node;
      }

      void checkOversampling();
      std::vector<std::shared_ptr<juce::AudioProcessor>> processors;
      std::unique_ptr<juce::AudioProcessorGraph>  processorGraph;
      Node::Ptr audioOutputNode;
      Node::Ptr midiInputNode;
      Node::Ptr midiOutputNode;
    private:
      void setOversamplingAmount(int oversampling_amount, int sample_rate);
      int last_oversampling_amount_;
      int last_sample_rate_;
      int buffer_size;
      int curr_sample_rate;
//      juce::Value* oversampling_;
//      juce::Value* bps_;
//      juce::Value* legato_;
//      Decimator* decimator_;
//      PeakMeter* peak_meter_;


      JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SoundEngine)
  };
} // namespace vital

