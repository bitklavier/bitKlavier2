#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <Identifiers.h>
#include "synth_base.h"

namespace bitklavier {

    class CommentProcessor : public juce::AudioProcessor {
    public:
        CommentProcessor(SynthBase& parent, const juce::ValueTree& vt, juce::UndoManager*) :
        juce::AudioProcessor(BusesProperties()), state(vt)
        {
            createUuidProperty(state);
        }

        bool acceptsMidi() const override { return false; }
        bool producesMidi() const override { return false; }
        bool isMidiEffect() const override { return false; }
        const juce::String getName() const override { return "comment"; }

        void prepareToPlay(double sampleRate, int samplesPerBlock) override {
            setRateAndBufferSizeDetails(sampleRate, samplesPerBlock);
        }

        void releaseResources() override {}
        double getTailLengthSeconds() const override { return 0.0; }
        void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override {}

        juce::AudioProcessorEditor* createEditor() override { return nullptr; }
        int getNumPrograms() override { return 1; }
        int getCurrentProgram() override { return 1; }
        void setCurrentProgram(int index) override {}
        void changeProgramName(int index, const juce::String& newName) override {}
        void getStateInformation(juce::MemoryBlock& destData) override {}
        void setStateInformation(const void* data, int sizeInBytes) override {}
        bool hasEditor() const override { return false; }
        const juce::String getProgramName(int index) override { return ""; }

        juce::ValueTree state;

    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CommentProcessor)
    };

} // bitklavier
