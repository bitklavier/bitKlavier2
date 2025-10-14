/*
==============================================================================

  Resonance.cpp
  Created: 12 May 2021 12:41:26pm
  Author:  Dan Trueman and Theodore R Trevisan
  Rewritten: Dan Trueman, October 2025

  Models the sympathetic resonance within the piano, with options for
  static resonances akin to the Hardanger fiddle

==============================================================================
*/

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

    for (int i = 0; i < MaxMidiNotes; ++i)
    {
        noteOnSpecMap[i] = NoteOnSpec{};
    }

    resetPartialStructure();
}

void ResonanceProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const auto spec = juce::dsp::ProcessSpec { sampleRate, (uint32_t) samplesPerBlock, (uint32_t) getMainBusNumInputChannels() };
    resonanceSynth->setCurrentPlaybackSampleRate (sampleRate);
}

bool ResonanceProcessor::isBusesLayoutSupported (const juce::AudioProcessor::BusesLayout& layouts) const
{
    return true;
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
void ResonanceProcessor::ProcessMIDIBlock(juce::MidiBuffer& inMidiMessages, juce::MidiBuffer& outMidiMessages, int numSamples)
{
    /*
     * process incoming MIDI messages, including the target messages
     */
    for (auto mi : inMidiMessages)
    {
        auto message = mi.getMessage();

        if(message.isNoteOn())
            keyPressed(message.getNoteNumber(), message.getVelocity(), message.getChannel());
        else if(message.isNoteOff())
            keyReleased(message.getNoteNumber());
    }

    updatePartialStructure();
}

/**
 * addPartial will insert this partial at the beginning of the array and shove
 * all the existing partials to the right by one, removing the oldest partial
 *
 * @param heldKey
 * @param partialKey
 * @param gain
 * @param offset
 */
void ResonanceProcessor::addPartial(int heldKey, int partialKey, float gain, float offset)
{
    insert_and_shift(heldKeys, heldKey);
    insert_and_shift(partialKeys, partialKey);
    insert_and_shift(gains, gain);
    insert_and_shift(offsets, offset);
    insert_and_shift(startTimes, 0);
}

/**
 * removePartialsForHeldKey will do what it says, removing all the partials from
 * the parallel arrays heldKeys, partialKeys, gains, and offsets, and compacting to the left
 * the heldKeys array will/should hold 0s for all inactive keys, so as soon as an
 * iterator encounters a 0, it can stop moving through the array looking for partials
 *
 * the four arrays should remain synchronized by index
 *
 * @param heldKey
 */
void ResonanceProcessor::removePartialsForHeldKey(int heldKey)
{
    synchronized_remove_and_compact(heldKeys, heldKey, partialKeys, gains, offsets, startTimes);
}

/**
 * clear the partialStructure array and set it to default vals
 */
void ResonanceProcessor::resetPartialStructure()
{
    for (size_t i = 0; i < partialStructure.size(); ++i)
    {
        /*
         * by default, each partial is inactive, with no offset from fundamental and gain multipler of 1.0
         */
        partialStructure[i] = {false, 0.f, 1.f};
    }
}

/**
 * set the partial structure to the first 8 partials of the overtone series
 * todo: move this to ResonanceParams?
 *          - perhaps give it an argument to set the number of partials?
 *          - and make this callable from the UI
 */
void ResonanceProcessor::setDefaultPartialStructure()
{
//    //fundamentalKey = 0;
//    rFundamentalKey = 0;
//
//    for(int i = 0; i < 128; i++)
//    {
//        //offsetsKeys.set(i, 0.);
//        //gainsKeys.set(i, 1.);
//        rOffsetsKeys.setArrayValue(i, 0.0f);
//        rGainsKeys.setArrayValue(i, 1.0f);
//    }
//
//    addResonanceKey( 0, 1.0, 0.);
//    addResonanceKey(12, 0.8, 0);
//    addResonanceKey(19, 0.7, 2);
//    addResonanceKey(24, 0.8, 0);
//    addResonanceKey(28, 0.6, -13.7);
//    addResonanceKey(31, 0.7, 2);
//    addResonanceKey(34, 0.5, -31.175);
//    addResonanceKey(36, 0.8, 0);
}

/**
 * set the partialStructure array vals based on parameter settings
 */
void ResonanceProcessor::updatePartialStructure()
{
    /*
     * partialStructure
     * - active (bool; false => default)
     * - offset from fundamental (fractional MIDI note val; 0. => default)
     * - gains (floats; 1.0 => default)
     */
    resetPartialStructure();

    int pFundamental = find_first_set_bit(state.params.fundamentalKeymap.keyStates);
    if (pFundamental >= state.params.fundamentalKeymap.keyStates.size())
    {
        //DBG("ResonanceProcessor::updatePartialStructure() -- no fundamental found!");
        return;
    }

    for (size_t i = 0; i < state.params.closestKeymap.keyStates.size(); ++i)
    {
        if (state.params.closestKeymap.keyStates.test(i))
        {
            float pOffset       = state.params.offsetsKeyboardState.absoluteTuningOffset[i];
            float pGain         = state.params.gainsKeyboardState.absoluteTuningOffset[i];
            partialStructure[i] = { true, i - pFundamental + pOffset, pGain };
        }
    }
}

void ResonanceProcessor::printPartialStructure()
{
    DBG("Partial Structure:");
    DBG("     fundamental = " + juce::String(find_first_set_bit(state.params.fundamentalKeymap.keyStates)));

    int partialNum = 1;
    for (auto& pstruct : partialStructure)
    {
        if (std::get<0>(pstruct))
        {
            DBG ("     partial " + juce::String (partialNum++) + " offset from fundamental = " + juce::String (std::get<1> (pstruct)) +  " and gain = " + juce::String (std::get<2> (pstruct)));
        }
    }
}

void ResonanceProcessor::ringSympStrings(int noteNumber, float velocity)
{
    int pkey_index = 0;
    for (auto pkey : partialKeys)
    {
        for (auto& pstruct : partialStructure)
        {
            if (std::get<0>(pstruct))
            {
                float partial       = static_cast<float>(noteNumber) + std::get<1>(pstruct);
                int   partialKey    = std::round(partial);
                float partialOffset = partial - static_cast<float>(partialKey);
                float partialGain   = std::get<2>(pstruct);

                if (partialKey == pkey)
                {
                    // need a way to check if this partialKey is actually active? or is pKey == 0 unless it is set?
                    DBG("found overlap of partials, play resonance here");
                }
            }
        }
        pkey_index++;
    }
}

void ResonanceProcessor::addSympStrings(int noteNumber)
{
//    for (int i = 0; i < getPartialStructure().size(); i++)
//    {
//        // heldKey      = noteNumber
//        // partialKey   = key that this partial is nearest, as assigned by partialStructure
//        int partialKey = noteNumber + getPartialStructure().getUnchecked(i)[0];
//        if (partialKey > 127 || partialKey < 0) continue;
//
//        // make a newPartial object, with gain and offset vals
//        //DBG("Resonance: adding partial " + String(partialKey) + " to " + String(noteNumber));
//        sympStrings.getReference(noteNumber).add(new SympPartial(noteNumber, partialKey, getPartialStructure()[i][1], getPartialStructure()[i][2]));
//    }

    for (auto& pstruct : partialStructure)
    {
        if (std::get<0>(pstruct))
        {
            float partial       = static_cast<float>(noteNumber) + std::get<1>(pstruct);
            int   partialKey    = std::round(partial);
            float partialOffset = partial - static_cast<float>(partialKey);
            float partialGain   = std::get<2>(pstruct);

            addPartial(noteNumber, partialKey, partialOffset, partialGain);
        }
    }

}

void ResonanceProcessor::keyPressed(int noteNumber, int velocity, int channel)
{
    handleMidiTargetMessages(channel);

    printPartialStructure();

    if (doRing)
    {
        // resonate the currently available strings and their overlapping partials
        ringSympStrings(noteNumber, velocity);
    }
    if (doAdd)
    {
        // then, add this new string and its partials to the currently available sympathetic strings
        // 3rd arg ignore repeated notes = true, so don't add this string if it's already there
        addSympStrings(noteNumber);
    }
}

void ResonanceProcessor::keyReleased(int noteNumber)
{
    if (doAdd)
    {
        removePartialsForHeldKey(noteNumber);
    }
    if (doRing)
    {
        // noteOffs here
    }
}

void ResonanceProcessor::handleMidiTargetMessages(int channel)
{
//    bool doRing = true;
//    bool doAdd = true;
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
    ProcessMIDIBlock(midiMessages, outMidi, numSamples);

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
     * then the more complex params
     */

    std::array<float, 128> tempAbsoluteOffsets;
    copyAtomicArrayToFloatArray(paramHolder.offsetsKeyboardState.absoluteTuningOffset, tempAbsoluteOffsets);
    Serializer::template addChildElement<128> (ser, "resonanceOffsets", tempAbsoluteOffsets, arrayToStringWithIndex<128>);

    copyAtomicArrayToFloatArray(paramHolder.gainsKeyboardState.absoluteTuningOffset, tempAbsoluteOffsets);
    Serializer::template addChildElement<128> (ser, "resonanceGains", tempAbsoluteOffsets, arrayToStringWithIndex<128>);

    Serializer::template addChildElement<128> (ser, "fundamental", paramHolder.fundamentalKeymap.keyStates, getOnKeyString);
    Serializer::template addChildElement<128> (ser, "partials", paramHolder.closestKeymap.keyStates, getOnKeyString);

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
    auto myStr = deserial->getStringAttribute ("resonanceOffsets");
    parseIndexValueStringToAtomicArray(myStr.toStdString(), paramHolder.offsetsKeyboardState.absoluteTuningOffset);

    myStr = deserial->getStringAttribute ("resonanceGains");
    parseIndexValueStringToAtomicArray(myStr.toStdString(), paramHolder.gainsKeyboardState.absoluteTuningOffset);

    paramHolder.fundamentalKeymap.keyStates = bitklavier::utils::stringToBitset (deserial->getStringAttribute ("fundamental"));
    paramHolder.closestKeymap.keyStates = bitklavier::utils::stringToBitset (deserial->getStringAttribute ("partials"));
}


