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

/*
 * Tuning approaches:
 *
 * can integrate semitonewidth at this stage, which is only active for static tunings
 * in fact, could differentiate between static and dynamic tunings (spring being the only one that needs to update at the block)
 *      and for static tunings, handle everything here before noteOn, and only do the block updates for dynamic tunings?
 * spring tuning would still need to handle transpositions, and actually we *could* assign an initial tuning here for them as well
 *      so perhaps ALL tunings could be setup here, and we only worry about updating them within Sample for dynamic (spring) tuning?
 *      in which case semitonewidth *could* be active, setting the initial "transposition" for every note,
 *          which would then be at work in BKsynth when the spring tuning is updated?
 * i think we want t0 to always "useTuning" no? otherwise, we'd need to have that checked in the UI for tuning to work at all!
 *      or always "useTuning" when there is only one value in there? or always "useTuning" for a transposition value of 0?
 *
 * soooo.... handle all the tuning here, and then including a "dynamic" flag as true for spring tuning, and only those would update in Sample.h in the block
 *      and always "useTuning" for t0?
 *      or always "useTuning" if there is only one value in transpositions?
 *      or always "useTuning" for a transposition value of 0?
 *      sooo.... in the old bK, the transpositions values are all initially relative to the attached Tuning!
 *          for example, if there is only one value in the transposition slider, -2, and tuning is Partial-C:
 *              with useTuning = false, playing a C will result in an ET Bb
 *              with useTuning = true,  playing a C will result in a 7th-partial of C
 *              with useTuning = false, playing a Bb will result in an Ab whole-step down (-2) from a Partial-C Bb! (basically have transposed to Partial-Bb?)
 *                  so our reference pitch is the Partial-C Bb, as set in Tuning
 *              with useTuning = true,  playing a Bb will result in an Ab tuned to the 13th-partial of C
 *                  so our pitch gets further filtered to fit in the Partial-C from Tuning; Ab being the 13th in that system
 *          so i think this means that with transp = 0, the behavior is the same regardless of whether useTuning is true or false, which is what we want
 *              and it doesn't matter where it is in the list of transpositions
 *
 * could we remove the useTuning dependency from tuning->getTargetFrequency? and handle that all right here?
 *
 * so, looking at the bK code:
 *      first, we call getOffset on every transposition, and determine whether we useTransp at this stage:
 *
     * if (prep->dTranspUsesTuning.value) // use the Tuning setting
     *      offset = t + tuner->getOffset(round(t) + noteNumber, false); // t is the transposition value
     * else  // or set it absolutely, tuning only the note that is played (default, and original behavior)
     *      offset = t + tuner->getOffset(noteNumber, false);
 *
 * then, inside getOffset we do the semitoneSize, circular, absolute, fundamentaloffset adjustments
 */
juce::Array<float> DirectProcessor::getMidiNoteTranspositions()
{

    if (tuning != nullptr) {
        // use these to adjust transpositions, if useTuning is true
        tuning->getState().params.semitoneWidthParams.reffundamental->getCurrentValueAsText();
        tuning->getState().params.semitoneWidthParams.octave->getCurrentValueAsText();
        tuning->getState().params.semitoneWidthParams.semitoneWidthSliderParam->getCurrentValue();
        state.params.transpose.transpositionUsesTuning->get();
        // make helper functions for the above, including one that cooks the reff and octave down to a midinotenumber for the fund
    }

    /*
     * in tuner->getOffset(), we have
     * newTransp =.01 * (midiNoteNumber - tuning->prep->getNToneRoot()) * (tuning->prep->getNToneSemitoneWidth() - 100); // this is an offset to midiNoteNumber
     *
    // and then this gets filtered through current circularTuning, and further adjusted by absoluteTuning, for nearest key
    /* int midiNoteNumberTemp = round(midiNoteNumber + newTransp);
     * newTransp += (currentTuning[(midiNoteNumberTemp - tuning->prep->getFundamental()) % currentTuning.size()]
     *           + tuning->prep->getAbsoluteOffsets().getUnchecked(midiNoteNumber)
     *           + tuning->prep->getFundamentalOffset());
     *
     * this is preceded by:
     * if (prep->dTranspUsesTuning.value)
     *      // use the Tuning setting
     *      offset = t + tuner->getOffset(round(t) + noteNumber, false);
     * else
     *      // or set it absolutely, tuning only the note that is played (default, and original behavior)
     *      offset = t + tuner->getOffset(noteNumber, false);
     *
     * so the aim here should be:
     *      assemble a complete collection of all the notes for the synth to play, given a particular midiNoteNumber coming from the input keyboard
     *          including the tuning offsets for each note, so each value can be a midiFloat value
     *          these will be transpositions from the original int midiNoteNumber input
     *      these values will boil down everything needed from semitoneWidth, useTuning, circular, absolute, fundamentaloffset to a single offset from the input midiNoteNumber
     *      pass that array on to the synth, which then only has to tune the notes exactly as specified
     *          it will also choose the closest sample to the desired sounding pitch for Sample to use
     *      so the midiNoteNumber that the synth gets from the midi messages becomes just the reference for this array of notes to actually play
     *          which it uses to turn on/off all those notes
     *
     * figure out how to deal with spring tuning separately.
     */

    juce::Array<float> transps;
//    std::vector<float> testarr;
//    if (std::find(testarr.begin(), testarr.end(), target) != testarr.end()) // finds whether target is in testarr

    auto paramVals = state.params.transpose.getFloatParams();
    for (auto const& tp : *paramVals)
    {
        if (tp->getCurrentValue() != 0.)
            transps.addIfNotAlreadyThere (tp->getCurrentValue());
    }

    // make sure that the first slider is always represented
    /*
     * and shouldn't this one always "useTuning"? only if it is 0.
     */
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

    // get last synthesizer state and update things accordingly
    lastSynthState = mainSynth->getSynthesizerState();
    state.params.velocityMinMax.lastVelocityParam->setParameterValue (lastSynthState.lastVelocity);
}
