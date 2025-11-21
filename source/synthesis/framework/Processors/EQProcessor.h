//
// Created by Myra Norton on 11/14/25.
//

#pragma once
#include "Identifiers.h"
#include "PluginBase.h"
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
            // add();
    }
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
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EQProcessor)
};