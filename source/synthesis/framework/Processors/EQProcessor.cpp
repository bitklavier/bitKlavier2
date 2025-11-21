//
// Created by Myra Norton on 11/14/25.
//

#include "EQProcessor.h"
#include "Synthesiser/Sample.h"
#include "common.h"
#include "synth_base.h"

EQProcessor::EQProcessor (SynthBase& parent, const juce::ValueTree& vt)
    : PluginBase (parent, vt, nullptr, eqBusLayout())
{
    parent.getValueTree().addListener(this);
}

void EQProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const auto spec = juce::dsp::ProcessSpec { sampleRate, (uint32_t) samplesPerBlock, (uint32_t) getMainBusNumInputChannels() };
}

bool EQProcessor::isBusesLayoutSupported (const juce::AudioProcessor::BusesLayout& layouts) const
{
    return true;
}

void EQProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    buffer.clear();
    state.getParameterListeners().callAudioThreadBroadcasters();
    state.params.processStateChanges();
}