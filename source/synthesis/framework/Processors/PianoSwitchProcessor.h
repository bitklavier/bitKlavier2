//
// Created by Dan Trueman on 7/22/25.
//

#ifndef BITKLAVIER0_PIANOSWITCHPROCESSOR_H
#define BITKLAVIER0_PIANOSWITCHPROCESSOR_H

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <Identifiers.h>
#include "synth_base.h"
class ModulatorBase;

namespace bitklavier {
    class ModulationConnection;
    class StateConnection;

    class PianoSwitchProcessor : public juce::AudioProcessor {
    public:
        PianoSwitchProcessor(const juce::ValueTree& vt,SynthBase& parent) :
              juce::AudioProcessor(BusesProperties().withInput("disabled",juce::AudioChannelSet::mono(),false)
                      .withOutput("disabled",juce::AudioChannelSet::mono(),false)
                      .withOutput("Modulation",juce::AudioChannelSet::discreteChannels(1),true)
                      .withInput( "Modulation",juce::AudioChannelSet::discreteChannels(1),true)
                      ), state(vt)
        {
            createUuidProperty(state);
        }

        static std::unique_ptr<juce::AudioProcessor> create(SynthBase& parent,const juce::ValueTree& v) {
            return std::make_unique<PianoSwitchProcessor>(v,parent);
        }

        bool acceptsMidi() const override {return true;}
        bool producesMidi() const override { return false; }
        bool isMidiEffect() const override { return false; }
        const juce::String getName() const override { return "pianoswitch"; }

        void prepareToPlay(double sampleRate, int samplesPerBlock) override {
            setRateAndBufferSizeDetails(sampleRate,samplesPerBlock);
        }

        void releaseResources() override {}
        double getTailLengthSeconds() const override {}
        void processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages) override;

        juce::AudioProcessorEditor * createEditor() override { return  nullptr; }
        int getNumPrograms() override { return 1; }
        int getCurrentProgram() override { return 1; }
        void setCurrentProgram(int index) override {}
        void changeProgramName(int index, const juce::String &newName) override {}
        void getStateInformation(juce::MemoryBlock &destData) override {}
        void setStateInformation(const void *data, int sizeInBytes) override {}
        bool hasEditor() const override { return true; }
        const juce::String getProgramName(int index) override { return ""; }

        void removeModulator(ModulatorBase*);

        juce::ValueTree state;

    private :

    };

} // bitklavier

#endif //BITKLAVIER0_PIANOSWITCHPROCESSOR_H
