// Copyright (C) 2022-2025 Dan Trueman
// SPDX-License-Identifier: GPL-3.0-or-later

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
        add(freq, waveShape, automaticStart);
    }

    chowdsp::FreqHzParameter::Ptr freq
    {
        // juce::ParameterID{"lfofreq",100},
        juce::ParameterID{"Frequency",100},
        "LFO Frequency",
        juce::NormalisableRange{0.001f,10.f,.001f},
        1.0f
    };

    chowdsp::ChoiceParameter::Ptr waveShape
    {
        juce::ParameterID { "waveShape", 100 },
        "Wave Shape",
        juce::StringArray { "Sine", "Square", "SawUp", "SawDown", "Triangle", "Random" },
        0 // default: Sine
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
    LFOModulatorProcessor(const juce::ValueTree&, juce::UndoManager *);
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
        // Start the LFO. Phase is NOT reset here; use Reset prep to reset phase.
        lfo_on = true;
        ModulatorBase::triggerModulation();
    }

    void triggerReset() override
    {
        reset(); // Reset phase — called only by Reset preparation.
    }

    void stopModulation() override
    {
        lfo_on = false; // Called on noteOff; phase is preserved.
    }

    // Called on a second noteOn in toggle mode: stop without resetting phase.
    void triggerNoteOnStop() override
    {
        lfo_on = false;
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
        // getNextSample(); // remove this
        // DBG("resetting phase, lastSample = " << lastSample);
    }

    float getNextSample(int shape = 0)
    {
        float sample;
        const float pi = juce::MathConstants<float>::pi;

        // Detect the start of a new cycle for Sample+Hold (phase near 0).
        const bool isNewCycle = (phase < phaseIncrement);

        switch (shape)
        {
            case 1: // Square
                sample = (phase < pi) ? 1.0f : -1.0f;
                break;
            case 2: // Sawtooth Up: ramps -1 → +1
                sample = phase / pi - 1.0f;
                break;
            case 3: // Sawtooth Down: ramps +1 → -1
                sample = 1.0f - phase / pi;
                break;
            case 4: // Triangle
                sample = (phase < pi) ? (phase / pi * 2.0f - 1.0f)
                                      : (3.0f - phase / pi * 2.0f);
                break;
            case 5: // Sample & Hold — new value at start of each cycle
                if (isNewCycle)
                    sampleHoldValue = juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f;
                sample = sampleHoldValue;
                break;
            default: // Sine
                sample = std::sin(phase);
                break;
        }

        phase += phaseIncrement;
        if (phase > juce::MathConstants<float>::twoPi)
            phase -= juce::MathConstants<float>::twoPi;

        return lastSample = sample;
    }
    void continuousReset() override {
        // No-op: LFO does not reset phase on noteOn retrigger.
        // Phase only resets via Reset preparation (triggerReset).
    }
private:
    float sampleRate = 44100.0f;
    float frequency = 1.0f;
    float depth = 1.0f;
    float phase = 0.0f;
    float phaseIncrement = 0.0f;
    float lastSample = 0.0f;
    float sampleHoldValue = 0.0f;
    bool lfo_on = false;
};

#endif //BITKLAVIER2_LFOMODULATOR_H
