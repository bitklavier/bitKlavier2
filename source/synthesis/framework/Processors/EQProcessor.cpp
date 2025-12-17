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
    // parent.getValueTree().addListener(this);
    // state.params.sampleRate = getSampleRate();
}

void EQProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const auto spec = juce::dsp::ProcessSpec { sampleRate, (uint32_t) samplesPerBlock, (uint32_t) getMainBusNumInputChannels() };
    state.params.leftChain.prepare(spec);
    state.params.rightChain.prepare(spec);
    state.params.updateCoefficients();
}

bool EQProcessor::isBusesLayoutSupported (const juce::AudioProcessor::BusesLayout& layouts) const
{
    return true;
}

void EQProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    state.getParameterListeners().callAudioThreadBroadcasters();
    state.params.processStateChanges();

    juce::dsp::AudioBlock<float> block(buffer);
    auto leftBlock = block.getSingleChannelBlock(0);
    juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
    state.params.leftChain.process(leftContext);
    if (buffer.getNumChannels() > 1)
    {
        auto rightBlock = block.getSingleChannelBlock(1);
        juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);
        state.params.rightChain.process(rightContext);
    }
}