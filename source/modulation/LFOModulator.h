//
// Created by Davis Polito on 2/7/25.
//

#ifndef BITKLAVIER2_LFOMODULATOR_H
#define BITKLAVIER2_LFOMODULATOR_H

#include "ModulatorBase.h"
#include "PreparationStateImpl.h"
#include "Identifiers.h"

struct LFOParams : public chowdsp::ParamHolder {
    LFOParams(const juce::ValueTree& v) : chowdsp::ParamHolder("lfo")
    {
        add(freq, automaticStart);
    }

    chowdsp::FreqHzParameter::Ptr freq
    {
        // juce::ParameterID{"lfofreq",100},
        juce::ParameterID{"Frequency",100},
        "LFO Frequency",
        juce::NormalisableRange{0.001f,10.f,.001f},
        0.001f
    };

    chowdsp::BoolParameter::Ptr automaticStart
    {
        juce::ParameterID { "automaticStart", 100 },
        "auto",
        false
    };
};

class LFOModulatorProcessor : public ModulatorStateBase<bitklavier::PreparationStateImpl<LFOParams>> {

public :
    LFOModulatorProcessor(juce::ValueTree&);
    ~LFOModulatorProcessor() {}

    void process() override{};
    void getNextAudioBlock (juce::AudioBuffer<float>& bufferToFill, juce::MidiBuffer& midiMessages) override;
    void prepareToPlay (double sampleRate, int samplesPerBlock) override {
        juce::dsp::ProcessSpec spec { sampleRate, static_cast<juce::uint32>(samplesPerBlock), 1};
        prepare(spec);
        setFrequency(_state.params.freq->getCurrentValue()); // 1 Hz LFO
        setDepth(1.f);     // Half modulation depth
    }

    void releaseResources() override {}
    SynthSection* createEditor() override;

    void triggerModulation() override
    {
        // turn the lfo on/off
        // - starts/stops in place.
        // - use Reset prep if you want to reset the phase as well
        if (lfo_on) lfo_on = false;
        else lfo_on = true;
        ModulatorBase::triggerModulation();
    }

    void triggerReset() override
    {
        reset(); //just reset the phase of the LFO
    }

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
        float sample = std::sin(phase);
        phase += phaseIncrement;

        if (phase > juce::MathConstants<float>::twoPi)
            phase -= juce::MathConstants<float>::twoPi;

        return sample;
    }
    void continuousReset() override {
        triggerReset();
    }
private:
    float sampleRate = 44100.0f;
    float frequency = 1.0f;
    float depth = 1.0f;
    float phase = 0.0f;
    float phaseIncrement = 0.0f;
    bool lfo_on = false;
};

#endif //BITKLAVIER2_LFOMODULATOR_H
