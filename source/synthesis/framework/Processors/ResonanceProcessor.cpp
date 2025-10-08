//
// Created by Joshua Warner on 6/27/24.
//

#include "ResonanceProcessor.h"
#include "Synthesiser/Sample.h"
#include "common.h"
#include "synth_base.h"

ResonanceProcessor::ResonanceProcessor(SynthBase& parent, const juce::ValueTree& vt) :
        PluginBase (parent, vt, nullptr, resonanceBusLayout()),
        resonanceSynth (new BKSynthesiser (state.params.env, state.params.noteOnGain))
{
    for (int i = 0; i < 300; i++)
    {
        resonanceSynth->addVoice (new BKSamplerVoice());
    }

}

void ResonanceProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const auto spec = juce::dsp::ProcessSpec { sampleRate, (uint32_t) samplesPerBlock, (uint32_t) getMainBusNumInputChannels() };
    resonanceSynth->setCurrentPlaybackSampleRate (sampleRate);
}

void ResonanceProcessor::processContinuousModulations(juce::AudioBuffer<float>& buffer)
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

void ResonanceProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    // this is a synth, so we want an empty audio buffer to start
    buffer.clear();

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
    processContinuousModulations(buffer);

    // process any mod changes to the multisliders
    state.params.processStateChanges();

    /*
     * ProcessMIDIBlock takes all the input MIDI messages and writes to outMIDI buffer
     *  to send to BKSynth
     */
    int numSamples = buffer.getNumSamples();
    juce::MidiBuffer outMidi;
    //ProcessMIDIBlock(midiMessages, outMidi, numSamples);

    /*
     * Then the Audio Stuff
     */

    /*
     * then the synthesizer process blocks
     */
    if (resonanceSynth->hasSamples())
    {
        resonanceSynth->setBypassed (false);
        resonanceSynth->setNoteOnSpecMap(noteOnSpecMap);
        resonanceSynth->renderNextBlock (buffer, outMidi, 0, buffer.getNumSamples());
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

void ResonanceProcessor::processBlockBypassed (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    /**
     * todo: handle noteOffs, otherwise nothing?
     */
}

/**
 * Serializers, for saving/loading complex params like the multisliders
 */
template <typename Serializer>
typename Serializer::SerializedType ResonanceParams::serialize (const ResonanceParams& paramHolder)
{
    /*
     * first, call the default serializer, which gets all the simple params
     */
    auto ser = chowdsp::ParamHolder::serialize<Serializer> (paramHolder);

    /*
     * then serialize the more complex params
     */
//    serializeMultiSliderParam<Serializer> (ser, paramHolder.transpositions, "transpositions_");
//    serializeMultiSliderParam<Serializer> (ser, paramHolder.accents, "accents_");
//    serializeMultiSliderParam<Serializer> (ser, paramHolder.sustainLengthMultipliers, "sustain_length_multipliers");
//    serializeMultiSliderParam<Serializer> (ser, paramHolder.beatLengthMultipliers, "beat_length_multipliers");
//
//    serializeArrayADSRParam<Serializer>(ser, paramHolder.envelopeSequence.envStates, "envelope_sequence");

    return ser;
}

template <typename Serializer>
void ResonanceParams::deserialize (typename Serializer::DeserializedType deserial, ResonanceParams& paramHolder)
{
    /*
     * call the default deserializer first, for the simple params
     */
    chowdsp::ParamHolder::deserialize<Serializer> (deserial, paramHolder);

    /*
     * then the more complex params
     */
//    deserializeMultiSliderParam<Serializer> (deserial, paramHolder.transpositions, "transpositions_");
//    deserializeMultiSliderParam<Serializer> (deserial, paramHolder.accents, "accents_");
//    deserializeMultiSliderParam<Serializer> (deserial, paramHolder.sustainLengthMultipliers, "sustain_length_multipliers");
//    deserializeMultiSliderParam<Serializer> (deserial, paramHolder.beatLengthMultipliers, "beat_length_multipliers");
//
//    deserializeArrayADSRParam<Serializer> (deserial, paramHolder.envelopeSequence.envStates, "envelope_sequence");
}


