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

    /*
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
    auto mod_params = v.getChildWithName(IDs::MODULATABLE_PARAMS);
    if (!mod_params.isValid()) {
        int mod = 0;
        mod_params = v.getOrCreateChildWithName(IDs::MODULATABLE_PARAMS,nullptr);
        for (auto param: state.params.modulatableParams)
        {
            juce::ValueTree modChan { IDs::MODULATABLE_PARAM };
            juce::String name = std::visit([](auto* p) -> juce::String
        {
            return p->paramID; // Works if all types have getParamID()
        }, param);
            const auto& a  = std::visit([](auto* p) -> juce::NormalisableRange<float>
        {
            return p->getNormalisableRange(); // Works if all types have getParamID()
        }, param);
            modChan.setProperty (IDs::parameter, name, nullptr);
            modChan.setProperty (IDs::channel, mod, nullptr);
            modChan.setProperty(IDs::start, a.start,nullptr);
            modChan.setProperty(IDs::end, a.end,nullptr);
            modChan.setProperty(IDs::skew, a.skew,nullptr);

            mod_params.appendChild (modChan, nullptr);
            mod++;
        }
    }
    bufferDebugger = new BufferDebugger();
    state.params.transpose.stateChanges.defaultState = v.getOrCreateChildWithName(IDs::PARAM_DEFAULT,nullptr);
    state.params.velocityMinMax.stateChanges.defaultState = v.getOrCreateChildWithName(IDs::PARAM_DEFAULT,nullptr);
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
    int i = 0;
    for (auto const& tp : *paramVals)
    {
//        if (tp->getCurrentValue() != 0. && state.params.transpose.sliderVals_size->getCurrentValue() > i)
        if (state.params.transpose.numActiveSliders->getCurrentValue() > i)
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
    /*
     * this updates all the AudioThread callbacks we might have in place
     * for instance, in TuningParametersView.cpp, we have lots of lambda callbacks from the UI
     *  they are all on the MessageThread, but if we wanted to have them synced to the block
     *      we would put them on the AudioThread and they would be heard here
     *  if we put them on the AudioThread, it would be important to have minimal actions in those
     *      callbacks, no UI stuff, etc, just updating params needed in the audio block here
     *      if we want to do other stuff for the same callback, we should have a second MessageThread callback
     */
    //DBG (v.getParent().getParent().getProperty (IDs::name).toString() + "direct");
    state.getParameterListeners().callAudioThreadBroadcasters();
    auto mod_Bus = getBus(true,1);
    auto index = mod_Bus->getChannelIndexInProcessBlockBuffer(0);
    int i = index;
    // melatonin::printSparkline(buffer);
    for(auto param: state.params.modulatableParams){
        // auto a = v.getChildWithName(IDs::MODULATABLE_PARAMS).getChild(i);
        // DBG(a.getProperty(IDs::parameter).toString());
        bufferDebugger->capture(v.getChildWithName(IDs::MODULATABLE_PARAMS).getChild(i).getProperty(IDs::parameter).toString(), buffer.getReadPointer(i++), buffer.getNumSamples(), -1.f, 1.f);
    }
   const auto&  modBus = getBusBuffer(buffer, true, 1);  // true = input, bus index 0 = mod

    int numInputChannels = modBus.getNumChannels();
    for (int channel = 0; channel < numInputChannels; ++channel) {
       const float* in = modBus.getReadPointer(channel);
        std::visit([in](auto* p)->void
    {
        p->applyMonophonicModulation(*in);
    },  state.params.modulatableParams[channel]);

    }
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
        mainSynth->setBypassed (false);
        mainSynth->updateMidiNoteTranspositions (updatedTransps, useTuningForTranspositions);
        mainSynth->updateVelocityMinMax (
            state.params.velocityMinMax.velocityMinParam->getCurrentValue(),
            state.params.velocityMinMax.velocityMaxParam->getCurrentValue());

        mainSynth->renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());
    }

    if (hammerSynth->hasSamples())
    {
        hammerSynth->setBypassed (false);

        hammerSynth->updateVelocityMinMax (
            state.params.velocityMinMax.velocityMinParam->getCurrentValue(),
            state.params.velocityMinMax.velocityMaxParam->getCurrentValue());
        // DBG("processblockhammersytnh
        hammerSynth->renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());
    }

    if (releaseResonanceSynth->hasSamples())
    {
        releaseResonanceSynth->setBypassed (false);
        releaseResonanceSynth->updateMidiNoteTranspositions (updatedTransps, useTuningForTranspositions);
        releaseResonanceSynth->updateVelocityMinMax (
            state.params.velocityMinMax.velocityMinParam->getCurrentValue(),
            state.params.velocityMinMax.velocityMaxParam->getCurrentValue());

        releaseResonanceSynth->renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());
    }

    if (pedalSynth->hasSamples())
    {
        pedalSynth->setBypassed (false);
        pedalSynth->renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());
    }

    // final output gain stage, from rightmost slider in DirectParametersView
    auto outputgainmult = bitklavier::utils::dbToMagnitude (state.params.outputGain->getCurrentValue());
    buffer.applyGain (outputgainmult);

    // level meter update stuff
    std::get<0> (state.params.outputLevels) = buffer.getRMSLevel (0, 0, buffer.getNumSamples());
    std::get<1> (state.params.outputLevels) = buffer.getRMSLevel (1, 0, buffer.getNumSamples());

    /*
     * Q: is all this thread-safe?
     *      - outputLevels above is std::atomic
     *      - the stuff below is not, but uses chowdsp params, so maybe that's ok?
     *          the answer is: yes! chowdsp handles the threading for us!
     */
    // get last synthesizer state and update things accordingly
    lastSynthState = mainSynth->getSynthesizerState();
    state.params.velocityMinMax.lastVelocityParam->setParameterValue (lastSynthState.lastVelocity);
    if (tuning != nullptr)
        tuning->getState().params.tuningState.updateLastFrequency (lastSynthState.lastPitch);


    int sendBufferIndex = getChannelIndexInProcessBlockBuffer (false, 2, 0);

    /**
     * todo add gain as last arg to these:
     */
    buffer.copyFrom(sendBufferIndex, 0, buffer.getReadPointer(0), buffer.getNumSamples());
    buffer.copyFrom(sendBufferIndex+1, 0, buffer.getReadPointer(1), buffer.getNumSamples());

}

/**
 * DirectProcessor::processBlockBypassed is called when this Direct prep is NOT in the active,
 *      visible Piano, but is in the overall graph for the complete Gallery.
 *
 * Usually, it should do nothing, but it may need to do some closing actions after a PianoSwitch
 *  for instance, if the player is holding some notes down with the left hand, and then executes a PianoSwitch
 *      with the right hand, and then releases the left hand notes after the switch, we need to sustain
 *      those notes through the switch, and then turn them off afterwards. We also should play the
 *      hammer/resonance samples.
 *  there will be more complicated things to handle here in Nostalgic, Synchronic, etc...
 *
 * @param buffer
 * @param midiMessages
 */
void DirectProcessor::processBlockBypassed (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    //DBG (v.getParent().getParent().getProperty (IDs::name).toString() + "direct bypassed");
    buffer.clear();
    state.getParameterListeners().callAudioThreadBroadcasters();

    if (mainSynth->hasSamples())
    {
        mainSynth->setBypassed (true);
        mainSynth->renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());
    }

    if (hammerSynth->hasSamples())
    {
        hammerSynth->setBypassed (true);
        hammerSynth->renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());
    }

    if (releaseResonanceSynth->hasSamples())
    {
        releaseResonanceSynth->setBypassed (true);
        releaseResonanceSynth->renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());
    }

    if (pedalSynth->hasSamples())
    {
        pedalSynth->setBypassed (true);
        pedalSynth->renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());
    }

    auto outputgainmult = bitklavier::utils::dbToMagnitude (state.params.outputGain->getCurrentValue());
    buffer.applyGain (outputgainmult);

    /**
     * todo: include send buffer copy stuff here?
     */
}
