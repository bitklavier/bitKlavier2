//
// Created by Davis Polito on 2/7/25.
//

#ifndef BITKLAVIER2_RAMPMODULATOR_H
#define BITKLAVIER2_RAMPMODULATOR_H

#include "ModulatorBase.h"
#include "PreparationStateImpl.h"
#include "Identifiers.h"
struct RampParams : public chowdsp::ParamHolder {
    RampParams(const juce::ValueTree& v) : chowdsp::ParamHolder("value")
    {
        add(time);
    }

    chowdsp::TimeMsParameter::Ptr time //ms
    {
        juce::ParameterID{"Time",100},
        "Value Change Time",
        juce::NormalisableRange{10.f,10000.f,1.f,2.f,false},
        10.f
    };
};

class RampModulatorProcessor : public ModulatorStateBase<bitklavier::PreparationStateImpl<RampParams>> {

public :
    RampModulatorProcessor(const juce::ValueTree&, juce::UndoManager*);
    ~RampModulatorProcessor() {}
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;

    void setTarget(float target);
    void setTime(float timeToDest);
    void setRate(float rate);
    float getNextSample();
    float getValue() { return value_; }
    void startRamp() { state_ = 1; }
    void stopRamp() { state_ = 0; }
    void retriggerFrom(float) override;
    void process() override{};
    void getNextAudioBlock (juce::AudioBuffer<float>& bufferToFill, juce::MidiBuffer& midiMessages) override;

    void releaseResources() override {}
    SynthSection* createEditor() override;
    void triggerModulation() override;
    void triggerReset() override;

    static constexpr ModulatorType type = ModulatorType::AUDIO;

    float value_ {0.f};
    float target_{0.f};
    float rate_ {0.001f};
    int state_{0};
    void continuousReset() override {
        triggerReset();
    }
    float sampleRate;

};


#endif //BITKLAVIER2_RAMPMODULATOR_H
