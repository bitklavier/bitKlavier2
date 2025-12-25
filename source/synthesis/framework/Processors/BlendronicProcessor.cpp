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

    // note: we are setting the buffer size to 10 seconds max here (delayBufferSize)
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
    prevBeat = state.params.beatLengths.sliderVals[0].load();
    prevDelay = state.params.delayLengths.sliderVals[0].load();

    delay->setSampleRate(getSampleRate());

    beatIndex = 0;
    delayIndex = 0;
    smoothIndex = 0;
    feedbackIndex = 0;

    /**
     * todo: get General Settings
     */
    pulseLength = getPulseLength();
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

void BlendronicProcessor::doPatternSync()
{
    beatIndex = state.params.beatLengths.sliderVals_size;
    delayIndex = state.params.delayLengths.sliderVals_size;
    smoothIndex = state.params.smoothingTimes.sliderVals_size;
    feedbackIndex = state.params.feedbackCoeffs.sliderVals_size;
}

void BlendronicProcessor::doBeatSync()
{
    sampleTimer = numSamplesBeat;
}

void BlendronicProcessor::doClear()
{
    delay->clear();
}

void BlendronicProcessor::doPausePlay()
{
    toggleActive();
}

void BlendronicProcessor::doOpenCloseInput()
{
    delay->toggleInput();
}

void BlendronicProcessor::doOpenCloseOutput()
{
    delay->toggleOutput();
}

void BlendronicProcessor::tick(float* inL, float* inR)
{
    // used for pause/play
    if (!blendronicActive) return;

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
    }

    // Tick the delay
    delay->tick(inL, inR);

}

void BlendronicProcessor::clearNextDelayBlock(int numSamples)
{
    for (int i = 0; i < numSamples; i++)
    {
        delay->scalePrevious(0, i, 0);
        delay->scalePrevious(0, i, 1);
    }
}

/**
 * Blendrónic receives MIDI messages for various targeted behaviors:
 *      - sync
 *      - clear
 *      - pause play,
 *      - and open/close inputs and outputs
 *
 * the message targets are determined by MIDI channel, and are set in a
 * MidiTarget preparation placed between a Keymap and Blendrónic
 *
 * @param midiMessages
 */
void BlendronicProcessor::handleMidiTargetMessages(juce::MidiBuffer& midiMessages)
{
    for (auto mi : midiMessages)
    {
        auto message = mi.getMessage();

        /*
         * 'BlendronicTargetPatternSync' is the first in the set of Blendrónic
         * PreparationParameterTargetTypes, so determines the offset for
         * the channel that the target messages are received on.
         */
        switch(message.getChannel() + (BlendronicTargetFirst))
        {
            case BlendronicTargetPatternSync:
                doPatternSync();
                break;

            case BlendronicTargetBeatSync:
                doBeatSync();
                break;

            case BlendronicTargetClear:
                doClear();
                break;

            case BlendronicTargetPausePlay:
                doPausePlay();
                break;

            case BlendronicTargetInput:
                doOpenCloseInput();
                break;

            case BlendronicTargetOutput:
                doOpenCloseOutput();
                break;
        }
    }
}



void BlendronicProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    /**
     * todo: General Settings and Tempo
     */
    /*
     * Update some params from Tempo and General Settings
     *      pulseLength = (60.0 / (tempoPrep->getSubdivisions() * tempoPrep->getTempo()));
     *      pulseLength *= (general->getPeriodMultiplier() * tempo->getPeriodMultiplier());
     */
    pulseLength = getPulseLength();
    pulseLength *= (_generalSettingsPeriodMultiplier * _periodMultiplier);


    // process any mod changes to the multisliders
    state.params.processStateChanges();

    /*
     * MIDI Targeting Stuff First
     */
    handleMidiTargetMessages(midiMessages);

    /*
     * Then the Audio Stuff
     */
    int numSamples = buffer.getNumSamples();

    // use these to display buffer info to bufferDebugger
    // bufferDebugger->capture("L", buffer.getReadPointer(0), numSamples, -1.f, 1.f);
    // bufferDebugger->capture("R", buffer.getReadPointer(1), numSamples, -1.f, 1.f);

    // apply the input gain multiplier
    auto inputgainmult = bitklavier::utils::dbToMagnitude (state.params.inputGain->getCurrentValue());
    buffer.applyGain(0, 0, numSamples, inputgainmult);
    buffer.applyGain(1, 0, numSamples, inputgainmult);

    // input level meter update stuff
    std::get<0> (state.params.inputLevels) = buffer.getRMSLevel (0, 0, numSamples);
    std::get<1> (state.params.inputLevels) = buffer.getRMSLevel (1, 0, numSamples);

    // get the pointers for the samples to read from and write to
    auto outL = buffer.getWritePointer(0, 0);
    auto outR = buffer.getWritePointer(1, 0);
    if (outL == nullptr) return;
    if (outR == nullptr) return;

    // clear the end of the delay buffer so we aren't writing on top of really old stuff
    clearNextDelayBlock(numSamples);

    // apply the delay
    while (--numSamples >= 0)
    {
        tick(outL++, outR++);
    }

    // update current slider val for UI
    state.params.beatLengths_current.store(beatIndex);
    state.params.delayLengths_current.store(delayIndex);
    state.params.smoothingTimes_current.store(smoothIndex);
    state.params.feedbackCoeffs_current.store(feedbackIndex);

    // reset for the rest of the calls here
    numSamples = buffer.getNumSamples();

    // handle the send
    int sendBufferIndex = getChannelIndexInProcessBlockBuffer (false, 2, 0);
    auto sendgainmult = bitklavier::utils::dbToMagnitude (state.params.outputSend->getCurrentValue());
    buffer.copyFrom(sendBufferIndex, 0, buffer.getReadPointer(0), numSamples, sendgainmult);
    buffer.copyFrom(sendBufferIndex+1, 0, buffer.getReadPointer(1), numSamples, sendgainmult);

    // send level meter update
    std::get<0> (state.params.sendLevels) = buffer.getRMSLevel (sendBufferIndex, 0, numSamples);
    std::get<1> (state.params.sendLevels) = buffer.getRMSLevel (sendBufferIndex+1, 0, numSamples);

    // final output gain stage, from rightmost slider in DirectParametersView
    auto outputgainmult = bitklavier::utils::dbToMagnitude (state.params.outputGain->getCurrentValue());
    buffer.applyGain(0, 0, numSamples, outputgainmult);
    buffer.applyGain(1, 0, numSamples, outputgainmult);

    // main level meter update
    std::get<0> (state.params.outputLevels) = buffer.getRMSLevel (0, 0, numSamples);
    std::get<1> (state.params.outputLevels) = buffer.getRMSLevel (1, 0, numSamples);
}

void BlendronicProcessor::processBlockBypassed (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    /**
     * I think for now at least Blendronic just doesn't produce sound if it is bypassed
     * - if the user wants it present across piano changes, they should use a Linked version
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

