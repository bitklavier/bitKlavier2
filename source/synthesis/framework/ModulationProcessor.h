//
// Created by Davis Polito on 1/30/25.
//

#ifndef BITKLAVIER2_MODULATIONPROCESSOR_H
#define BITKLAVIER2_MODULATIONPROCESSOR_H
#include <juce_audio_processors/juce_audio_processors.h>
#include <Identifiers.h>
#include "synth_base.h"
#include "ModulationList.h"
class ModulatorBase;

namespace bitklavier {
    static constexpr int kMaxModulationChannels = 256;
   class ModulationConnection;
class StateConnection;
    struct ModulatorRouting{
        std::vector<ModulationConnection*> mod_connections;
    };
    class ModulationProcessor : public juce::AudioProcessor {
    public:
        ModulationProcessor(SynthBase& parent,const juce::ValueTree& vt);
        ~ModulationProcessor() {
            juce::SpinLock::ScopedLockType sl (snapshotPublishLock);
            snapshotAtomic.store(nullptr, std::memory_order_release);
            snapshotOwner = nullptr;
        }
        bool acceptsMidi() const override {
            return true;
        }

        bool producesMidi() const override { return false; }

        bool isMidiEffect() const override { return false; }

        const juce::String getName() const override {
            return "modulation";
        }

        void prepareToPlay(double sampleRate, int samplesPerBlock) override {
            setRateAndBufferSizeDetails(sampleRate, samplesPerBlock);
            sampleRate_ = sampleRate;
            blockSize_ = samplesPerBlock;

            for (auto* m : modulators_)
                if (m != nullptr)
                    m->prepareToPlay(sampleRate, samplesPerBlock);

            // build snapshot on message thread; if prepareToPlay is not message thread,
            // callOnMainThread(...) instead. Many hosts call it on audio thread.
            callOnMainThread([this] { rebuildAndPublishSnapshot(); }, true);
        }

        void releaseResources() override
        {

        }

        double getTailLengthSeconds() const override
        {

        }

        void processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages) override;

        [[maybe_unused]] void addModulator(ModulatorBase*);

        juce::AudioProcessorEditor * createEditor() override
        {
           return  nullptr;
        }

        int getNumPrograms() override
        {
            return 1;
        }

        int getCurrentProgram() override
        {
            return 1;
        }

        void setCurrentProgram(int index) override
        {

        }

        void changeProgramName(int index, const juce::String &newName) override
        {

        }

        void getStateInformation(juce::MemoryBlock &destData) override
        {

        }

        void setStateInformation(const void *data, int sizeInBytes) override
        {

        }

        bool hasEditor() const override
        {
            return true;
        }

        const juce::String getProgramName(int index) override
        {
            return "";
        }

        void addModulationConnection(ModulationConnection*);
        void addModulationConnection(StateConnection*);
        void removeModulationConnection(ModulationConnection*,std::string);
        void removeModulationConnection(StateConnection*);

        int getNewModulationOutputIndex(const ModulationConnection&);
        int getNewModulationOutputIndex(const StateConnection&);
        void removeModulator(ModulatorBase*);
        //could probabalt make this into a struct
        std::vector<ModulatorBase*> modulators_;
        std::vector<juce::AudioBuffer<float>> tmp_buffers;
        std::vector<ModulatorRouting> mod_routing;
        std::vector<ModulationConnection*> all_modulation_connections_;
        std::vector<StateConnection*> all_state_connections_;
//        std::unordered_map<ModulatorBase*,> outchannel_to_mods_;
        int blockSize_ =0;
        int sampleRate_ = 0;
        ModulatorBase* getModulatorBase(std::string& uuid);
        juce::ValueTree state;
        /** Calls an action on the main thread via chowdsp::DeferredAction */
        template <typename Callable>
        void callOnMainThread (Callable&& func, bool couldBeAudioThread = false)
        {
            mainThreadAction.call (std::forward<Callable> (func), couldBeAudioThread);
        }
        std::unique_ptr<ModulationList >mod_list;

    private :
        // ===== Snapshot types =====
    struct RoutingSnapshot : public juce::ReferenceCountedObject
    {
        using Ptr = juce::ReferenceCountedObjectPtr<RoutingSnapshot>;

        struct ModEntry
        {
            ModulatorBase* mod = nullptr; // not owning
            juce::AudioBuffer<float> tmp; // owned by snapshot
            std::vector<ModulationConnection*> connections; // not owning
        };

        std::vector<ModEntry> mods;
        int blockSize = 0;
        double sampleRate = 0.0;
    };

        // ===== Published to audio thread =====
        std::atomic<RoutingSnapshot*> snapshotAtomic { nullptr };

        // ===== Only touched on message thread =====
        RoutingSnapshot::Ptr snapshotOwner; // keeps current snapshot alive
        juce::SpinLock snapshotPublishLock; // short lock only for pointer swap

        // Call this whenever graph changes (message thread only)
        void rebuildAndPublishSnapshot();

        // Optional: make bus resizing safer by suspending processing while changing buses
        struct ScopedSuspendProcessing
        {
            explicit ScopedSuspendProcessing(juce::AudioProcessor& p) : proc(p) { proc.suspendProcessing(true); }
            ~ScopedSuspendProcessing() { proc.suspendProcessing(false); }
            juce::AudioProcessor& proc;
        };
        // ===== Channel allocation (message thread only) =====
        std::bitset<kMaxModulationChannels> modChannelUsed;
        int maxAllocatedChannel = 0; // highest channel index ever allocated + 1

        struct DestChannelInfo
        {
            int channel = -1;
            int refCount = 0;
        };

        std::unordered_map<std::string, DestChannelInfo> destChannelMap;

        int allocateModulationChannel (const std::string& destination);
        bool releaseModulationChannel  (const std::string& destination);
        //could create new bus may need to happen on audio threafd?
        int createNewModIndex();
        chowdsp::DeferredAction mainThreadAction;
        SynthBase &parent;
    };

} // bitklavier

#endif //BITKLAVIER2_MODULATIONPROCESSOR_H
