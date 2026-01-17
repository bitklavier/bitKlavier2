//
// Created by Davis Polito on 1/30/25.
//

#include "ModulationProcessor.h"
#include "ModulatorBase.h"
#include "ModulationConnection.h"
#include "ModulationList.h"
#include "sound_engine.h"

bitklavier::ModulationProcessor::ModulationProcessor(SynthBase &parent,
                                                     const juce::ValueTree &vt) : juce::AudioProcessor(
        BusesProperties().withInput("disabled", juce::AudioChannelSet::mono(), false)
        .withOutput("disabled", juce::AudioChannelSet::mono(), false)
        .withOutput("Modulation", juce::AudioChannelSet::discreteChannels(1), true)
        .withInput("Modulation", juce::AudioChannelSet::discreteChannels(1), true)
        .withInput("Reset", juce::AudioChannelSet::discreteChannels(1), true)), state(vt),
    parent(parent) {
    // getBus(false,1)->setNumberOfChannels(state.getProperty(IDs::numModChans,0));
    createUuidProperty(state);
    mod_list = std::make_unique<ModulationList>(state, &parent, this);
}

void bitklavier::ModulationProcessor::processBlock(juce::AudioBuffer<float> &buffer,
                                                   juce::MidiBuffer &midiMessages) {
    const int idx = activeSnapshotIndex.load(std::memory_order_acquire);
    auto &snap = snapshots[idx];

    if (snap.mods.empty())
        return;

    // Consume external RT reset request
    if (pendingResetAll_.exchange(false, std::memory_order_acq_rel))
    {
        DBG("ModulationProcessor::processBlock, pendingResetAll_ = true");
        for (auto& e : snap.mods)
            if (auto* mod = e.mod)
                mod->continuousReset(); // For Ramp, this calls triggerReset()
    }

    // from Audio Bus
    auto reset_in = getBusBuffer(buffer, true, 2);
    if (reset_in.getSample(0, 0) == 1.0f) {
        DBG("ModulationProcessor::processBlock, reset from sample == 1 on reset bus");
        for (auto &e: snap.mods)
            if (e.mod != nullptr)
                e.mod->triggerReset();
    }

    // MIDI note-on => request retrigger
    for (auto msg: midiMessages) {
        if (msg.getMessage().isNoteOn())
        {
            DBG("ModulationProcessor::processBlock, reset from noteOn (attached Reset)");
            for (auto &e: snap.mods)
                if (e.mod != nullptr)
                    e.mod->pendingRetrigger.store(true, std::memory_order_release);
        }
    }

    // Main render/mix
    for (auto &e: snap.mods) {
        auto *mod = e.mod;
        if (mod == nullptr)
            continue;

        if (mod->type != ModulatorType::AUDIO)
            continue;

        const bool doRetrig = mod->pendingRetrigger.exchange(false, std::memory_order_acq_rel);
        if (doRetrig) {
            DBG("doTrig == true");
            // capture per-connection carry + compute new scaling per connection
            for (auto *c: e.connections) {
                if (!c) continue;
                const float raw0 = e.lastRaw0;               // where the ramp was before retrigger
                const float oldScale = c->getScalingForDSP();// locked/old scaling BEFORE you change it
                float carry = 0.0f;
                DBG("oldScale = " + juce::String(oldScale));
                DBG("lastRaw = " + juce::String(raw0));
                // handle polarity the same way you mix
                const bool polarityMatches =
                    (mod->isDefaultBipolar && c->isBipolar()) ||
                    (!mod->isDefaultBipolar && !c->isBipolar());

                if (polarityMatches)
                {
                    carry = raw0 * oldScale; // ramp raw is already 0..1
                }
                else if (mod->isDefaultBipolar && !c->isBipolar())
                {
                    const float unipolar0 = 0.5f * (raw0 + 1.0f);
                    carry = unipolar0 * oldScale;
                }
                const float currentTotal = parent.getParamOffsetBank().getOffset(c->getDestParamIndex());

                // const float carry = c->lastAppliedPrev_.load(std::memory_order_relaxed);
                // c->setCarryActive(carry);
                c->updateScalingAudioThread(currentTotal, raw0);
                // const float currentTotal = parent.getParamOffsetBank().getOffset(c->getDestParamIndex());
                // c->setCurrentTotalBaseValue(baseNoThis);
                // c->lockScaling();
                // c->updateScalingAudioThread(currentTotal);
                c->lockScaling();
            }

            // restart the ramp (or whatever mod) now
            mod->continuousReset();
            mod->triggerModulation();
        }

        // NOW generate the audio AFTER retrigger/reset, so e.tmp matches new state
        mod->getNextAudioBlock(e.tmp, midiMessages);

        // Mix connections
        for (auto *c: e.connections) {
            if (!c) continue;

            const int outCh = c->modulation_output_bus_index;
            if ( outCh >=  buffer.getNumChannels())
                continue;

            auto *dest = buffer.getWritePointer(outCh);
            auto *src = e.tmp.getReadPointer(0);
            const float scale = c->getScalingForDSP();

            // (optional) keep for your scaling update logic
            c->setCurrentTotalBaseValue(parent.getParamOffsetBank().getOffset(c->getDestParamIndex()));

            // polarity handling
            const bool polarityMatches =
                    (mod->isDefaultBipolar && c->isBipolar()) ||
                    (!mod->isDefaultBipolar && !c->isBipolar());

            if (polarityMatches) {
                // Carry-crossfade only for ramps
                // if ( c->carryActive_.load(std::memory_order_acquire)) {
                    const float carry = c->carryApplied_.load(std::memory_order_relaxed);

                    for (int s = 0; s < buffer.getNumSamples(); ++s) {
                        const float r = src[s]; // ramp raw 0..1
                        dest[s] +=  carry + r * scale;
                        // dest[s] +=  carry + r * scale;
                    }

                    if (src[buffer.getNumSamples() - 1] >= 0.9999f)
                        c->carryActive_.store(false, std::memory_order_release);

                    // c->lastApplied_.store(src[0] * scale, std::memory_order_relaxed);
                // } else {
                    // buffer.addFrom(outCh, 0, src, buffer.getNumSamples(), scale);
                    // c->lastApplied_.store(src[0] * scale, std::memory_order_relaxed);
                // }
            } else if (mod->isDefaultBipolar && !c->isBipolar()) {
                // bipolar src -> unipolar dest
                for (int s = 0; s < buffer.getNumSamples(); ++s) {
                    const float unipolar = 0.5f * (src[s] + 1.0f);
                    dest[s] += unipolar * scale;
                }

                // control-rate lastApplied (consistent with what you actually add)
                // const float unipolar0 = 0.5f * (src[0] + 1.0f);
                // c->lastApplied_.store(unipolar0 * scale, std::memory_order_relaxed);

            }
            // c->lastAppliedPrev_.store(c->lastApplied_.load(std::memory_order_relaxed),
                          // std::memory_order_relaxed);
        }
        e.lastRaw0 = e.tmp.getSample(0, 0);
    }
}


void bitklavier::ModulationProcessor::addModulator(ModulatorBase *mod) {
    // callOnMainThread([this, mod]
    //    {
    mod->parent_ = this;

    auto it = std::find(modulators_.begin(), modulators_.end(), nullptr);
    std::size_t index = 0;

    if (it != modulators_.end()) {
        index = std::distance(modulators_.begin(), it);
        *it = mod;
    } else {
        index = modulators_.size();
        modulators_.push_back(mod);
    }

    if (tmp_buffers.size() <= index) tmp_buffers.resize(index + 1);
    if (mod_routing.size() <= index) mod_routing.resize(index + 1);

    tmp_buffers[index].setSize(1, blockSize_);
    mod_routing[index] = {};

    if (blockSize_ > 0 && sampleRate_ > 0.0)
        mod->prepareToPlay(sampleRate_, blockSize_);

    rebuildAndPublishSnapshot();
    // }, true);
}

void bitklavier::ModulationProcessor::removeModulator(ModulatorBase *mod) {
    // callOnMainThread([this, mod]
    // {
    auto it = std::find(modulators_.begin(), modulators_.end(), mod);
    if (it == modulators_.end()) return;

    const auto index = (size_t) std::distance(modulators_.begin(), it);

    modulators_[index] = nullptr;
    tmp_buffers[index] = {};
    mod_routing[index] = {};

    rebuildAndPublishSnapshot();
    // }, true);
}

void bitklavier::ModulationProcessor::addModulationConnection(ModulationConnection *connection) {
    // callOnMainThread([this, connection]
    // {
    // Allocate / reuse output channel
    auto mystring = connection->isContinuousMod ? connection->destination_name + "cont" : connection->destination_name;
    connection->modulation_output_bus_index =
            allocateModulationChannel(mystring);
    all_modulation_connections_.push_back(connection);

    auto it = std::find(modulators_.begin(), modulators_.end(), connection->processor);
    if (it == modulators_.end() || *it == nullptr) return;

    const auto index = (size_t) std::distance(modulators_.begin(), it);
    if (index >= mod_routing.size()) return;

    mod_routing[index].mod_connections.push_back(connection);
    (*it)->connections_.push_back(connection->state);

    rebuildAndPublishSnapshot();
    // }, true);
}

void bitklavier::ModulationProcessor::removeModulationConnection(ModulationConnection *connection,
                                                                 std::string destination_name) {
    // callOnMainThread([this, connection]
    // {
    {
        auto end = std::remove(all_modulation_connections_.begin(),
                               all_modulation_connections_.end(),
                               connection);
        all_modulation_connections_.erase(end, all_modulation_connections_.end());
    }

    auto it = std::find(modulators_.begin(), modulators_.end(), connection->processor);
    if (it != modulators_.end()) {
        const auto index = (size_t) std::distance(modulators_.begin(), it);
        if (index < mod_routing.size()) {
            auto &v = mod_routing[index].mod_connections;
            auto end = std::remove(v.begin(), v.end(), connection);
            v.erase(end, v.end());
        }
    }
    auto mystring = connection->isContinuousMod ? destination_name + "cont" : destination_name;
    if (releaseModulationChannel(mystring)) {
        parent.getEngine()->removeConnection(connection->connection_);
    }
    rebuildAndPublishSnapshot();
    // }, true);
}

void bitklavier::ModulationProcessor::addModulationConnection(StateConnection *connection) {
    all_state_connections_.push_back(connection);

    // auto it = std::find(modulators_.begin(), modulators_.end(), connection->processor);
    //size_t index = std::distance(modulators_.begin(), it);
    //mod_routing[index].mod_connections.push_back(connection);
}

void bitklavier::ModulationProcessor::removeModulationConnection(StateConnection *connection) {
    auto end = std::remove(all_state_connections_.begin(), all_state_connections_.end(), connection);
    all_state_connections_.erase(end, all_state_connections_.end());
};

int bitklavier::ModulationProcessor::createNewModIndex() {
    getBus(false, 1)->setNumberOfChannels(getBus(false, 1)->getNumberOfChannels() + 1);
    // state.setProperty(IDs::numModChans, getBus(false,1)->getNumberOfChannels(),nullptr);
    DBG("createnewmodindex " + juce::String(getBus(false,1)->getNumberOfChannels()));
    return (getBus(false, 1)->getNumberOfChannels() - 1); //(old number of channels) -1 to get 0 based
}

int bitklavier::ModulationProcessor::getNewModulationOutputIndex(const bitklavier::ModulationConnection &connection) {
    for (auto _connection: all_modulation_connections_) {
        if (connection.destination_name == _connection->destination_name) {
            return _connection->modulation_output_bus_index;
        }
    }
    return createNewModIndex();
}

int bitklavier::ModulationProcessor::getNewModulationOutputIndex(const bitklavier::StateConnection &connection) {
    for (auto _connection: all_state_connections_) {
        if (connection.destination_name == _connection->destination_name) {
            return _connection->modulation_output_bus_index;
        }
    }
    return -1;
}

ModulatorBase *bitklavier::ModulationProcessor::getModulatorBase(std::string &uuid) {
    for (auto mod: modulators_) {
        if (mod == nullptr) continue;
        if (mod->state.getProperty(IDs::uuid).toString() == juce::String(uuid)) {
            return mod;
        }
    }
    return nullptr;
}

void bitklavier::ModulationProcessor::rebuildAndPublishSnapshot() {
    // MESSAGE THREAD ONLY
    JUCE_ASSERT_MESSAGE_THREAD
    const int cur = activeSnapshotIndex.load(std::memory_order_relaxed);
    const int next = 1 - cur;
    auto &dst = snapshots[next];
    dst.clearForRebuild();
    dst.blockSize = blockSize_;
    dst.sampleRate = sampleRate_;
    if (modulators_.empty())
        return;
    dst.mods.reserve(modulators_.size());

    for (size_t i = 0; i < modulators_.size(); ++i) {
        auto *mod = modulators_[i];
        if (!mod) continue;
        // 1) add an element (value) first
        dst.mods.emplace_back();

        // 2) mutate the newly-added element in place
        auto &entry = dst.mods.back();
        entry.mod = mod;

        // Allocate/resize tmp buffer (message thread ok)
        entry.tmp.setSize(1, blockSize_, false, false, true);

        // Copy pointer list into snapshot (don’t alias shared vectors)
        entry.connections.clear();
        if (i < mod_routing.size())
            entry.connections = mod_routing[i].mod_connections;
    }

    // Publish: one atomic store
    activeSnapshotIndex.store(next, std::memory_order_release);
}

int bitklavier::ModulationProcessor::allocateModulationChannel(const std::string &destination) {
    auto &info = destChannelMap[destination];

    // Already allocated → reuse
    if (info.refCount > 0) {
        ++info.refCount;
        return info.channel;
    }

    // Find first free bit
    int channel = -1;
    for (int i = 0; i < kMaxModulationChannels; ++i) {
        if (!modChannelUsed.test(i)) {
            channel = i;
            break;
        }
    }

    jassert(channel >= 0); // ran out of channels

    modChannelUsed.set(channel);
    info.channel = channel;
    info.refCount = 1;

    // Grow bus if needed (grow-only, safe)
    if (channel >= maxAllocatedChannel) {
        const int newBusSize = channel + 1;
        maxAllocatedChannel = newBusSize;

        // IMPORTANT: message thread only
        ScopedSuspendProcessing suspend(*this);
        getBus(false, 1)->setNumberOfChannels(newBusSize);
    }

    return channel;
}

bool bitklavier::ModulationProcessor::releaseModulationChannel(const std::string &destination) {
    auto it = destChannelMap.find(destination);
    if (it == destChannelMap.end())
        return false;

    auto &info = it->second;

    jassert(info.refCount > 0);
    --info.refCount;

    if (info.refCount == 0) {
        jassert(info.channel >= 0 && info.channel < kMaxModulationChannels);
        modChannelUsed.reset(info.channel);
        destChannelMap.erase(it);

        return true;
        // NOTE:
        // We deliberately do NOT shrink the bus here.
        // Shrinking buses during runtime causes host instability
        // and is unnecessary since channels are now reusable.
    }
    return false;
}
