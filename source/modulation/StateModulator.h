//
// Created by Davis Polito on 2/17/25.
//

#ifndef BITKLAVIER2_STATEMODULATOR_H
#define BITKLAVIER2_STATEMODULATOR_H


#include "ModulatorBase.h"
#include "PreparationStateImpl.h"
#include "Identifiers.h"

class StateModulatorProcessor : public ModulatorBase {

public :

    StateModulatorProcessor(const juce::ValueTree&, juce::UndoManager*);
    ~StateModulatorProcessor(){}
    void process() override {};
    void getNextAudioBlock (juce::AudioBuffer<float>& bufferToFill, juce::MidiBuffer& midiMessages) override;
    void prepareToPlay ( double sampleRate, int samplesPerBlock) override {}
    void releaseResources() override {}
    SynthSection* createEditor() override;

    static constexpr ModulatorType type = ModulatorType::STATE;
    void getStateInformation(juce::MemoryBlock &destData) override {}
    void setStateInformation (const void *data, int sizeInBytes) override {}
};


#endif //BITKLAVIER2_STATEMODULATOR_H
