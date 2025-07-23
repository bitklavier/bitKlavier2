//
// Created by Davis Polito on 5/2/24.
//

#include "DirectProcessor.h"
#include "Synthesiser/Sample.h"
#include "common.h"
#include "synth_base.h"
DirectProcessor::DirectProcessor (SynthBase& parent, const juce::ValueTree& vt) : PluginBase (parent, vt, nullptr, directBusLayout()),
                                                                                  mainSynth (new BKSynthesiser (state.params.env, state.params.gainParam)),
                                                                                  hammerSynth (new BKSynthesiser (state.params.env, state.params.hammerParam)),
                                                                                  releaseResonanceSynth (new BKSynthesiser (state.params.env, state.params.releaseResonanceParam)),
                                                                                  pedalSynth (new BKSynthesiser (state.params.env, state.params.pedalParam))
{
    for (int i = 0; i < 300; i++)
    {
        mainSynth->addVoice (new BKSamplerVoice());
        hammerSynth->addVoice (new BKSamplerVoice());
        releaseResonanceSynth->addVoice (new BKSamplerVoice());
        pedalSynth->addVoice (new BKSamplerVoice());
    }

    /**
     * these synths play their stuff on noteOff rather than noteOn
     */
    hammerSynth->isKeyReleaseSynth (true);
    releaseResonanceSynth->isKeyReleaseSynth (true);
    pedalSynth->isPedalSynth (true);

    /*
     * generates mappings between audio-rate modulatable parameters and the audio channel the modulation comes in on
     *      from a modification preparation
     *      modulations like this come on an audio channel
     *      this is on a separate bus from the regular audio graph that carries audio between preparations
     */
    int mod = 0;
    for (auto [key, param] : state.params.modulatableParams)
    {
        juce::ValueTree modChan { IDs::MODULATABLE_PARAM };
        modChan.setProperty (IDs::parameter, juce::String (key), nullptr);
        modChan.setProperty (IDs::channel, mod, nullptr);
        v.appendChild (modChan, nullptr);
        mod++;
    }
    v.appendChild(state.params.transpose.paramDefault,nullptr);
    //add state change params here; this will add this to the set of params that are exposed to the state change mod system
        // not needed for audio-rate modulatable params
    parent.getStateBank().addParam (std::make_pair<std::string, bitklavier::ParameterChangeBuffer*> (v.getProperty (IDs::uuid).toString().toStdString() + "_" + "transpose", &(state.params.transpose.stateChanges)));
    parent.getStateBank().addParam (std::make_pair<std::string, bitklavier::ParameterChangeBuffer*> (v.getProperty (IDs::uuid).toString().toStdString() + "_" + "velocity_min_max", &(state.params.velocityMinMax.stateChanges)));
}

void DirectProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const auto spec = juce::dsp::ProcessSpec { sampleRate, (uint32_t) samplesPerBlock, (uint32_t) getMainBusNumInputChannels() };

    mainSynth->setCurrentPlaybackSampleRate (sampleRate);
    gain.prepare (spec);
    gain.setRampDurationSeconds (0.05);

    hammerSynth->setCurrentPlaybackSampleRate (sampleRate);
    releaseResonanceSynth->setCurrentPlaybackSampleRate (sampleRate);
    pedalSynth->setCurrentPlaybackSampleRate (sampleRate);
    setRateAndBufferSizeDetails (sampleRate, samplesPerBlock);
}

bool DirectProcessor::isBusesLayoutSupported (const juce::AudioProcessor::BusesLayout& layouts) const
{
    //    // only supports mono and stereo (for now)
    //    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
    //        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
    //        return false;
    //
    //    // input and output layout must be the same
    //    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
    //        return false;

    return true;
}

/*
 * grabs all the TransposeParams values and compiles them into a single array
 * the first slider is always represented, so we always have at least on value to return
 *
 * these operate at the synthesizer level, not the voice level, so need to be passed here
 * and not just looked at by individual voices in the synth
 *
 * this is all pretty inefficient, making copies of copies, but also very small arrays, so....
 */
juce::Array<float> DirectProcessor::getMidiNoteTranspositions()
{
    juce::Array<float> transps;
    auto paramVals = state.params.transpose.getFloatParams();
    int i =0;
    for (auto const& tp : *paramVals)
    {
        /**
         * todo for Davis: it's supposed to be the commented out line, but something need to get updated.
         */
//        if (tp->getCurrentValue() != 0. && state.params.transpose.numActiveSliders->getCurrentValue() > i)
        if (tp->getCurrentValue() != 0. && state.params.transpose.numActive > i)
            transps.addIfNotAlreadyThere (tp->getCurrentValue());
        i++;
    }

    // make sure that the first slider is always represented
    transps.addIfNotAlreadyThere (state.params.transpose.t0->getCurrentValue());

    return transps;
}

void DirectProcessor::setTuning (TuningProcessor* tun)
{
    tuning = tun;
    mainSynth->setTuning (&tuning->getState().params.tuningState);
    releaseResonanceSynth->setTuning (&tuning->getState().params.tuningState);
}

void DirectProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    // DBG("direct");
    state.getParameterListeners().callAudioThreadBroadcasters();

    // always top of the chain as an instrument source; doesn't take audio in
    buffer.clear();

    // need to call these every block; this is for state change discrete mods (as opposed to audio rate continuous mods)
    state.params.transpose.processStateChanges();
    state.params.velocityMinMax.processStateChanges();

    /**
     * todo:
     * used fixed length array since we know the max
     * and reduce numbers of copies of this array
     * fastest would be to have an array of pointers that points to the chowdsp version of these
     * but, these are small arrays, at the block, not a huge concern
     */
    // update transposition slider values
    juce::Array<float> updatedTransps = getMidiNoteTranspositions(); // from the Direct transposition slider
    bool useTuningForTranspositions = state.params.transpose.transpositionUsesTuning->get();

    if (mainSynth->hasSamples())
    {
        mainSynth->updateMidiNoteTranspositions (updatedTransps, useTuningForTranspositions);
        mainSynth->updateVelocityMinMax (
            state.params.velocityMinMax.velocityMinParam->getCurrentValue(),
            state.params.velocityMinMax.velocityMaxParam->getCurrentValue());

        mainSynth->renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());
    }

    if (hammerSynth->hasSamples())
    {
        hammerSynth->updateVelocityMinMax (
            state.params.velocityMinMax.velocityMinParam->getCurrentValue(),
            state.params.velocityMinMax.velocityMaxParam->getCurrentValue());
        // DBG("processblockhammersytnh
        hammerSynth->renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());
    }

    if (releaseResonanceSynth->hasSamples())
    {
        releaseResonanceSynth->updateMidiNoteTranspositions (updatedTransps, useTuningForTranspositions);
        releaseResonanceSynth->updateVelocityMinMax (
            state.params.velocityMinMax.velocityMinParam->getCurrentValue(),
            state.params.velocityMinMax.velocityMaxParam->getCurrentValue());

        releaseResonanceSynth->renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());
    }

    if (pedalSynth->hasSamples())
    {
        pedalSynth->renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());
    }

    // final output gain stage, from rightmost slider in DirectParametersView
    auto outputgainmult = bitklavier::utils::dbToMagnitude(state.params.outputGain->getCurrentValue());
    buffer.applyGain(outputgainmult);

    // level meter update stuff
    std::get<0> (state.params.outputLevels) = buffer.getRMSLevel (0, 0, buffer.getNumSamples());
    std::get<1> (state.params.outputLevels) = buffer.getRMSLevel (1, 0, buffer.getNumSamples());

    /**
     * Q: is all this thread-safe?
     *      - outputLevels above is std::atomic
     *      - the stuff below is not, but uses chowdsp params, so maybe that's ok?
     *          the answer is: yes! chowdsp handles the threading for us!
     */
    // get last synthesizer state and update things accordingly
    lastSynthState = mainSynth->getSynthesizerState();
    state.params.velocityMinMax.lastVelocityParam->setParameterValue (lastSynthState.lastVelocity);
    if (tuning != nullptr) tuning->getState().params.tuningState.updateLastFrequency(lastSynthState.lastPitch);

}
void DirectProcessor::processBlockBypassed(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages) {
    DBG("is bypassed");
}
