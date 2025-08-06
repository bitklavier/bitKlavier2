//
// Created by Joshua Warner on 6/27/24.
//

#include "SynchronicProcessor.h"
#include "Synthesiser/Sample.h"
#include "common.h"
#include "synth_base.h"

/**
 * todo: change constructor ar for backwardSynth to backwardEnvParams, once we figure out how to manage multiple envParams
 */
SynchronicProcessor::SynchronicProcessor(SynthBase& parent, const juce::ValueTree& vt) : PluginBase (parent, vt, nullptr, synchronicBusLayout()),
                                                                                          forwardsSynth (new BKSynthesiser (state.params.env, state.params.outputGain)),
                                                                                          backwardsSynth (new BKSynthesiser (state.params.env, state.params.outputGain))
{
    // for testing
    bufferDebugger = new BufferDebugger();

    for (int i = 0; i < 300; i++)
    {
        forwardsSynth->addVoice (new BKSamplerVoice());
        backwardsSynth->addVoice (new BKSamplerVoice());
    }

    /*
     * backwardsSynth is for playing backwards samples
     * forwardsSynth is for playing forwards samples
     */
    backwardsSynth->setPlaybackDirection(Direction::backward);

    /*
     * modulations and state changes
     */
    setupModulationMappings();

    /**
     * todo: add these when we know what they are
     */
//    state.params.transpose.stateChanges.defaultState = v.getOrCreateChildWithName(IDs::PARAM_DEFAULT,nullptr);
//    state.params.velocityMinMax.stateChanges.defaultState = v.getOrCreateChildWithName(IDs::PARAM_DEFAULT,nullptr);
//
//    //add state change params here; this will add this to the set of params that are exposed to the state change mod system
//    // not needed for audio-rate modulatable params
//    parent.getStateBank().addParam (std::make_pair<std::string,
//        bitklavier::ParameterChangeBuffer*> (v.getProperty (IDs::uuid).toString().toStdString() + "_" + "transpose",
//        &(state.params.transpose.stateChanges)));
//    parent.getStateBank().addParam (std::make_pair<std::string,
//        bitklavier::ParameterChangeBuffer*> (v.getProperty (IDs::uuid).toString().toStdString() + "_" + "velocity_min_max",
//        &(state.params.velocityMinMax.stateChanges)));

    state.params.clusterMinMaxParams.stateChanges.defaultState = v.getOrCreateChildWithName(IDs::PARAM_DEFAULT,nullptr);
    parent.getStateBank().addParam (std::make_pair<std::string,
        bitklavier::ParameterChangeBuffer*> (v.getProperty (IDs::uuid).toString().toStdString() + "_" + "cluster_min_max",
        &(state.params.clusterMinMaxParams.stateChanges)));
}

/**
 * generates mappings between audio-rate modulatable parameters and the audio channel the modulation comes in on
 *      from a modification preparation
 *      modulations like this come on an audio channel
 *      this is on a separate bus from the regular audio graph that carries audio between preparations
 *
 * todo: perhaps this should be an inherited function for all preparation processors?
 */
void SynchronicProcessor::setupModulationMappings()
{
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
}

void SynchronicProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const auto spec = juce::dsp::ProcessSpec { sampleRate, (uint32_t) samplesPerBlock, (uint32_t) getMainBusNumInputChannels() };

    forwardsSynth->setCurrentPlaybackSampleRate (sampleRate);
    backwardsSynth->setCurrentPlaybackSampleRate (sampleRate);

}

bool SynchronicProcessor::isBusesLayoutSupported (const juce::AudioProcessor::BusesLayout& layouts) const
{
    return true;
}

void SynchronicProcessor::setTuning (TuningProcessor* tun)
{
    tuning = tun;
    forwardsSynth->setTuning (&tuning->getState().params.tuningState);
    backwardsSynth->setTuning (&tuning->getState().params.tuningState);
}

/**
 * todo: should this inherited from a PreparationProcessor superclass?
 * @param buffer
 */
void SynchronicProcessor::processContinuousModulations(juce::AudioBuffer<float>& buffer)
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

    const auto&  modBus = getBusBuffer(buffer, true, 1);  // true = input, bus index 0 = mod

    int numInputChannels = modBus.getNumChannels();
    for (int channel = 0; channel < numInputChannels; ++channel) {
        const float* in = modBus.getReadPointer(channel);
        std::visit([in](auto* p)->void
            {
                p->applyMonophonicModulation(*in);
            },  state.params.modulatableParams[channel]);
    }
}

void SynchronicProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
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

    // process continuous modulations (gain level sliders)
//    processContinuousModulations(buffer);

    // process any mod changes to the multisliders
    state.params.processStateChanges();

    /*
     * MIDI Targeting Stuff First
     */
//    handleMidiTargetMessages(midiMessages);

    /*
     * do the MIDI stuff here
     */

    /*
     * Then the Audio Stuff
     */
    int numSamples = buffer.getNumSamples();

    // use these to display buffer info to bufferDebugger
    bufferDebugger->capture("L", buffer.getReadPointer(0), numSamples, -1.f, 1.f);
    bufferDebugger->capture("R", buffer.getReadPointer(1), numSamples, -1.f, 1.f);

    /*
     * then the synthesizer process blocks
     */
    if (forwardsSynth->hasSamples())
    {
        forwardsSynth->setBypassed (false);
//        forwardsSynth->updateMidiNoteTranspositions (updatedTransps, useTuningForTranspositions);
        forwardsSynth->renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());
    }

    if (backwardsSynth->hasSamples())
    {
        backwardsSynth->setBypassed (false);
        //        forwardsSynth->updateMidiNoteTranspositions (updatedTransps, useTuningForTranspositions);
        backwardsSynth->renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());
    }

    // handle the send
    int sendBufferIndex = getChannelIndexInProcessBlockBuffer (false, 2, 0);
    auto sendgainmult = bitklavier::utils::dbToMagnitude (state.params.outputSendGain->getCurrentValue());
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

void SynchronicProcessor::processBlockBypassed (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
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
typename Serializer::SerializedType SynchronicParams::serialize (const SynchronicParams& paramHolder)
{
    /*
     * first, call the default serializer, which gets all the simple params
     */
    auto ser = chowdsp::ParamHolder::serialize<Serializer> (paramHolder);

    /*
     * then serialize the more complex params
     */
    serializeMultiSliderParam<Serializer> (ser, paramHolder.transpositions, "transpositions_");
    serializeMultiSliderParam<Serializer> (ser, paramHolder.accents, "accents_");
    serializeMultiSliderParam<Serializer> (ser, paramHolder.sustainLengthMultipliers, "sustain_length_multipliers");
    serializeMultiSliderParam<Serializer> (ser, paramHolder.beatLengthMultipliers, "beat_length_multipliers");

    return ser;
}

template <typename Serializer>
void SynchronicParams::deserialize (typename Serializer::DeserializedType deserial, SynchronicParams& paramHolder)
{
    /*
     * call the default deserializer first, for the simple params
     */
    chowdsp::ParamHolder::deserialize<Serializer> (deserial, paramHolder);

    /*
     * then the more complex params
     */
    deserializeMultiSliderParam<Serializer> (deserial, paramHolder.transpositions, "transpositions_");
    deserializeMultiSliderParam<Serializer> (deserial, paramHolder.accents, "accents_");
    deserializeMultiSliderParam<Serializer> (deserial, paramHolder.sustainLengthMultipliers, "sustain_length_multipliers");
    deserializeMultiSliderParam<Serializer> (deserial, paramHolder.beatLengthMultipliers, "beat_length_multipliers");
}