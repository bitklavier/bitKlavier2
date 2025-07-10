//
// Created by Davis Polito on 1/30/25.
//

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <Identifiers.h>
#include "synth_base.h"
class ModulatorBase;

namespace bitklavier {
class ModulationConnection;
class StateConnection;
    // struct ModulatorRouting{
    //     std::vector<ModulationConnection*> mod_connections;
    // };
    class ResetProcessor : public juce::AudioProcessor {
    public:
        ResetProcessor(const juce::ValueTree& vt,SynthBase& parent) :
        juce::AudioProcessor(BusesProperties().withInput("disabled",juce::AudioChannelSet::mono(),false)
        .withOutput("disabled",juce::AudioChannelSet::mono(),false)
        .withOutput("Modulation",juce::AudioChannelSet::discreteChannels(1),true)
        .withInput( "Modulation",juce::AudioChannelSet::discreteChannels(1),true)
        .withOutput("Reset",juce::AudioChannelSet::discreteChannels(1),true)), state(vt)
        {
            createUuidProperty(state);
        }
        static std::unique_ptr<juce::AudioProcessor> create(SynthBase& parent,const juce::ValueTree& v) {
            return std::make_unique<ResetProcessor>(v,parent);
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
            setRateAndBufferSizeDetails(sampleRate,samplesPerBlock);
            // for(auto buffer : tmp_buffers)
            // {
            //     buffer.setSize(1,samplesPerBlock);
            // }
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


//        std::unordered_map<ModulatorBase*,> outchannel_to_mods_;
int blockSize_ =0;
int sampleRate_ = 0;

        juce::ValueTree state;
    private :

    };

} // bitklavier

