//
// Created by Davis Polito on 2/7/25.
//

#ifndef BITKLAVIER2_RAMPMODULATOR_H
#define BITKLAVIER2_RAMPMODULATOR_H

#include "ModulatorBase.h"
#include "PreparationStateImpl.h"
#include "Identifiers.h"
struct RampParams : public chowdsp::ParamHolder {
    RampParams() : chowdsp::ParamHolder("ramp")
    {
        add(time);
    }

    chowdsp::TimeMsParameter::Ptr time
            {
        juce::ParameterID{"Time",100},
        "Time",
        juce::NormalisableRange{10.f,10000.f,1.f,2.f,false},
        10.f
            };
};
class RampModulatorProcessor : public ModulatorStateBase<bitklavier::PreparationStateImpl<RampParams>> {

public :
    RampModulatorProcessor(juce::ValueTree&);
    ~RampModulatorProcessor()
    {

    }
    void process() override{};
    void getNextAudioBlock (juce::AudioBuffer<float>& bufferToFill, juce::MidiBuffer& midiMessages) override;
    void prepareToPlay (int samplesPerBlock, double sampleRate ) override {}
    void releaseResources() override {}
    SynthSection* createEditor() override
    {
        return new bitklavier::ParametersView(_state, _state.params, state.getProperty(IDs::type).toString() + "-" + state.getProperty(IDs::uuid).toString());
    }
    void triggerModulation() override
    {
        trigger = true;
    }
    bool trigger = false;
    static constexpr ModulatorType type = ModulatorType::AUDIO;

};


#endif //BITKLAVIER2_RAMPMODULATOR_H
