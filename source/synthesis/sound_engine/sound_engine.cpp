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

    SoundEngine::SoundEngine() : /*voice_handler_(nullptr),*/
            last_oversampling_amount_(-1), last_sample_rate_(-1),
            processorGraph(std::make_unique<juce::AudioProcessorGraph>()) {

        processorGraph->clear();
        processorGraph->enableAllBuses();
    }

    SoundEngine::~SoundEngine() {}

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

        auto nodes = processorGraph->getNodes();

        /**
         * start with the 4th node.
         *  - the first three are initialized in initialiseGraph() in sound_engine.h
         *      - node 0: audio output to DAW
         *      - nodes 1-2: MIDI in and out from/to DAW
         *      - eventually we will want a 4th, to take audio in from DAW
         */
        for (int i = 3; i < nodes.size(); i++){
            auto node = nodes[i];
            auto tree = v.getChildWithName(IDs::PREPARATIONS).getChildWithProperty(IDs::nodeID,
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

} // namespace bitklavier
