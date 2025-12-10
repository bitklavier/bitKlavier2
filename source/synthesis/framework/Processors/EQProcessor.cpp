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
    leftChain.prepare(spec);
    rightChain.prepare(spec);
    updateCoefficients();
}

bool EQProcessor::isBusesLayoutSupported (const juce::AudioProcessor::BusesLayout& layouts) const
{
    return true;
}

void EQProcessor::updateCoefficients()
{
    auto peak1Coefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(),
        state.params.peak1FilterParams.filterFreq->getCurrentValue(),
        state.params.peak1FilterParams.filterQ->getCurrentValue(),
        state.params.peak1FilterParams.filterGain->getCurrentValue());
    auto peak2Coefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(),
        state.params.peak2FilterParams.filterFreq->getCurrentValue(),
        state.params.peak2FilterParams.filterQ->getCurrentValue(),
        state.params.peak2FilterParams.filterGain->getCurrentValue());
    auto peak3Coefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(),
        state.params.peak3FilterParams.filterFreq->getCurrentValue(),
        state.params.peak3FilterParams.filterQ->getCurrentValue(),
        state.params.peak3FilterParams.filterGain->getCurrentValue());

    *leftChain.get<ChainPositions::Peak1>().coefficients = *peak1Coefficients;
    *leftChain.get<ChainPositions::Peak2>().coefficients = *peak2Coefficients;
    *leftChain.get<ChainPositions::Peak3>().coefficients = *peak3Coefficients;
    *rightChain.get<ChainPositions::Peak1>().coefficients = *peak1Coefficients;
    *rightChain.get<ChainPositions::Peak2>().coefficients = *peak2Coefficients;
    *rightChain.get<ChainPositions::Peak3>().coefficients = *peak3Coefficients;

    leftChain.setBypassed<ChainPositions::Peak1>(!state.params.peak1FilterParams.filterActive->get());
    leftChain.setBypassed<ChainPositions::Peak2>(!state.params.peak2FilterParams.filterActive->get());
    leftChain.setBypassed<ChainPositions::Peak3>(!state.params.peak3FilterParams.filterActive->get());
    rightChain.setBypassed<ChainPositions::Peak1>(!state.params.peak1FilterParams.filterActive->get());
    rightChain.setBypassed<ChainPositions::Peak2>(!state.params.peak2FilterParams.filterActive->get());
    rightChain.setBypassed<ChainPositions::Peak3>(!state.params.peak3FilterParams.filterActive->get());

    // calculate and set low cut filter coefficients
    int lowCutSlope = state.params.loCutFilterParams.filterSlope->getCurrentValue();
    auto lowCutCoefficients = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(
        state.params.loCutFilterParams.filterFreq->getCurrentValue(),
        getSampleRate(),
        lowCutSlope / 6);

    auto& leftLowCutChain = leftChain.get<ChainPositions::LowCut>();
    leftLowCutChain.setBypassed<Slope::S12>(true);
    leftLowCutChain.setBypassed<Slope::S24>(true);
    leftLowCutChain.setBypassed<Slope::S36>(true);
    leftLowCutChain.setBypassed<Slope::S48>(true);
    auto& rightLowCutChain = rightChain.get<ChainPositions::LowCut>();
    rightLowCutChain.setBypassed<Slope::S12>(true);
    rightLowCutChain.setBypassed<Slope::S24>(true);
    rightLowCutChain.setBypassed<Slope::S36>(true);
    rightLowCutChain.setBypassed<Slope::S48>(true);

    // breaks omitted on purpose to facilitate fall through
    switch(lowCutSlope) {
        case 48: {
            *leftLowCutChain.get<Slope::S48>().coefficients =
            *lowCutCoefficients[Slope::S48];
            leftLowCutChain.setBypassed<Slope::S48>(!state.params.loCutFilterParams.filterActive->get());
            *rightLowCutChain.get<Slope::S48>().coefficients =
            *lowCutCoefficients[Slope::S48];
            rightLowCutChain.setBypassed<Slope::S48>(!state.params.loCutFilterParams.filterActive->get());
        }
        case 36: {
            *leftLowCutChain.get<Slope::S36>().coefficients =
            *lowCutCoefficients[Slope::S36];
            leftLowCutChain.setBypassed<Slope::S36>(!state.params.loCutFilterParams.filterActive->get());
            *rightLowCutChain.get<Slope::S36>().coefficients =
            *lowCutCoefficients[Slope::S36];
            rightLowCutChain.setBypassed<Slope::S36>(!state.params.loCutFilterParams.filterActive->get());
        }
        case 24: {
            *leftLowCutChain.get<Slope::S24>().coefficients =
            *lowCutCoefficients[Slope::S24];
            leftLowCutChain.setBypassed<Slope::S24>(!state.params.loCutFilterParams.filterActive->get());
            *rightLowCutChain.get<Slope::S24>().coefficients =
            *lowCutCoefficients[Slope::S24];
            rightLowCutChain.setBypassed<Slope::S24>(!state.params.loCutFilterParams.filterActive->get());
        }
        case 12: {
            *leftLowCutChain.get<Slope::S12>().coefficients =
            *lowCutCoefficients[Slope::S12];
            leftLowCutChain.setBypassed<Slope::S12>(!state.params.loCutFilterParams.filterActive->get());
            *rightLowCutChain.get<Slope::S12>().coefficients =
            *lowCutCoefficients[Slope::S12];
            rightLowCutChain.setBypassed<Slope::S12>(!state.params.loCutFilterParams.filterActive->get());
        }
    }

    // calculate and set high cut filter coefficients
    int highCutSlope = state.params.hiCutFilterParams.filterSlope->getCurrentValue();
    auto highCutCoefficients = juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(
        state.params.hiCutFilterParams.filterFreq->getCurrentValue(),
        getSampleRate(),
        highCutSlope / 6);

    auto& leftHighCutChain = leftChain.get<ChainPositions::HighCut>();
    leftHighCutChain.setBypassed<Slope::S12>(true);
    leftHighCutChain.setBypassed<Slope::S24>(true);
    leftHighCutChain.setBypassed<Slope::S36>(true);
    leftHighCutChain.setBypassed<Slope::S48>(true);
    auto& rightHighCutChain = rightChain.get<ChainPositions::HighCut>();
    rightHighCutChain.setBypassed<Slope::S12>(true);
    rightHighCutChain.setBypassed<Slope::S24>(true);
    rightHighCutChain.setBypassed<Slope::S36>(true);
    rightHighCutChain.setBypassed<Slope::S48>(true);

    // breaks omitted on purpose to facilitate fall through
    switch(highCutSlope) {
        case 48: {
            *leftHighCutChain.get<Slope::S48>().coefficients = *highCutCoefficients[Slope::S48];
            leftHighCutChain.setBypassed<Slope::S48>(!state.params.hiCutFilterParams.filterActive->get());
            *rightHighCutChain.get<Slope::S48>().coefficients = *highCutCoefficients[Slope::S48];
            rightHighCutChain.setBypassed<Slope::S48>(!state.params.hiCutFilterParams.filterActive->get());
        }
        case 36: {
            *leftHighCutChain.get<Slope::S36>().coefficients = *highCutCoefficients[Slope::S36];
            leftHighCutChain.setBypassed<Slope::S36>(!state.params.hiCutFilterParams.filterActive->get());
            *rightHighCutChain.get<Slope::S36>().coefficients = *highCutCoefficients[Slope::S36];
            rightHighCutChain.setBypassed<Slope::S36>(!state.params.hiCutFilterParams.filterActive->get());
        }
        case 24: {
            *leftHighCutChain.get<Slope::S24>().coefficients = *highCutCoefficients[Slope::S24];
            leftHighCutChain.setBypassed<Slope::S24>(!state.params.hiCutFilterParams.filterActive->get());
            *rightHighCutChain.get<Slope::S24>().coefficients = *highCutCoefficients[Slope::S24];
            rightHighCutChain.setBypassed<Slope::S24>(!state.params.hiCutFilterParams.filterActive->get());
        }
        case 12: {
            *leftHighCutChain.get<Slope::S12>().coefficients = *highCutCoefficients[Slope::S12];
            leftHighCutChain.setBypassed<Slope::S12>(!state.params.hiCutFilterParams.filterActive->get());
            *rightHighCutChain.get<Slope::S12>().coefficients = *highCutCoefficients[Slope::S12];
            rightHighCutChain.setBypassed<Slope::S12>(!state.params.hiCutFilterParams.filterActive->get());
        }
    }
}


void EQProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    buffer.clear();
    state.getParameterListeners().callAudioThreadBroadcasters();
    state.params.processStateChanges();

    juce::dsp::AudioBlock<float> block(buffer);
    auto leftBlock = block.getSingleChannelBlock(0);
    juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
    leftChain.process(leftContext);
    if (buffer.getNumChannels() > 1)
    {
        auto rightBlock = block.getSingleChannelBlock(1);
        juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);
        rightChain.process(rightContext);
    }
}

double EQProcessor::magForFreq(double freq) {
    double mag = 1.f;

    // Since leftChain and rightChain use the same coefficients, it's fine to just get them from left
    if (!leftChain.isBypassed<ChainPositions::Peak1>())
        mag *= leftChain.get<ChainPositions::Peak1>()
        .coefficients->getMagnitudeForFrequency(freq, getSampleRate());
    if (!leftChain.isBypassed<ChainPositions::Peak2>())
        mag *= leftChain.get<ChainPositions::Peak2>()
        .coefficients->getMagnitudeForFrequency(freq, getSampleRate());
    if (!leftChain.isBypassed<ChainPositions::Peak3>())
        mag *= leftChain.get<ChainPositions::Peak3>()
        .coefficients->getMagnitudeForFrequency(freq, getSampleRate());

    if(!leftChain.get<ChainPositions::LowCut>().isBypassed<0>())
        mag *= leftChain.get<ChainPositions::LowCut>().get<0>()
        .coefficients->getMagnitudeForFrequency(freq, getSampleRate());
    if(!leftChain.get<ChainPositions::LowCut>().isBypassed<1>())
        mag *= leftChain.get<ChainPositions::LowCut>().get<1>()
        .coefficients->getMagnitudeForFrequency(freq, getSampleRate());
    if(!leftChain.get<ChainPositions::LowCut>().isBypassed<2>())
        mag *= leftChain.get<ChainPositions::LowCut>().get<2>()
        .coefficients->getMagnitudeForFrequency(freq, getSampleRate());
    if(!leftChain.get<ChainPositions::LowCut>().isBypassed<3>())
        mag *= leftChain.get<ChainPositions::LowCut>().get<3>()
        .coefficients->getMagnitudeForFrequency(freq, getSampleRate());

    if(!leftChain.get<ChainPositions::HighCut>().isBypassed<0>())
        mag *= leftChain.get<ChainPositions::HighCut>().get<0>()
        .coefficients->getMagnitudeForFrequency(freq, getSampleRate());
    if(!leftChain.get<ChainPositions::HighCut>().isBypassed<1>())
        mag *= leftChain.get<ChainPositions::HighCut>().get<1>()
        .coefficients->getMagnitudeForFrequency(freq, getSampleRate());
    if(!leftChain.get<ChainPositions::HighCut>().isBypassed<2>())
        mag *= leftChain.get<ChainPositions::HighCut>().get<2>()
        .coefficients->getMagnitudeForFrequency(freq, getSampleRate());
    if(!leftChain.get<ChainPositions::HighCut>().isBypassed<3>())
        mag *= leftChain.get<ChainPositions::HighCut>().get<3>()
        .coefficients->getMagnitudeForFrequency(freq, getSampleRate());

    return mag;
}