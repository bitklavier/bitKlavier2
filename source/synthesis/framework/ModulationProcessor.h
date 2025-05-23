//
// Created by Davis Polito on 1/30/25.
//

#ifndef BITKLAVIER2_MODULATIONPROCESSOR_H
#define BITKLAVIER2_MODULATIONPROCESSOR_H
#include <juce_audio_processors/juce_audio_processors.h>
#include <Identifiers.h>
class ModulatorBase;

namespace bitklavier {
class ModulationConnection;
class StateConnection;
    struct ModulatorRouting{
        std::vector<bitklavier::ModulationConnection*> mod_connections;
    };
    class ModulationProcessor : public juce::AudioProcessor {
    public:
        ModulationProcessor(juce::ValueTree& vt) :
        juce::AudioProcessor(BusesProperties().withInput("disabled",juce::AudioChannelSet::mono(),false)
        .withOutput("disabled",juce::AudioChannelSet::mono(),false)
        .withOutput("Modulation",juce::AudioChannelSet::discreteChannels(1),true)
        .withInput( "Modulation",juce::AudioChannelSet::discreteChannels(1),true))
        {
            createUuidProperty(vt);
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
            for(auto buffer : tmp_buffers)
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
         void addModulationConnection(bitklavier::ModulationConnection*);
        void addModulationConnection(bitklavier::StateConnection*);
        void removeModulationConnection(bitklavier::ModulationConnection*);
        void removeModulationConnection(bitklavier::StateConnection*);

        int getNewModulationOutputIndex(const bitklavier::ModulationConnection&);
        int getNewModulationOutputIndex(const bitklavier::StateConnection&);
        void removeModulator(ModulatorBase*);
        //could probabalt make this into a struct
        std::vector<ModulatorBase*> modulators_;
        std::vector<juce::AudioBuffer<float>> tmp_buffers;
        std::vector<ModulatorRouting> mod_routing;
        std::vector<bitklavier::ModulationConnection*> all_modulation_connections_;
        std::vector<bitklavier::StateConnection*> all_state_connections_;
//        std::unordered_map<ModulatorBase*,> outchannel_to_mods_;
int blockSize_ =0;
int sampleRate_ = 0;
        ModulatorBase* getModulatorBase(std::string& uuid);
        ModulationProcessor();
    private :
        //could create new bus may need to happen on audio threafd?
        int createNewModIndex();
    };

} // bitklavier

#endif //BITKLAVIER2_MODULATIONPROCESSOR_H
