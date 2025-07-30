//
// Created by Davis Polito on 2/17/25.
//

#ifndef BITKLAVIER2_STATEMODULATOR_H
#define BITKLAVIER2_STATEMODULATOR_H


#include "ModulatorBase.h"
#include "PreparationStateImpl.h"
#include "Identifiers.h"
//struct StateParams : public chowdsp::ParamHolder {
//    RampParams() : chowdsp::ParamHolder("ramp")
//    {
//        add(time);
//    }
//
//    chowdsp::TimeMsParameter::Ptr time
//            {
//                    juce::ParameterID{"Time",100},
//                    "Time",
//                    juce::NormalisableRange{10.f,10000.f,1.f,2.f,false},
//                    10.f
//            };
//};
class StateModulatorProcessor : public ModulatorBase {

public :

    StateModulatorProcessor(juce::ValueTree&);
    ~StateModulatorProcessor(){}
    void process() override {};
    void getNextAudioBlock (juce::AudioBuffer<float>& bufferToFill, juce::MidiBuffer& midiMessages) override;
    void prepareToPlay (int samplesPerBlock, double sampleRate ) override {}
    void releaseResources() override {}
    SynthSection* createEditor() override;

    static constexpr ModulatorType type = ModulatorType::STATE;
    void getStateInformation(juce::MemoryBlock &destData) override {
    }
    void setStateInformation (const void *data, int sizeInBytes) override {
    }
};


#endif //BITKLAVIER2_STATEMODULATOR_H
