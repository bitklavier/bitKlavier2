//
// Created by Myra Norton on 11/14/25.
//

#include "CompressorProcessor.h"
#include "Synthesiser/Sample.h"
#include "common.h"
#include "synth_base.h"

CompressorProcessor::CompressorProcessor (SynthBase& parent, const juce::ValueTree& vt)
    : PluginBase (parent, vt, nullptr, compressorBusLayout())
{
    parent.getValueTree().addListener(this);
}

void CompressorProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const auto spec = juce::dsp::ProcessSpec { sampleRate, (uint32_t) samplesPerBlock, (uint32_t) getMainBusNumInputChannels() };
}

bool CompressorProcessor::isBusesLayoutSupported (const juce::AudioProcessor::BusesLayout& layouts) const
{
    return true;
}

void CompressorProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    buffer.clear();
    state.getParameterListeners().callAudioThreadBroadcasters();
    state.params.processStateChanges();
}