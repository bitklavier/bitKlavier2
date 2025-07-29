//
// Created by Davis Polito on 2/7/25.
//

#ifndef BITKLAVIER2_LFOMODULATOR_H
#define BITKLAVIER2_LFOMODULATOR_H

#include "ModulatorBase.h"
#include "PreparationStateImpl.h"
#include "Identifiers.h"
struct LFOParams : public chowdsp::ParamHolder {
    LFOParams() : chowdsp::ParamHolder("lfo")
    {
        add(freq);
    }

    chowdsp::FreqHzParameter::Ptr freq
            {
        juce::ParameterID{"lfofreq",100},
        "Freq",
        juce::NormalisableRange{0.f,30.f,.01f},
        15.f
            };
};
class LFOModulatorProcessor : public ModulatorStateBase<bitklavier::PreparationStateImpl<LFOParams>> {

public :
    LFOModulatorProcessor(juce::ValueTree&);
    ~LFOModulatorProcessor()
    {

    }
    void process() override{};
    void getNextAudioBlock (juce::AudioBuffer<float>& bufferToFill, juce::MidiBuffer& midiMessages) override;
    void prepareToPlay (int samplesPerBlock, double sampleRate ) override {
        juce::dsp::ProcessSpec spec { sampleRate, static_cast<juce::uint32>(samplesPerBlock), 1};
        prepare(spec);
        setFrequency(_state.params.freq->getCurrentValue()); // 1 Hz LFO
        setDepth(1.f);     // Half modulation depth
    }
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

    void prepare(const juce::dsp::ProcessSpec& spec)
    {
        sampleRate = spec.sampleRate;
        phase = 0.0f;
    }

    void setFrequency(float newFreq)
    {
        frequency = newFreq;
        phaseIncrement = (2.0f * juce::MathConstants<float>::pi * frequency) / sampleRate;
    }

    void setDepth(float newDepth)
    {
        depth = newDepth;
    }

    void reset()
    {
        phase = 0.0f;
    }

    float getNextSample()
    {
        float sample = std::sin(phase) * depth;
        phase += phaseIncrement;

        if (phase > juce::MathConstants<float>::twoPi)
            phase -= juce::MathConstants<float>::twoPi;

        return sample;
    }

private:
    float sampleRate = 44100.0f;
    float frequency = 1.0f;
    float depth = 1.0f;
    float phase = 0.0f;
    float phaseIncrement = 0.0f;
};


#endif //BITKLAVIER2_LFOMODULATOR_H
