//
// Created by Myra Norton on 11/13/25.
//

#include "GainProcessor.h"

#include "common.h"
#include "synth_base.h"

GainProcessor::GainProcessor (SynthBase& parent, const juce::ValueTree& vt)
    : PluginBase (parent, vt, nullptr, gainBusLayout())
{
}

/*
 * this is where we define the buses for audio in/out, including the param modulation channels
 *      the "discreteChannels" number is currently just by hand set based on the max that this particularly preparation could have
 *      so if you add new params, might need to increase that number
 */
juce::AudioProcessor::BusesProperties GainProcessor::gainBusLayout()
{
    return BusesProperties()
        .withOutput("Output", juce::AudioChannelSet::stereo(), true) // Main Output
        .withInput ("Input", juce::AudioChannelSet::stereo(), false)  // Main Input (not used here)

        /**
         * IMPORTANT: set discreteChannels below equal to the number of params you want to continuously modulate!!
         *              for Gain, we have 10:
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

void GainProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    state.params.processStateChanges();
    int numSamples = buffer.getNumSamples();

    // final output gain stage, from rightmost slider in DirectParametersView
    auto outputgainmult = bitklavier::utils::dbToMagnitude (state.params.outputGain->getCurrentValue());
    buffer.applyGain(0, 0, numSamples, outputgainmult);
    buffer.applyGain(1, 0, numSamples, outputgainmult);
    // main level meter update
    std::get<0> (state.params.outputLevels) = buffer.getRMSLevel (0, 0, numSamples);
    std::get<1> (state.params.outputLevels) = buffer.getRMSLevel (1, 0, numSamples);
}
