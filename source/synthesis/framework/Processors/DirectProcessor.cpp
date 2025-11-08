//
// Created by Davis Polito on 5/2/24.
//

#include "DirectProcessor.h"
#include "Synthesiser/Sample.h"
#include "synth_base.h"

DirectProcessor::DirectProcessor (SynthBase& parent, const juce::ValueTree& vt) : PluginBase (parent, vt, nullptr, directBusLayout()),
                                                                                  mainSynth (new BKSynthesiser (state.params.env, state.params.gainParam)),
                                                                                  hammerSynth (new BKSynthesiser (state.params.env, state.params.hammerParam)),
                                                                                  releaseResonanceSynth (new BKSynthesiser (state.params.env, state.params.releaseResonanceParam)),
                                                                                  pedalSynth (new BKSynthesiser (state.params.env, state.params.pedalParam))
{
    // for testing
    bufferDebugger = new BufferDebugger();

    for (int i = 0; i < 300; i++)
    {
        mainSynth->addVoice (new BKSamplerVoice());
        hammerSynth->addVoice (new BKSamplerVoice());
        releaseResonanceSynth->addVoice (new BKSamplerVoice());
        pedalSynth->addVoice (new BKSamplerVoice());
    }

    for (int i = 0; i < MaxMidiNotes; ++i)
    {
        noteOnSpecMap[i] = NoteOnSpec{};
    }

    /*
     * these synths play their stuff on noteOff rather than noteOn
     */
    hammerSynth->isKeyReleaseSynth (true);
    releaseResonanceSynth->isKeyReleaseSynth (true);
    pedalSynth->isPedalSynth (true);

    state.params.transpose.stateChanges.defaultState = v.getOrCreateChildWithName (IDs::PARAM_DEFAULT, nullptr);
    state.params.transpose.transpositionUsesTuning->stateChanges.defaultState = v.getOrCreateChildWithName (IDs::PARAM_DEFAULT, nullptr);

    midiNoteTranspositions.ensureStorageAllocated (50);

    //add state change params here; this will add this to the set of params that are exposed to the state change mod system
    // not needed for audio-rate modulatable params
    parent.getStateBank().addParam (std::make_pair<std::string,
        bitklavier::ParameterChangeBuffer*> (v.getProperty (IDs::uuid).toString().toStdString() + "_" + "transpose",
        &(state.params.transpose.stateChanges)));
    parent.getStateBank().addParam (std::make_pair<std::string,
        bitklavier::ParameterChangeBuffer*> (v.getProperty (IDs::uuid).toString().toStdString() + "_" + "UseTuning",
        &(state.params.transpose.transpositionUsesTuning->stateChanges)));
    v.addListener (this);
    parent.getValueTree().addListener (this);
    loadSamples();
}

void DirectProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const auto spec = juce::dsp::ProcessSpec { sampleRate, (uint32_t) samplesPerBlock, (uint32_t) getMainBusNumInputChannels() };

    mainSynth->setCurrentPlaybackSampleRate (sampleRate);
    //    gain.prepare (spec);
    //    gain.setRampDurationSeconds (0.05);

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
 * like above, but assigns transpositions to a particular noteOnSpec
 */
void DirectProcessor::updateMidiNoteTranspositions(int noteOnNumber)
{
    noteOnSpecMap[noteOnNumber].transpositions.clear();
    auto paramVals = state.params.transpose.getFloatParams();
    int i = 0;
    for (auto const& tp : *paramVals)
    {
        if (state.params.transpose.numActiveSliders->getCurrentValue() > i)
            noteOnSpecMap[noteOnNumber].transpositions.addIfNotAlreadyThere (tp->getCurrentValue());
        i++;
    }

    // make sure that the first slider is always represented
    noteOnSpecMap[noteOnNumber].transpositions.addIfNotAlreadyThere (state.params.transpose.t0->getCurrentValue());
}

/*
 * update them all to the same transpositions.
 * - since Direct uses the same transpositions, we just have them all set to the same values
 * - this is different in other preps like Resonance, where individual noteOn msgs will have their own transpositions
 */
void DirectProcessor::updateAllMidiNoteTranspositions()
{
    for (int i=0; i<MaxMidiNotes; i++)
    {
        updateMidiNoteTranspositions(i);
    }
}

void DirectProcessor::setTuning (TuningProcessor* tun)
{
    tuning = tun;
    tuning->addListener(this);
    mainSynth->setTuning (&tuning->getState().params.tuningState);
    releaseResonanceSynth->setTuning (&tuning->getState().params.tuningState);
}

void DirectProcessor::tuningStateInvalidated() {
    DBG("DirectProcessor::tuningStateInvalidated()");
    tuning = nullptr;
    mainSynth->setTuning(nullptr);
    releaseResonanceSynth->setTuning(nullptr);

}

void DirectProcessor::processContinuousModulations (juce::AudioBuffer<float>& buffer)
{
    // this for debugging
    //    auto mod_Bus = getBus(true,1);
    //    auto index = mod_Bus->getChannelIndexInProcessBlockBuffer(0);
    //    int i = index;
    //    // melatonin::printSparkline(buffer);
    //    for(auto param: state.params.modulatableParams){
    //        // auto a = v.getChildWithName(IDs::MODULATABLE_PARAMS).getChild(i);
    //        // DBG(a.getProperty(IDs::parameter).toString());
    //        bufferDebugger->capture(v.getChildWithName(IDs::MODULATABLE_PARAMS).getChild(i).getProperty(IDs::parameter).toString(), buffer.getReadPointer(i++), buffer.getNumSamples(), -1.f, 1.f);
    //    }

    const auto& modBus = getBusBuffer (buffer, true, 1); // true = input, bus index 0 = mod

    int numInputChannels = modBus.getNumChannels();
    for (int channel = 0; channel < numInputChannels; ++channel)
    {
        const float* in = modBus.getReadPointer (channel);
        std::visit ([in] (auto* p) -> void {
            p->applyMonophonicModulation (*in);
        },
            state.params.modulatableParams[channel]);
    }
}

void DirectProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    //DBG (v.getParent().getParent().getProperty (IDs::name).toString() + "direct");

    /*
     * this updates all the AudioThread callbacks we might have in place
     * for instance, in TuningParametersView.cpp, we have lots of lambda callbacks from the UI
     *  they are all on the MessageThread, but if we wanted to have them synced to the block
     *      we would put them on the AudioThread and they would be heard here
     *  if we put them on the AudioThread, it would be important to have minimal actions in those
     *      callbacks, no UI stuff, etc, just updating params needed in the audio block here
     *      if we want to do other stuff for the same callback, we should have a second MessageThread callback
     *
     *  I'm not sure we have any of these for Direct, but no harm in calling it, and for reference going forward
     */
    state.getParameterListeners().callAudioThreadBroadcasters();

    /*
     * modulation stuff
     */

    // first, the continuous modulations (simple knobs/sliders...)
    processContinuousModulations (buffer);

    // then, the state-change modulations, for more complex params
    state.params.transpose.processStateChanges();
    state.params.transpose.transpositionUsesTuning->processStateChanges();

    // since this is an instrument source; doesn't take audio in, other than mods handled above
    buffer.clear();

    // update transposition slider values
    //updateMidiNoteTranspositions();
    updateAllMidiNoteTranspositions();
    /**
     * todo: need to include useTuningForTranspositions in noteOnSpecMap now...
     *      - in place of synth->updateMidiNoteTranspositions (midiNoteTranspositions, useTuningForTranspositions);
     */
    bool useTuningForTranspositions = state.params.transpose.transpositionUsesTuning->get();

    if (mainSynth->hasSamples())
    {
        mainSynth->setBypassed (false);
        mainSynth->setNoteOnSpecMap(noteOnSpecMap);
        mainSynth->renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());
    }

    if (hammerSynth->hasSamples())
    {
        hammerSynth->setBypassed (false);
        hammerSynth->renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());
    }

    if (releaseResonanceSynth->hasSamples())
    {
        releaseResonanceSynth->setBypassed (false);
        releaseResonanceSynth->setNoteOnSpecMap(noteOnSpecMap);
        releaseResonanceSynth->renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());
    }

    if (pedalSynth->hasSamples())
    {
        pedalSynth->setBypassed (false);
        pedalSynth->renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());
    }

    // send goes out the right outlets: prefader send
    /*
     * all the audio channels are in buffer, so we first identify which channels carry the sendBuffer
     * we look at directBusLayout() in DirectProcessor.h, and see that 'Send' is the 3rd output, so its busIndex is 2
     * "getChannelIndexInProcessBlockBuffer" will give us the proper channel in buffer to copy the buffer to for the send
     * busIndex is different than channel # because each bus might have a number of channels
     */

    // handle the send
    int sendBufferIndex = getChannelIndexInProcessBlockBuffer (false, 2, 0);
    auto sendgainmult = bitklavier::utils::dbToMagnitude (*state.params.outputSendParam);
    buffer.copyFrom (sendBufferIndex, 0, buffer.getReadPointer (0), buffer.getNumSamples(), sendgainmult);
    buffer.copyFrom (sendBufferIndex + 1, 0, buffer.getReadPointer (1), buffer.getNumSamples(), sendgainmult);

    // send level meter update
    std::get<0> (state.params.sendLevels) = buffer.getRMSLevel (sendBufferIndex, 0, buffer.getNumSamples());
    std::get<1> (state.params.sendLevels) = buffer.getRMSLevel (sendBufferIndex + 1, 0, buffer.getNumSamples());

    // final output gain stage, from rightmost slider in DirectParametersView
    auto outputgainmult = bitklavier::utils::dbToMagnitude (state.params.outputGain->getCurrentValue());
    buffer.applyGain (0, 0, buffer.getNumSamples(), outputgainmult);
    buffer.applyGain (1, 0, buffer.getNumSamples(), outputgainmult);

    // level meter update stuff
    std::get<0> (state.params.outputLevels) = buffer.getRMSLevel (0, 0, buffer.getNumSamples());
    std::get<1> (state.params.outputLevels) = buffer.getRMSLevel (1, 0, buffer.getNumSamples());

    /**
     * todo: create an output send level meter?
     */

    /*
     * Q: is all this thread-safe?
     *      - outputLevels above is std::atomic
     *      - the stuff below is not, but uses chowdsp params, so maybe that's ok?
     *          the answer is: yes! chowdsp handles the threading for us!
     */
    // get last synthesizer state and update things accordingly
    lastSynthState = mainSynth->getSynthesizerState();
    if (tuning != nullptr)
        tuning->getState().params.tuningState.updateLastFrequency (lastSynthState.lastPitch);
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


