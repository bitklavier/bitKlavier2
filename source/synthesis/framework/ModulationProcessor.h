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
class ModulationConnection;
class StateConnection;
    struct ModulatorRouting{
        std::vector<ModulationConnection*> mod_connections;
    };
    class ModulationProcessor : public juce::AudioProcessor {
    public:
        ModulationProcessor(SynthBase& parent,const juce::ValueTree& vt);
        // ~ModulationProcessor()
        bool acceptsMidi() const override {
            return true;
        }

        bool producesMidi() const override { return false; }

        bool isMidiEffect() const override { return false; }

        const juce::String getName() const override {
            return "modulation";
        }

        void prepareToPlay(double sampleRate, int samplesPerBlock) override {
            setRateAndBufferSizeDetails(sampleRate,samplesPerBlock);
            for(auto& buffer : tmp_buffers)
            {
                buffer.setSize(1,samplesPerBlock);
            }
            sampleRate_ = sampleRate;
            blockSize_ = samplesPerBlock;
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
        void removeModulationConnection(ModulationConnection*);
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
        //could create new bus may need to happen on audio threafd?
        int createNewModIndex();
        chowdsp::DeferredAction mainThreadAction;

    };


} // bitklavier

#endif //BITKLAVIER2_MODULATIONPROCESSOR_H
