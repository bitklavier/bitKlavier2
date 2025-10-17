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

/*
 * ========================== ResonantString class ==========================
 */

ResonantString::ResonantString(
    ResonanceParams* inparams,
    std::array<PartialSpec, TotalNumberOfPartialKeysInUI>& inPartialStructure,
    std::array<NoteOnSpec, MaxMidiNotes>& inNoteOnSpecMap)
    : _rparams(inparams),
      _partialStructure(inPartialStructure),
      _noteOnSpecMap(inNoteOnSpecMap)
{
    heldKey = 0;
    active = false;

    for (int i = 0; i < MaxMidiNotes; ++i)
    {
        currentPlayingPartialsFromHeldKey.add(juce::Array<int>{});
        juce::Array<int>& internalArray = currentPlayingPartialsFromHeldKey.getReference(i);
        internalArray.ensureStorageAllocated(MaxMidiNotes);
    }
}

/**
 * when a key is pressed, call initialize string to setup the partials
 */
void ResonantString::addString (int midiNote)
{
    heldKey = midiNote;
    active = true;
    //DBG("added sympathetic string, channel = " + juce::String(channel));
}

/**
 * when another note is played, call ringString(), which will look for overlapping
 * partials with this held string and send the appropriate noteOn messages
 */
void ResonantString::ringString(int midiNote, int velocity, juce::MidiBuffer& outMidiMessages)
{
    if(!active) return;

    for (auto& heldPartialToCheck : _partialStructure)
    {
        // if not an active partial from UI, skip
        if(!std::get<0>(heldPartialToCheck)) continue;

        for (auto& struckPartialToCheck : _partialStructure)
        {
            // again, skip if partial is inactive
            if(!std::get<0>(struckPartialToCheck)) continue;

            float heldPartial       = static_cast<float>(heldKey) + std::get<1>(heldPartialToCheck);
            int   heldPartialKey    = std::round(heldPartial);

            float struckPartial       = static_cast<float>(midiNote) + std::get<1>(struckPartialToCheck);
            int   struckPartialKey    = std::round(struckPartial);

            if (heldPartialKey == struckPartialKey)
            {
                /*
                 * todo: decide how to use these
                 *          - we could have differences between the struck and held offsets impact how much sympathetic resonance is induced
                 *          - and we can have the gain be a multiplier of the velocity, or considered some other way
                 *          - also might be fine to ignore!
                 */
                float heldPartialOffset = heldPartial - static_cast<float>(heldPartialKey);
                float heldPartialGain   = std::get<2>(heldPartialToCheck);
                float struckPartialOffset = struckPartial - static_cast<float>(struckPartialKey);
                float struckPartialGain   = std::get<2>(struckPartialToCheck);

                /*
                 * add the held key offset for this partialKey to the transpositions
                 *  - we may have more than one partial attached to this key, with different offsets
                 *  - but we also don't want to add duplicates, so only add if not already there
                 */
                _noteOnSpecMap[heldPartialKey].transpositions.addIfNotAlreadyThere(heldPartialOffset);

                /*
                 * start time should be into the sample, to play just its tail
                 * - perhaps modulate with velocity, and/or set range by user as in old bK?
                 * - hard-wired for now, and maybe that's best anyhow...
                 *
                 * also, setting a fixed sustain time to avoid pileup of nearly-silent tails being added to the CPU load
                 * - setting the release time will be crucial for how it all sounds (and the CPU load), since that is in addition to the sustain time here
                 * - channel is important for disambiguating from other held strings
                 */
                _noteOnSpecMap[heldPartialKey].channel = channel;
                _noteOnSpecMap[heldPartialKey].startTime = 400;      // ms
                _noteOnSpecMap[heldPartialKey].sustainTime = 2000;   // ms
                _noteOnSpecMap[heldPartialKey].stopSameCurrentNote = false;
                //DBG("held key associated with partial " + juce::String(heldPartialKey) + " = " + juce::String(heldKeys[heldPartialKey_index]));

                /*
                 * make the midi message, play it on the channel for this particular resonant string
                 */
                auto newmsg = juce::MidiMessage::noteOn (channel, heldPartialKey, static_cast<float>(velocity/128.)); // velocity * partialGain?
                outMidiMessages.addEvent(newmsg, 0);

                /*
                 * add this partial to the array of partial associated with this heldKey that are currently playing
                 * - for noteOffs when the heldKey is released
                 */
                auto& cp = currentPlayingPartialsFromHeldKey.getReference(heldKey);
                cp.addIfNotAlreadyThere(heldPartialKey);

                //DBG("found overlap of partials, play resonance here: " + juce::String(struckPartial) + " at key " + juce::String(heldPartialKey) + " with offset " + juce::String(heldPartialOffset));

            }
        }
    }
}

/**
 * when keys are released, we call removeString to see if this heldKey should been released,
 * and if so, send the appropriate noteOff messages.
 * this should also start a timer of some sort that will set doneRinging to true after
 * the noteOff release time has passed, and release the MIDI channel
 */
void ResonantString::removeString (int midiNote, juce::MidiBuffer& outMidiMessages)
{
    /*
     * need to figure out how to delay setting this until release time is done
     */
    active = false;
    //DBG("removed string " + juce::String(midiNote) + " on channel " + juce::String(channel));

    // read through currentPlayingPartialsFromHeldKey and send noteOffs for each
    for (auto pnotes : currentPlayingPartialsFromHeldKey[midiNote])
    {
        _noteOnSpecMap[pnotes].keyState = true; // override the UI controlled envelope and use envParams specified here
        _noteOnSpecMap[pnotes].envParams = {50.0f * .001, 10.0f * .001, 1.0f, 50.0f * .001, 0.0f, 0.0f, 0.0f};
        _noteOnSpecMap[pnotes].channel = channel;

        //DBG("sending noteOffs for partial " + juce::String(pnotes) + " of held note " + juce::String(midiNote) + " on channel " + juce::String(channel));
        auto newmsg = juce::MidiMessage::noteOff (channel, pnotes, 0.0f);
        outMidiMessages.addEvent(newmsg, 0);
    }
    currentPlayingPartialsFromHeldKey.set(midiNote, {});
}


/*
 * ========================== ResonanceProcessor ==========================
 */
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

    for (size_t i = 0; i < resonantStringsArray.size(); ++i)
    {
        // Use std::make_unique to safely allocate and store the pointer
        resonantStringsArray[i] = std::make_unique<ResonantString>(&state.params, partialStructure, noteOnSpecMap);

        // Set channel
        // Note: If you want channel to match index, use the casted size_t 'i'
        resonantStringsArray[i]->channel = static_cast<int>(i + 1);
    }
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
    // start with a clean slate of noteOn specifications; assuming normal noteOns without anything special
    for (auto& spec : noteOnSpecMap)
    {
        spec.clear();
    }

    /*
     * process incoming MIDI messages, including the target messages
     */
    for (auto mi : inMidiMessages)
    {
        auto message = mi.getMessage();

        if(message.isNoteOn())
            keyPressed(message.getNoteNumber(), message.getVelocity(), message.getChannel(), outMidiMessages);
        else if(message.isNoteOff())
            keyReleased(message.getNoteNumber(), outMidiMessages);
    }

    updatePartialStructure();
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
            float pFundOffset   = static_cast<float>(i) - static_cast<float>(pFundamental) + pOffset * .01;
            partialStructure[i] = { true, pFundOffset, pGain };
            //DBG("added to partialStructure " + juce::String(i) + " " + juce::String(pFundOffset) + " " + juce::String(pGain));
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

/**
 * ringSympStrings is called when a noteOn message is received.
 * It looks for overlaps between the partials of the struck note (from the noteOn message) and partials of all the held notes.
 * Overlaps will cause sympathetic resonance in the held note strings,
 * modeled very simply by playing the tails of the samples at the appropriate pitch as set by the partialStructure.
 *
 * @param noteNumber
 * @param velocity
 * @param outMidiMessages
 */
void ResonanceProcessor::ringSympStrings(int noteNumber, float velocity, juce::MidiBuffer& outMidiMessages)
{
    for (auto& _string : resonantStringsArray)
    {
        _string->ringString(noteNumber, velocity, outMidiMessages);
    }
}

void ResonanceProcessor::addSympStrings(int noteNumber)
{
    for (auto& _string : resonantStringsArray)
    {
        if(!_string->active)
        {
            _string->addString (noteNumber);
            return;
        }
    }
    DBG("no available string found!");
}

void ResonanceProcessor::keyPressed(int noteNumber, int velocity, int channel, juce::MidiBuffer& outMidiMessages)
{
    handleMidiTargetMessages(channel);

    //printPartialStructure();

    if (doRing)
    {
        // resonate the currently available strings and their overlapping partials
        ringSympStrings(noteNumber, velocity, outMidiMessages);
    }
    if (doAdd)
    {
        // then, add this new string and its partials to the currently available sympathetic strings
        addSympStrings(noteNumber);
    }
}

void ResonanceProcessor::keyReleased(int noteNumber, juce::MidiBuffer& outMidiMessages)
{
    if (doAdd)
    {
        for (auto& _string : resonantStringsArray)
        {
            if(_string->heldKey == noteNumber)
                _string->removeString (noteNumber, outMidiMessages);
        }
    }
    if (doRing)
    {

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


