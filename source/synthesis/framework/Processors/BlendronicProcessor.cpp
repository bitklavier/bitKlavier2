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
#include "synth_base.h"

BlendronicProcessor::BlendronicProcessor (SynthBase& parent, const juce::ValueTree& vt) : PluginBase (parent, vt, nullptr, blendronicBusLayout())
{
    // for testing
    bufferDebugger = new BufferDebugger();

    /**
     * todo: set this constructor params from default params?
     */
    delay = std::make_unique<BlendronicDelay>(44100 * 10., 0., 1, 44100 * 10, getSampleRate());

    /*
     * state-change parameter stuff (for multisliders)
     */
    state.params.beatLengths.stateChanges.defaultState      = v.getOrCreateChildWithName(IDs::PARAM_DEFAULT,nullptr);
    state.params.delayLengths.stateChanges.defaultState     = v.getOrCreateChildWithName(IDs::PARAM_DEFAULT,nullptr);
    state.params.smoothingTimes.stateChanges.defaultState   = v.getOrCreateChildWithName(IDs::PARAM_DEFAULT,nullptr);
    state.params.feedbackCoeffs.stateChanges.defaultState   = v.getOrCreateChildWithName(IDs::PARAM_DEFAULT,nullptr);

    parent.getStateBank().addParam (std::make_pair<std::string,
        bitklavier::ParameterChangeBuffer*> (v.getProperty (IDs::uuid).toString().toStdString() + "_" + "beat_lengths",
        &(state.params.beatLengths.stateChanges)));
    parent.getStateBank().addParam (std::make_pair<std::string,
        bitklavier::ParameterChangeBuffer*> (v.getProperty (IDs::uuid).toString().toStdString() + "_" + "delay_lengths",
        &(state.params.delayLengths.stateChanges)));
    parent.getStateBank().addParam (std::make_pair<std::string,
        bitklavier::ParameterChangeBuffer*> (v.getProperty (IDs::uuid).toString().toStdString() + "_" + "smoothing_times",
        &(state.params.smoothingTimes.stateChanges)));
    parent.getStateBank().addParam (std::make_pair<std::string,
        bitklavier::ParameterChangeBuffer*> (v.getProperty (IDs::uuid).toString().toStdString() + "_" + "feedback_coefficients",
        &(state.params.feedbackCoeffs.stateChanges)));
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

    prevBeat = state.params.beatLengths.sliderVals[0].load();
    prevDelay = state.params.delayLengths.sliderVals[0].load();

    delay->setSampleRate(getSampleRate());

    beatIndex = 0;
    delayIndex = 0;
    smoothIndex = 0;
    feedbackIndex = 0;

    pulseLength = (60.0 / (_subdivisions * _tempo));
    pulseLength *= (_generalSettingsPeriodMultiplier * _periodMultiplier);
    numSamplesBeat = state.params.beatLengths.sliderVals[beatIndex].load();
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
    numSamplesDelay = state.params.delayLengths.sliderVals[delayIndex].load() * pulseLength * getSampleRate();
    if (pulseLength == INFINITY) numSamplesDelay = 0;

    float delayDelta = fabsf(prevDelay - state.params.delayLengths.sliderVals[delayIndex].load());
    prevDelay = state.params.delayLengths.sliderVals[delayIndex].load();

    float smoothRate = (pulseLength * delayDelta) / (state.params.smoothingTimes.sliderVals[smoothIndex].load() * 0.001f);
    if (delayDelta == 0 || pulseLength == INFINITY) smoothRate = INFINITY;

    delay->setDelayTargetLength(numSamplesDelay);
    delay->setSmoothRate(smoothRate); // this is really a rate, not a duration
    delay->setFeedback(state.params.feedbackCoeffs.sliderVals[feedbackIndex].load());

//    DBG("===== BlendronicProcessor::updateDelayParameters =====");
//    DBG("beat length    = " + juce::String(state.params.beatLengths.sliderVals[beatIndex]));
//    DBG("delay length   = " + juce::String(state.params.delayLengths.sliderVals[delayIndex]));
//    DBG("smooth time    = " + juce::String(state.params.smoothingTimes.sliderVals[smoothIndex]));
//    DBG("feedback coeff = " + juce::String(state.params.feedbackCoeffs.sliderVals[feedbackIndex]));
}

void BlendronicProcessor::tick(float* inL, float* inR)
{
    /**
     * todo: does this ever get called, given the same check later in tick()?
     */
    if (pulseLength != prevPulseLength) numSamplesBeat = state.params.beatLengths.sliderVals[beatIndex].load() * pulseLength * getSampleRate();

    // Check for beat change
    if (sampleTimer >= numSamplesBeat)
    {
        /*
         * everything in here only happens every beat, so not very often, so we don't need to overly optimize
         */

//        // Clear if we need to
//        /**
//         * todo: is setClearDelayOnNextBeat actually used?
//         */
//        if (clearDelayOnNextBeat)
//        {
//            delay->clear();
//            /**
//             * todo: for display
//             */
////            for (auto channel : audio)
////                channel->clear();
//            clearDelayOnNextBeat = false;
//        }

        /**
         * for display?
         */
//        float beatPatternLength = 0.0;
//        for (int i=0; i<state.params.beatLengths.sliderVals_size; i++) {
//            beatPatternLength += state.params.beatLengths.sliderVals[i].load() * pulseLength * getSampleRate();
//        }

//        if (numBeatPositions != (int)((delay->getDelayBuffer()->getNumSamples() / beatPatternLength) * state.params.beatLengths.sliderVals_size) - 1)
//        {
//            beatPositionsInBuffer.clear();
//            beatPositionsIndex = -1;
//            pulseOffset = delay->getInPoint();
//            numBeatPositions = ((delay->getDelayBuffer()->getNumSamples() / beatPatternLength) * state.params.beatLengths.sliderVals_size) - 1;
//        }
//
//        // Set the next beat position, cycle if we've reached the max number of positions for the buffer
//        beatPositionsInBuffer.set(++beatPositionsIndex, delay->getInPoint());
//        if (beatPositionsIndex >= numBeatPositions) beatPositionsIndex = -1;

        // Step sequenced params
        beatIndex++;
        if (beatIndex >= state.params.beatLengths.sliderVals_size) beatIndex = 0;
        delayIndex++;
        if (delayIndex >= state.params.delayLengths.sliderVals_size) delayIndex = 0;
        smoothIndex++;
        if (smoothIndex >= state.params.smoothingTimes.sliderVals_size) smoothIndex = 0;
        feedbackIndex++;
        if (feedbackIndex >= state.params.feedbackCoeffs.sliderVals_size) feedbackIndex = 0;

        // Update numSamplesBeat for the new beat and reset sampleTimer
        numSamplesBeat = state.params.beatLengths.sliderVals[beatIndex].load() * pulseLength * getSampleRate();
        sampleTimer = 0;

        updateDelayParameters();
    }

    sampleTimer++;

    if (pulseLength != prevPulseLength)
    {
        // Set parameters of the delay object
        updateDelayParameters();

        prevPulseLength = pulseLength;

        /**
         * for display?
         */
//        resetPhase = true;
    }

    // Tick the delay
    delay->tick(inL, inR);

    /**
     * todo: for display
     */
//    float dlr = 0.0f;
//    if (pulseLength != INFINITY) dlr = delay->getDelayLength() / (pulseLength * getSampleRate());
//
//    int i = delay->getInPoint() - 1;
//    if (i < 0) i = delay->getDelayBuffer()->getNumSamples() - 1;
//    for (auto channel : audio)
//        channel->pushSample (delay->getSample(0, i));
//    smoothing->pushSample(dlr);
}

void BlendronicProcessor::clearNextDelayBlock(int numSamples)
{
    for (int i = 0; i < numSamples; i++)
    {
        delay->scalePrevious(0, i, 0);
        delay->scalePrevious(0, i, 1);
    }
}

void BlendronicProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    /**
     * MIDI Targeting Stuff First
     */

    for (auto mi : midiMessages)
    {
        auto message = mi.getMessage();
        DBG("BlendronicProcessor::processBlock MIDI message received on channel " + juce::String(message.getChannel()));
    }


    /**
     * Then the Audio Stuff
     */
    int numSamples = buffer.getNumSamples();

    /*
     * use these to display buffer info to bufferDebugger
     */
    bufferDebugger->capture("L", buffer.getReadPointer(0), numSamples, -1.f, 1.f);
    bufferDebugger->capture("R", buffer.getReadPointer(1), numSamples, -1.f, 1.f);

    /*
     * process any mod changes to the multisliders
     */
    state.params.processStateChanges();

    // apply the input gain multiplier
    auto inputgainmult = bitklavier::utils::dbToMagnitude (state.params.inputGain->getCurrentValue());
    buffer.applyGain(0, 0, numSamples, inputgainmult);
    buffer.applyGain(1, 0, numSamples, inputgainmult);
    /**
     * todo: level meter for input? yes, place on left side of prep pop_up
     */

    auto outL = buffer.getWritePointer(0, 0);
    auto outR = buffer.getWritePointer(1, 0);
    if (outL == nullptr) return;
    if (outR == nullptr) return;

    /**
     * todo: General Settings and Tempo
     */
    // update some params
    //    pulseLength = (60.0 / (tempoPrep->getSubdivisions() * tempoPrep->getTempo()));
    //    pulseLength *= (general->getPeriodMultiplier() * tempo->getPeriodMultiplier());
    pulseLength = (60.0 / (_subdivisions * _tempo));
    pulseLength *= (_generalSettingsPeriodMultiplier * _periodMultiplier);

    // clears the end of the delay buffer so we aren't writing on top of really old stuff
    clearNextDelayBlock(numSamples);

    // apply the delay
    while (--numSamples >= 0)
    {
        tick(outL++, outR++);
    }

    // reset for the rest of the calls here
    numSamples = buffer.getNumSamples();

    // output send
    int sendBufferIndex = getChannelIndexInProcessBlockBuffer (false, 2, 0);
    auto sendgainmult = bitklavier::utils::dbToMagnitude (state.params.outputSend->getCurrentValue());
    buffer.copyFrom(sendBufferIndex, 0, buffer.getReadPointer(0), numSamples, sendgainmult);
    buffer.copyFrom(sendBufferIndex+1, 0, buffer.getReadPointer(1), numSamples, sendgainmult);

    // final output gain stage, from rightmost slider in DirectParametersView
    auto outputgainmult = bitklavier::utils::dbToMagnitude (state.params.outputGain->getCurrentValue());
    buffer.applyGain(0, 0, numSamples, outputgainmult);
    buffer.applyGain(1, 0, numSamples, outputgainmult);

    // level meter update stuff
    std::get<0> (state.params.outputLevels) = buffer.getRMSLevel (0, 0, numSamples);
    std::get<1> (state.params.outputLevels) = buffer.getRMSLevel (1, 0, numSamples);

    /**
     * todo: level meter for output send? yes, place on right side of prep pop_up, just to the left of the main outputgain slider
     */
}

void BlendronicProcessor::processBlockBypassed (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    /**
     * todo: perhaps have a fadeout param, followed by a buffer clear?
     * - these could be user settable, perhaps...
     */
}


/**
 * Serializers, for saving/loading complex params like the multisliders
 */
template <typename Serializer>
typename Serializer::SerializedType BlendronicParams::serialize (const BlendronicParams& paramHolder)
{
    /*
     * first, call the default serializer, which gets all the simple params
     */
    auto ser = chowdsp::ParamHolder::serialize<Serializer> (paramHolder);

    /*
     * then serialize the more complex params
     */
    serializeMultiSliderParam<Serializer> (ser, paramHolder.beatLengths, "beat_lengths");
    serializeMultiSliderParam<Serializer> (ser, paramHolder.delayLengths, "delay_lengths");
    serializeMultiSliderParam<Serializer> (ser, paramHolder.smoothingTimes, "smoothing_times");
    serializeMultiSliderParam<Serializer> (ser, paramHolder.feedbackCoeffs, "feedback_coeffs");

    return ser;
}

template <typename Serializer>
void BlendronicParams::deserialize (typename Serializer::DeserializedType deserial, BlendronicParams& paramHolder)
{
    /*
     * call the default deserializer first, for the simple params
     */
    chowdsp::ParamHolder::deserialize<Serializer> (deserial, paramHolder);

    /*
     * then the more complex params
     */
    deserializeMultiSliderParam<Serializer> (deserial, paramHolder.beatLengths, "beat_lengths");
    deserializeMultiSliderParam<Serializer> (deserial, paramHolder.delayLengths, "delay_lengths");
    deserializeMultiSliderParam<Serializer> (deserial, paramHolder.smoothingTimes, "smoothing_times");
    deserializeMultiSliderParam<Serializer> (deserial, paramHolder.feedbackCoeffs, "feedback_coeffs");
}

