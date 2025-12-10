//
// Created by Myra Norton on 11/14/25.
//

#pragma once
#include "Identifiers.h"
#include "PluginBase.h"
#include "EQFilterParams.h"
#include "utils.h"
#include <PreparationStateImpl.h>
#include <chowdsp_sources/chowdsp_sources.h>
// ********************************************************************************************* //
// ****************************  EQParams  ********************************************* //
// ********************************************************************************************* //

struct EQParams : chowdsp::ParamHolder
{
    // Adds the appropriate parameters to the Nostalgic Processor
    EQParams(const juce::ValueTree& v) : chowdsp::ParamHolder ("eq")
    {
            add(activeEq,
                resetEq,
                loCutFilterParams,
                peak1FilterParams,
                peak2FilterParams,
                peak3FilterParams,
                hiCutFilterParams);
    }

    // bypass bool
    chowdsp::BoolParameter::Ptr activeEq {
        juce::ParameterID { "activeEq", 100 },
        "activeEq",
        true
    };

    // reset bool
    chowdsp::BoolParameter::Ptr resetEq {
        juce::ParameterID { "resetEq", 100},
        "reset",
        false
    };

    // filters
    EQCutFilterParams loCutFilterParams{"loCut"};
    EQPeakFilterParams peak1FilterParams{"peak1"};
    EQPeakFilterParams peak2FilterParams{"peak2"};
    EQPeakFilterParams peak3FilterParams{"peak3"};
    EQCutFilterParams hiCutFilterParams{"hiCut"};

    juce::Array<bool> activeFilters = {false, false, false, false, false};

    // DSP stuff
    using Filter = juce::dsp::IIR::Filter<float>;
    using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;
    using Chain = juce::dsp::ProcessorChain<CutFilter, Filter, Filter, Filter, CutFilter>;
    Chain leftChain;
    Chain rightChain;
    
    double sampleRate = 44100;

    double magForFreq(double freq) {
        double mag = 1.f;

        // Since leftChain and state.params.rightChain use the same coefficients, it's fine to just get them from left
        if (!leftChain.isBypassed<ChainPositions::Peak1>())
            mag *= leftChain.get<ChainPositions::Peak1>()
            .coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!leftChain.isBypassed<ChainPositions::Peak2>())
            mag *= leftChain.get<ChainPositions::Peak2>()
            .coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!leftChain.isBypassed<ChainPositions::Peak3>())
            mag *= leftChain.get<ChainPositions::Peak3>()
            .coefficients->getMagnitudeForFrequency(freq, sampleRate);

        if(!leftChain.get<ChainPositions::LowCut>().isBypassed<0>())
            mag *= leftChain.get<ChainPositions::LowCut>().get<0>()
            .coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if(!leftChain.get<ChainPositions::LowCut>().isBypassed<1>())
            mag *= leftChain.get<ChainPositions::LowCut>().get<1>()
            .coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if(!leftChain.get<ChainPositions::LowCut>().isBypassed<2>())
            mag *= leftChain.get<ChainPositions::LowCut>().get<2>()
            .coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if(!leftChain.get<ChainPositions::LowCut>().isBypassed<3>())
            mag *= leftChain.get<ChainPositions::LowCut>().get<3>()
            .coefficients->getMagnitudeForFrequency(freq, sampleRate);

        if(!leftChain.get<ChainPositions::HighCut>().isBypassed<0>())
            mag *= leftChain.get<ChainPositions::HighCut>().get<0>()
            .coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if(!leftChain.get<ChainPositions::HighCut>().isBypassed<1>())
            mag *= leftChain.get<ChainPositions::HighCut>().get<1>()
            .coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if(!leftChain.get<ChainPositions::HighCut>().isBypassed<2>())
            mag *= leftChain.get<ChainPositions::HighCut>().get<2>()
            .coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if(!leftChain.get<ChainPositions::HighCut>().isBypassed<3>())
            mag *= leftChain.get<ChainPositions::HighCut>().get<3>()
            .coefficients->getMagnitudeForFrequency(freq, sampleRate);

        return mag;
    }

    enum ChainPositions {
        LowCut,
        Peak1,
        Peak2,
        Peak3,
        HighCut
    };
};

/****************************************************************************************/

struct EQNonParameterState : chowdsp::NonParamState
{
    EQNonParameterState() {}
};

// ****************************** EQProcessor ****************************************** //
// ********************************************************************************************* //

class EQProcessor : public bitklavier::PluginBase<bitklavier::PreparationStateImpl<EQParams, EQNonParameterState>>,
                        public juce::ValueTree::Listener
{
public:
    EQProcessor (SynthBase& parent, const juce::ValueTree& v);
    ~EQProcessor()
    {
        parent.getValueTree().removeListener(this);
    }

    static std::unique_ptr<juce::AudioProcessor> create (SynthBase& parent, const juce::ValueTree& v)
    {
        return std::make_unique<EQProcessor> (parent, v);
    }

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}

    // void setupModulationMappings();

    void processAudioBlock (juce::AudioBuffer<float>& buffer) override {};
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    void processBlockBypassed (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override{};

    // Called upon initializiation and whenever a user changes parameters with a slider
    void updateCoefficients();
    // Return the magnitude corresponding to this frequency based off the current parameters of this equalizer
    double magForFreq(double freq);

    /*
     * this is where we define the buses for audio in/out, including the param modulation channels
     *      the "discreteChannels" number is currently just by hand set based on the max that this particularly preparation could have
     *      so if you add new params, might need to increase that number
     */
    juce::AudioProcessor::BusesProperties eqBusLayout()
    {
        return BusesProperties()
            .withOutput("Output", juce::AudioChannelSet::stereo(), true) // Main Output
            .withInput ("Input", juce::AudioChannelSet::stereo(), false)  // Main Input (not used here)

            /**
             * IMPORTANT: set discreteChannels below equal to the number of params you want to continuously modulate!!
             *              for nostalgic, we have 10:
             *                  - the ADSR params: attackParam, decayParam, sustainParam, releaseParam, and
             *                  - the main params: gainParam, hammerParam, releaseResonanceParam, pedalParam, OutputSendParam, outputGain,
             */
             /**
              * todo: check the number of discrete channels to match needs here
              */
            .withInput ("Modulation", juce::AudioChannelSet::discreteChannels (10), true) // Mod inputs; numChannels for the number of mods we want to enable
            .withOutput("Modulation", juce::AudioChannelSet::mono(),false)  // Modulation send channel; disabled for all but Modulation preps!
            .withOutput("Send",juce::AudioChannelSet::stereo(),true);       // Send channel (right outputs)
    }
    bool isBusesLayoutSupported (const juce::AudioProcessor::BusesLayout& layouts) const override;

    bool hasEditor() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }

private:

    enum ChainPositions {
        LowCut,
        Peak1,
        Peak2,
        Peak3,
        HighCut
    };

    enum Slope {
        S12,
        S24,
        S36,
        S48
    };
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EQProcessor)
};