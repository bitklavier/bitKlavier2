/*
==============================================================================

  Blendronic.h
  Created: 11 Jun 2019 2:00:53pm
  Author:  Theodore R Trevisan

  The original algorithm for Blendrónic was developed by Dan for the Feedback
  movement from "neither Anvil nor Pulley," and was subsequently used in
  Clapping Machine Music Variations, Olagón, and others. A paper describing
  the original algorithm was presented at the International Computer Music
  Conference in 2010 (http://www.manyarrowsmusic.com/papers/cmmv.pdf).

  "Clapping Machine Music Variations: A Composition for Acoustic/Laptop Ensemble"
  Dan Trueman
  Proceedings for the International Computer Music Conference
  SUNY Stony Brook, 2010

  The basic idea is that the length of a delay line changes periodically, as
  set by a sequence of beat lengths; the changes can happen instantaneously,
  or can take place over a period of time, a "smoothing" time that creates
  a variety of artifacts, tied to the beat pattern. The smoothing parameters
  themselves can be sequenced in a pattern, as can a feedback coefficient,
  which determines how much of the out of the delay line is fed back into it.

==============================================================================
*/

#include "BlendronicProcessor.h"

BlendronicProcessor::BlendronicProcessor (SynthBase& parent, const juce::ValueTree& vt) : PluginBase (parent, vt, nullptr, blendronicBusLayout())
{
    /**
     * todo: set this constructor params from default params
     */
    delay = std::make_unique<BlendronicDelay>(44100 * 10., 0., 1, 44100 * 10, getSampleRate());

}

void BlendronicProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const auto spec = juce::dsp::ProcessSpec { sampleRate, (uint32_t) samplesPerBlock, (uint32_t) getMainBusNumInputChannels() };

    gain.prepare (spec);
    gain.setRampDurationSeconds (0.05);

    /**
     * todo: for display
     */
//    for (int i = 0; i < 1/*numChannels*/; ++i)
//    {
//        audio.getUnchecked(i)->setBufferSize(delay->getDelayBuffer()->getNumSamples());
//    }
//    smoothing->setBufferSize(delay->getDelayBuffer()->getNumSamples());
//    beatPositionsInBuffer.ensureStorageAllocated(128);
//    beatPositionsInBuffer.clear();
//    beatPositionsInBuffer.add(0);
//    pulseOffset = 0;
//    beatPositionsIndex = 0;

//    prevBeat = prep->bBeats.value[0];
    prevBeat = state.params.beatLengths.sliderVals[0].load();
//    prevDelay = prep->bDelayLengths.value[0];
    prevDelay = state.params.delayLengths.sliderVals[0].load();

    delay->setSampleRate(getSampleRate());

    beatIndex = 0;
    delayIndex = 0;
    smoothIndex = 0;
    feedbackIndex = 0;

//    pulseLength = (60.0 / (tempoPrep->getSubdivisions() * tempoPrep->getTempo()));
//    pulseLength *= (general->getPeriodMultiplier() * tempo->getPeriodMultiplier());
    pulseLength = (60.0 / (_subdivisions * _tempo));
    pulseLength *= (_generalSettingsPeriodMultiplier * _periodMultiplier);
//    numSamplesBeat = prep->bBeats.value[beatIndex] * getSampleRate() * pulseLength;
    numSamplesBeat = state.params.beatLengths.sliderVals[beatIndex].load();
//    numSamplesDelay = prep->bDelayLengths.value[delayIndex] * getSampleRate() * pulseLength;
    numSamplesDelay = state.params.delayLengths.sliderVals[delayIndex].load() * getSampleRate() * pulseLength;

    if (pulseLength == INFINITY)
    {
        numSamplesBeat = INFINITY;
        numSamplesBeat = 0;
    }

    delay->setDelayLength(numSamplesDelay);
    delay->setDelayTargetLength(numSamplesDelay);

    updateDelayParameters();
}

void BlendronicProcessor::updateDelayParameters()
{
//    numSamplesDelay = prep->bDelayLengths.value[delayIndex] * pulseLength * getSampleRate();
    numSamplesDelay = state.params.delayLengths.sliderVals[delayIndex].load() * pulseLength * getSampleRate();
    if (pulseLength == INFINITY) numSamplesDelay = 0;

//    float delayDelta = fabsf(prevDelay - prep->bDelayLengths.value[delayIndex]);
    float delayDelta = fabsf(prevDelay - state.params.delayLengths.sliderVals[delayIndex].load());
//    prevDelay = prep->bDelayLengths.value[delayIndex];
    prevDelay = state.params.delayLengths.sliderVals[delayIndex].load();

//    float smoothRate = (pulseLength * delayDelta) / (prep->bSmoothLengths.value[smoothIndex] * 0.001f);
    float smoothRate = (pulseLength * delayDelta) / (state.params.smoothingTimes.sliderVals[smoothIndex].load() * 0.001f);
    if (delayDelta == 0 || pulseLength == INFINITY) smoothRate = INFINITY;

    // DBG(String(getId()) + " new envelope target = " + String(numSamplesDelay));
    // DBG(String(1000. / smoothRate) + "ms"); // smooth rate is change / second
    delay->setDelayTargetLength(numSamplesDelay);
    delay->setSmoothRate(smoothRate); // this is really a rate, not a duration
//    delay->setFeedback(prep->bFeedbackCoefficients.value[feedbackIndex]);
    delay->setFeedback(state.params.feedbackCoeffs.sliderVals[feedbackIndex].load());
}

void BlendronicProcessor::tick(float* outputs)
{
//    BlendronicPreparation::Ptr prep = blendronic->prep;
//    TempoPreparation::Ptr tempoPrep = tempo->getTempo()->prep;
//
//    if (tempoPrep->getSubdivisions() * tempoPrep->getTempo() == 0) return;

    /**
     * todo: put these pulseLength updates in another function called on the block?
     */
//    pulseLength = (60.0 / (tempoPrep->getSubdivisions() * tempoPrep->getTempo()));
//    pulseLength *= (general->getPeriodMultiplier() * tempo->getPeriodMultiplier());
    pulseLength = (60.0 / (_subdivisions * _tempo));
    pulseLength *= (_generalSettingsPeriodMultiplier * _periodMultiplier);
//    if (pulseLength != prevPulseLength) numSamplesBeat = prep->bBeats.value[beatIndex] * pulseLength * getSampleRate();
    if (pulseLength != prevPulseLength) numSamplesBeat = state.params.beatLengths.sliderVals[beatIndex].load() * pulseLength * getSampleRate();

    // Check for beat change
    if (sampleTimer >= numSamplesBeat)
    {
        // Clear if we need to
        if (clearDelayOnNextBeat)
        {
            delay->clear();
            /**
             * todo: for display
             */
//            for (auto channel : audio)
//                channel->clear();
            clearDelayOnNextBeat = false;
        }

        float beatPatternLength = 0.0;
//        for (auto b : prep->bBeats.value) beatPatternLength += b * pulseLength * getSampleRate();
        for (int i=0; i<state.params.beatLengths.sliderVals.size(); i++) {
            beatPatternLength += state.params.beatLengths.sliderVals[i].load() * pulseLength * getSampleRate();
        }

//        if (numBeatPositions != (int)((delay->getDelayBuffer()->getNumSamples() / beatPatternLength) * prep->bBeats.value.size()) - 1)
        if (numBeatPositions != (int)((delay->getDelayBuffer()->getNumSamples() / beatPatternLength) * state.params.beatLengths.sliderVals.size()) - 1)
        {
            beatPositionsInBuffer.clear();
            beatPositionsIndex = -1;
            pulseOffset = delay->getInPoint();
//            numBeatPositions = ((delay->getDelayBuffer()->getNumSamples() / beatPatternLength) * prep->bBeats.value.size()) - 1;
            numBeatPositions = ((delay->getDelayBuffer()->getNumSamples() / beatPatternLength) * state.params.beatLengths.sliderVals.size()) - 1;
        }

        // Set the next beat position, cycle if we've reached the max number of positions for the buffer
        beatPositionsInBuffer.set(++beatPositionsIndex, delay->getInPoint());
        if (beatPositionsIndex >= numBeatPositions) beatPositionsIndex = -1;

        // Step sequenced params
        beatIndex++;
//        if (beatIndex >= prep->bBeats.value.size()) beatIndex = 0;
        if (beatIndex >= state.params.beatLengths.sliderVals.size()) beatIndex = 0;

        delayIndex++;
//        if (delayIndex >= prep->bDelayLengths.value.size()) delayIndex = 0;
        if (delayIndex >= state.params.delayLengths.sliderVals.size()) delayIndex = 0;

        smoothIndex++;
//        if (smoothIndex >= prep->bSmoothLengths.value.size()) smoothIndex = 0;
        if (smoothIndex >= state.params.smoothingTimes.sliderVals.size()) smoothIndex = 0;

        feedbackIndex++;
//        if (feedbackIndex >= prep->bFeedbackCoefficients.value.size()) feedbackIndex = 0;
        if (feedbackIndex >= state.params.feedbackCoeffs.sliderVals.size()) feedbackIndex = 0;


        // Update numSamplesBeat for the new beat and reset sampleTimer
//        numSamplesBeat = prep->bBeats.value[beatIndex] * pulseLength * getSampleRate();
        numSamplesBeat = state.params.beatLengths.sliderVals[beatIndex].load() * pulseLength * getSampleRate();
        sampleTimer = 0;

        updateDelayParameters();
    }
    sampleTimer++;

    if (pulseLength != prevPulseLength)
    {
        // Set parameters of the delay object
        updateDelayParameters();
        resetPhase = true;
        prevPulseLength = pulseLength;
    }
    //prevPulseLength = pulseLength;

    // Tick the delay
//    delay->tick(outputs, juce::Decibels::decibelsToGain(prep->outGain.value));
    delay->tick(outputs, 1.);

    float dlr = 0.0f;
    if (pulseLength != INFINITY) dlr = delay->getDelayLength() / (pulseLength * getSampleRate());


    int i = delay->getInPoint() - 1;
    if (i < 0) i = delay->getDelayBuffer()->getNumSamples() - 1;

    /**
     * todo: for display
     */
//    for (auto channel : audio)
//        channel->pushSample (delay->getSample(0, i));
//    smoothing->pushSample(dlr);
}


void BlendronicProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{

}

void BlendronicProcessor::processBlockBypassed (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{

}



template <typename Serializer>
typename Serializer::SerializedType BlendronicParams::serialize (const BlendronicParams& paramHolder)
{
    /*
     * first, call the default serializer, which gets all the simple params
     */
    auto ser = chowdsp::ParamHolder::serialize<Serializer> (paramHolder);

    /**
     * todo: make these using blendronicState.beatLengthsActual, rather than MAXMULTISLIDERLENGTH, so we don't write so much to the xml
     */

    /*
     * then serialize the more complex params
     */
    //    Serializer::template addChildElement<MAXMULTISLIDERLENGTH> (ser, "blendronic_beatLengths", paramHolder.beatLengths.sliderVals, arrayToString);

    return ser;
}

template <typename Serializer>
void BlendronicParams::deserialize (typename Serializer::DeserializedType deserial, BlendronicParams& paramHolder)
{
    chowdsp::ParamHolder::deserialize<Serializer> (deserial, paramHolder);

    auto myStr = deserial->getStringAttribute ("blendronic_beatLengths");
    //    paramHolder.beatLengths.sliderVals = parseFloatStringToArrayCircular<MAXMULTISLIDERLENGTH> (myStr.toStdString());
}

