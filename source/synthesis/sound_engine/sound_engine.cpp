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
//        DBG("***********SWITCH PIANOS********************");
//        DBG("*********curr piano is " + v.getProperty(IDs::name).toString() + " ***********************");
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

    void SoundEngine::addDefaultChain(SynthBase& parent, juce::ValueTree& tree) {
        if (gainProcessor || compressorProcessor || eqProcessor)
            return;
        juce::ValueTree buseq (IDs::BUSEQ);
        juce::ValueTree buscompressor (IDs::BUSCOMPRESSOR);
        tree.appendChild (buseq, nullptr);
        tree.appendChild (buscompressor, nullptr);
        buseq.setProperty(IDs::type, bitklavier::BKPreparationType::PreparationTypeEQ, nullptr);
        buscompressor.setProperty(IDs::type, bitklavier::BKPreparationType::PreparationTypeCompressor, nullptr);
        gainProcessor = std::make_unique<GainProcessor>(parent,tree);
        compressorProcessor = std::make_unique<CompressorProcessor>(parent,buseq);
        eqProcessor = std::make_unique<EQProcessor>(parent,buscompressor);
        gainProcessor       = std::make_unique<GainProcessor>(parent, tree);
        compressorProcessor = std::make_unique<CompressorProcessor>(parent, buseq);
        eqProcessor         = std::make_unique<EQProcessor>(parent, buscompressor);
    }
} // namespace bitklavier
