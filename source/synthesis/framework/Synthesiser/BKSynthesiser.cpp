//
// Created by Davis Polito on 8/12/24.
//

#include "BKSynthesiser.h"
//==============================================================================
BKSynthesiser::BKSynthesiser(EnvParams &params, chowdsp::GainDBParameter& gain) : adsrParams (params), synthGain(gain)
{
    for (int i = 0; i < juce::numElementsInArray (lastPitchWheelValues); ++i)
        lastPitchWheelValues[i] = 0x2000;

    // init hash of currently playing notes
    for (int i = 0; i<=128; i++)
        playingVoicesByNote.insert(0, {  });

    activeNotes.reset();
}

BKSynthesiser::~BKSynthesiser()
{
    voices.clear();
}

//==============================================================================
BKSamplerVoice* BKSynthesiser::getVoice (const int index) const
{
    const juce::ScopedLock sl (lock);
    return voices [index];
}

void BKSynthesiser::clearVoices()
{
    const juce::ScopedLock sl (lock);
    voices.clear();
}

BKSamplerVoice* BKSynthesiser::addVoice (BKSamplerVoice* const newVoice)
{
    BKSamplerVoice* voice;

    {
        const juce::ScopedLock sl (lock);
        newVoice->setCurrentPlaybackSampleRate (sampleRate);
        voice = voices.add (newVoice);
    }

    {
        const juce::ScopedLock sl (stealLock);
        usableVoicesToStealArray.ensureStorageAllocated (voices.size() + 1);
    }

    return voice;
}

void BKSynthesiser::removeVoice (const int index)
{
    const juce::ScopedLock sl (lock);
    voices.remove (index);
}

void BKSynthesiser::addSoundSet(juce::ReferenceCountedArray<BKSamplerSound<juce::AudioFormatReader>>* s)
{
    const juce::ScopedLock sl (lock);
    sounds = s;
}
//
//void BKSynthesiser::clearSounds()
//{
//    const juce::ScopedLock sl (lock);
//    sounds->clear();
//}

//BKSamplerSound<juce::AudioFormatReader>* BKSynthesiser::addSound (const BKSamplerSound<juce::AudioFormatReader>::Ptr& newSound)
//{
//    const juce::ScopedLock sl (lock);
//    return sounds->add ((newSound));
//}

//void BKSynthesiser::removeSound (const int index)
//{
//    const juce::ScopedLock sl (lock);
//    sounds->remove (index);
//}

void BKSynthesiser::setNoteStealingEnabled (const bool shouldSteal)
{
    shouldStealNotes = shouldSteal;
}

void BKSynthesiser::setMinimumRenderingSubdivisionSize (int numSamples, bool shouldBeStrict) noexcept
{
    jassert (numSamples > 0); // it wouldn't make much sense for this to be less than 1
    minimumSubBlockSize = numSamples;
    subBlockSubdivisionIsStrict = shouldBeStrict;
}

//==============================================================================
void BKSynthesiser::setCurrentPlaybackSampleRate (const double newRate)
{
    if (! juce::approximatelyEqual (sampleRate, newRate))
    {
        const juce::ScopedLock sl (lock);
        allNotesOff (0, false);
        sampleRate = newRate;

        for (auto* voice : voices)
            voice->setCurrentPlaybackSampleRate (newRate);
    }
}

template <typename floatType>
void BKSynthesiser::processNextBlock (juce::AudioBuffer<floatType>& outputAudio,
                                    const juce::MidiBuffer& midiData,
                                    int startSample,
                                    int numSamples)
{
    // must set the sample rate before using this!
    jassert (! juce::exactlyEqual (sampleRate, 0.0));
    const int targetChannels = outputAudio.getNumChannels();

    auto midiIterator = midiData.findNextSamplePosition (startSample);

    bool firstEvent = true;

    const juce::ScopedLock sl (lock);

    /*
     * if we are in a bypassed state, and have already handled all vestigial midinotes,
     * just render any remaining active voices and skip the rest
     */
    if(bypassed && activeNotes == 0)
    {
        if (targetChannels > 0 && someVoicesActive)
            renderVoices (outputAudio, startSample, numSamples);
        return;
    }

    for (; numSamples > 0; ++midiIterator)
    {
        if (midiIterator == midiData.cend())
        {
            if (targetChannels > 0)
                renderVoices (outputAudio, startSample, numSamples);

            return;
        }

        const auto metadata = *midiIterator;
        const int samplesToNextMidiMessage = metadata.samplePosition - startSample;

        if (samplesToNextMidiMessage >= numSamples)
        {
            if (targetChannels > 0)
                renderVoices (outputAudio, startSample, numSamples);

            handleMidiEvent (metadata.getMessage());
            break;
        }

        if (samplesToNextMidiMessage < ((firstEvent && ! subBlockSubdivisionIsStrict) ? 1 : minimumSubBlockSize))
        {
            handleMidiEvent (metadata.getMessage());
            continue;
        }

        firstEvent = false;

        if (targetChannels > 0)
            renderVoices (outputAudio, startSample, samplesToNextMidiMessage);

        handleMidiEvent (metadata.getMessage());
        startSample += samplesToNextMidiMessage;
        numSamples  -= samplesToNextMidiMessage;
    }

    std::for_each (midiIterator,
                   midiData.cend(),
                   [&] (const juce::MidiMessageMetadata& meta) { handleMidiEvent (meta.getMessage()); });
}

// explicit template instantiation
template void BKSynthesiser::processNextBlock<float>  (juce::AudioBuffer<float>&,  const juce::MidiBuffer&, int, int);

void BKSynthesiser::renderNextBlock (juce::AudioBuffer<float>& outputAudio, const juce::MidiBuffer& inputMidi,
                                   int startSample, int numSamples)
{
    processNextBlock (outputAudio, inputMidi, startSample, numSamples);
}

void BKSynthesiser::renderVoices (juce::AudioBuffer<float>& buffer, int startSample, int numSamples)
{
    for (auto* voice : voices)
    {
        voice->renderNextBlock (buffer, startSample, numSamples);
        if (voice->isVoiceActive() ) someVoicesActive = true;
    }
}

void BKSynthesiser::handleMidiEvent (const juce::MidiMessage& m)
{
    const int channel = m.getChannel();

    /*
     * regarding keyReleaseSynths:
     * in most cases, this operates as expected
     *
     * for a synth that is in keyRelease mode, however, the function of noteOn and noteOff releases are reversed
     *
     * note that this is separate from the "invert noteOn/Off" modality in KeyMap, which could effectively
     * reverse this already reversed behaviour!
     */

    if(!bypassed)
    {
        if (m.isNoteOn())
        {
            DBG ("BKSynthesizer Note On " + juce::String (m.getNoteNumber()) + " " + juce::String (m.getVelocity()));

            if (pedalSynth)
                return;

            if (!keyReleaseSynth)
            {
                noteOn (channel, m.getNoteNumber(), m.getVelocity());
                activeNotes.set(m.getNoteNumber());
            }
            else
            {
                noteOff (channel, m.getNoteNumber(), m.getVelocity(), true);
                activeNotes.set(m.getNoteNumber());
            }
        }
        else if (m.isNoteOff())
        {
            if (pedalSynth)
                return;

            if (!keyReleaseSynth)
            {
                noteOff (channel, m.getNoteNumber(), m.getVelocity(), true);
                activeNotes.reset(m.getNoteNumber());
            }
            else
            {
                if(activeNotes.test(m.getNoteNumber())) {
                    noteOn (channel, m.getNoteNumber(), m.getVelocity());
                    activeNotes.reset(m.getNoteNumber());
                }
            }
        }
        else if (m.isAllNotesOff() || m.isAllSoundOff())
        {
            allNotesOff (channel, true);
        }
        else if (m.isPitchWheel())
        {
            const int wheelPos = m.getPitchWheelValue();
            lastPitchWheelValues[channel - 1] = wheelPos;
            handlePitchWheel (channel, wheelPos);
        }
        else if (m.isAftertouch())
        {
            handleAftertouch (channel, m.getNoteNumber(), m.getAfterTouchValue());
        }
        else if (m.isChannelPressure())
        {
            handleChannelPressure (channel, m.getChannelPressureValue());
        }
        else if (m.isController())
        {
            handleController (channel, m.getControllerNumber(), m.getControllerValue());
        }
        else if (m.isProgramChange())
        {
            handleProgramChange (channel, m.getProgramChangeNumber());
        }
    } else // bypassed!
    {
        /*
         * if this synth is bypassed, we are aiming to clean up its activity, let it ring down, and then be silent
         * basically:
         *      - noteOns should be ignored so we don't activate new sounds
         *      - noteOffs should do their thing, but only for currently sounding noteOns in this synth
         *          we don't want hammers playing for every noteOff, for instance, only for already sounding noteOns
         *
         * so we need to keep track of currently active notes and only activate noteOffs for those: activeNotes array of bools
         */

        if (m.isNoteOn())
        {
            DBG ("BKSynthesizer Note On (bypassed) " + juce::String (m.getNoteNumber()) + " " + juce::String (m.getVelocity()));

            if (pedalSynth)
                return;

            if(keyReleaseSynth && activeNotes.test(m.getNoteNumber()))
            {
                noteOff (channel, m.getNoteNumber(), m.getVelocity(), true);
                activeNotes.reset(m.getNoteNumber());
            }
        }
        else if (m.isNoteOff())
        {
            DBG ("BKSynthesizer Note Off (bypassed) " + juce::String (m.getNoteNumber()) + " " + juce::String (m.getVelocity()));

            if (pedalSynth)
                return;

            if (!keyReleaseSynth)
            {
                if(activeNotes.test(m.getNoteNumber())) {
                    noteOff (channel, m.getNoteNumber(), m.getVelocity(), true);
                    activeNotes.reset(m.getNoteNumber());
                }
            }
            else // for keyReleaseSynths (hammers, resonance)
            {
                if(activeNotes.test(m.getNoteNumber()))
                {
                    noteOn (channel, m.getNoteNumber(), m.getVelocity());
                    activeNotes.reset(m.getNoteNumber());
                }
            }
        }
    }
}

//==============================================================================
void BKSynthesiser::noteOn (const int midiChannel,
                          const int midiNoteNumber,
                          const float velocity)
{
    const juce::ScopedLock sl (lock);

    /**
     * mute instruments with gain turned all the way down
     */
    if (synthGain <= -60.) return;

    /**
     * store this velocity for the UI to use
     */
    lastSynthState.lastVelocity = velocity;

    /**
     * check first to see if velocity is within velocity min/max range, and return if not
     */
    if (!checkVelocityRange(velocity)) return;

    /**
     * moved this out of the loop below because it was messing up voice handling with multiple transpositions
     * however, this move might break something else in the future, we'll have to see..
     */
    // If hitting a note that's still ringing, stop it first (it could be
    // still playing because of the sustain or sostenuto pedal).
    for (auto* voice : voices)
        if (voice->getCurrentlyPlayingNote() == midiNoteNumber && voice->isPlayingChannel (midiChannel))
            stopVoice (voice, 1.0f, true);

    /**
     * a midiNoteNumber, reflective of what key the player plays, might result in multiple notes being played
     *      as set by currentTransposition sliders in Direct/Nostalgic/Synchronic.
     *      iterate through each currentTransposition here, but they are all tracked by the same original midiNoteNumber
     */
    for (auto transp : midiNoteTranspositions)
    {
        for (auto* sound : *sounds)
        {
            int closestKey;
            if(tuning != nullptr) {
                closestKey = tuning->getClosestKey(midiNoteNumber, transp, tuneTranspositions);
                lastSynthState.lastPitch = tuning->getTargetFrequency(midiNoteNumber, transp, tuneTranspositions);
            }
            else closestKey = std::round(midiNoteNumber + transp);
            //if (sound->appliesToNote (std::round(midiNoteNumber + transp)) && sound->appliesToChannel (midiChannel) && sound->appliesToVelocity (velocity))
            if (sound->appliesToNote ( closestKey) && sound->appliesToChannel (midiChannel) && sound->appliesToVelocity (velocity))
            {
                startVoice (findFreeVoice (sound, midiChannel, midiNoteNumber, shouldStealNotes),
                    sound,
                    midiChannel,
                    midiNoteNumber,
                    velocity,
                    transp);

                break;
            }
        }
    }
}

void BKSynthesiser::startVoice (BKSamplerVoice* const voice,
                                BKSamplerSound<juce::AudioFormatReader>* const sound,
                                const int midiChannel,
                                const int midiNoteNumber,
                                const float velocity,
                                const float transposition)
{
    /**
     * save this voice, since it might be one of several associated with this midiNoteNumber
     * and we will need to be able to stop it on noteOff(midiNoteNumber)
     */
    auto tempv = playingVoicesByNote.getUnchecked(midiNoteNumber);
    tempv.insert(0, voice);
    playingVoicesByNote.set(midiNoteNumber, tempv);

    //if (voice != nullptr && sound != nullptr)
    {
        if (voice->currentlyPlayingSound != nullptr)
            voice->stopNote (0.0f, false);

        voice->setTuning(tuning);

        voice->copyAmpEnv ({ adsrParams.attackParam->getCurrentValue() * 0.001f,
            adsrParams.decayParam->getCurrentValue() * 0.001f,
            adsrParams.sustainParam->getCurrentValue(),
            adsrParams.releaseParam->getCurrentValue() * 0.001f,
            static_cast<float> (adsrParams.attackPowerParam->getCurrentValue() * -1.),
            static_cast<float> (adsrParams.decayPowerParam->getCurrentValue() * -1.),
            static_cast<float> (adsrParams.releasePowerParam->getCurrentValue() * -1.) });

        voice->setGain(juce::Decibels::decibelsToGain (synthGain.getCurrentValue()));
        voice->currentlyPlayingNote = midiNoteNumber;
        voice->currentPlayingMidiChannel = midiChannel;
        voice->noteOnTime = ++lastNoteOnCounter;
        voice->currentlyPlayingSound = sound;
        voice->setKeyDown (true);
        voice->setSostenutoPedalDown (false);
        voice->setSustainPedalDown (sustainPedalsDown[midiChannel]);

        voice->startNote (
            midiNoteNumber,
            velocity,
            transposition,
            tuneTranspositions, // bool: whether to tune using Tuning, or just literally by transposition value given previously
            sound,
            lastPitchWheelValues [midiChannel - 1]
            );
    }
}

void BKSynthesiser::stopVoice (BKSamplerVoice* voice, float velocity, const bool allowTailOff)
{
    jassert (voice != nullptr);

    voice->stopNote (velocity, allowTailOff);

    // the subclass MUST call clearCurrentNote() if it's not tailing off! RTFM for stopNote()!
    jassert (allowTailOff || (voice->getCurrentlyPlayingNote() < 0 && voice->getCurrentlyPlayingSound() == nullptr));
}

void BKSynthesiser::noteOff (const int midiChannel,
                           const int midiNoteNumber,
                           const float velocity,
                           const bool allowTailOff)
{
    const juce::ScopedLock sl (lock);

    /**
     * go through all voices that were triggered by this particular midiNoteNumber and turn them off
     * by storing voices as they are played, we can avoid the problem where the transpositions change
     * while the notes are still held and turn them off here
     */
    for (auto* voice : playingVoicesByNote[midiNoteNumber])
    {
        voice->setKeyDown (false);

        if (!(voice->isSustainPedalDown() || voice->isSostenutoPedalDown()))
        {
            stopVoice (voice, velocity, allowTailOff);
        }
    }
    playingVoicesByNote.set(midiNoteNumber, {}); // clear and clearQuick didn't actually do the trick for this!
}

void BKSynthesiser::allNotesOff (const int midiChannel, const bool allowTailOff)
{
    const juce::ScopedLock sl (lock);

    for (auto* voice : voices)
        if (midiChannel <= 0 || voice->isPlayingChannel (midiChannel))
            voice->stopNote (1.0f, allowTailOff);

    sustainPedalsDown.clear();
    activeNotes.reset();
}

void BKSynthesiser::handlePitchWheel (const int midiChannel, const int wheelValue)
{
    const juce::ScopedLock sl (lock);

    for (auto* voice : voices)
        if (midiChannel <= 0 || voice->isPlayingChannel (midiChannel))
            voice->pitchWheelMoved (wheelValue);
}

void BKSynthesiser::handleController (const int midiChannel,
                                    const int controllerNumber,
                                    const int controllerValue)
{
    switch (controllerNumber)
    {
        case 0x40:  handleSustainPedal   (midiChannel, controllerValue >= 64); break;
        case 0x42:  handleSostenutoPedal (midiChannel, controllerValue >= 64); break;
        case 0x43:  handleSoftPedal      (midiChannel, controllerValue >= 64); break;
        default:    break;
    }

    const juce::ScopedLock sl (lock);

    for (auto* voice : voices)
        if (midiChannel <= 0 || voice->isPlayingChannel (midiChannel))
            voice->controllerMoved (controllerNumber, controllerValue);
}

void BKSynthesiser::handleAftertouch (int midiChannel, int midiNoteNumber, int aftertouchValue)
{
    const juce::ScopedLock sl (lock);

    for (auto* voice : voices)
        if (voice->getCurrentlyPlayingNote() == midiNoteNumber
            && (midiChannel <= 0 || voice->isPlayingChannel (midiChannel)))
            voice->aftertouchChanged (aftertouchValue);
}

void BKSynthesiser::handleChannelPressure (int midiChannel, int channelPressureValue)
{
    const juce::ScopedLock sl (lock);

    for (auto* voice : voices)
        if (midiChannel <= 0 || voice->isPlayingChannel (midiChannel))
            voice->channelPressureChanged (channelPressureValue);
}

void BKSynthesiser::handleSustainPedal (int midiChannel, bool isDown)
{
    DBG("BKSynthesiser::handleSustainPedal");
    jassert (midiChannel > 0 && midiChannel <= 16);
    const juce::ScopedLock sl (lock);

    if (isDown)
    {
        if (pedalSynth) // only do this if this is a sustainPedal synth
        {
            DBG("pressing sustain pedal");
            if (!sustainPedalAlreadyDown){
                // play pedal down sample here
                sustainPedalAlreadyDown = true;
                noteOn(midiChannel, 65, 64); // 64 for pedal down. velocity?
            }
        }

        else { // regular synth
            sustainPedalsDown.setBit (midiChannel);

            for (auto* voice : voices)
                if (voice->isPlayingChannel (midiChannel) && voice->isKeyDown())
                    voice->setSustainPedalDown (true);
        }
    }
    else
    {
        if (pedalSynth)
        {
            if(sustainPedalAlreadyDown)
            {
                DBG("releasing sustain pedal");
                // play pedal up sample here
                noteOff(midiChannel, 65, 64, true); // turn off sustain pedal down sample, which can be long
                noteOn(midiChannel, 66, 64); // 65 for pedal up
            }
            sustainPedalAlreadyDown = false;
        }

        else {
            for (auto* voice : voices)
            {
                if (voice->isPlayingChannel (midiChannel))
                {
                    voice->setSustainPedalDown (false);

                    if (! (voice->isKeyDown() || voice->isSostenutoPedalDown()))
                        stopVoice (voice, 1.0f, true);
                }
            }
            sustainPedalsDown.clearBit (midiChannel);
        }
    }
}

void BKSynthesiser::handleSostenutoPedal (int midiChannel, bool isDown)
{
    jassert (midiChannel > 0 && midiChannel <= 16);
    const juce::ScopedLock sl (lock);

    for (auto* voice : voices)
    {
        if (voice->isPlayingChannel (midiChannel))
        {
            if (isDown)
                voice->setSostenutoPedalDown (true);
            else if (voice->isSostenutoPedalDown())
                stopVoice (voice, 1.0f, true);
        }
    }
}

void BKSynthesiser::handleSoftPedal ([[maybe_unused]] int midiChannel, bool /*isDown*/)
{
    jassert (midiChannel > 0 && midiChannel <= 16);
}

void BKSynthesiser::handleProgramChange ([[maybe_unused]] int midiChannel,
                                       [[maybe_unused]] int programNumber)
{
    jassert (midiChannel > 0 && midiChannel <= 16);
}

//==============================================================================
BKSamplerVoice* BKSynthesiser::findFreeVoice (BKSamplerSound<juce::AudioFormatReader>* soundToPlay,
                                              int midiChannel, int midiNoteNumber,
                                              const bool stealIfNoneAvailable) const
{
    const juce::ScopedLock sl (lock);

    for (auto* voice : voices)
        if ((! voice->isVoiceActive()) && voice->canPlaySound (soundToPlay))
            return voice;

    if (stealIfNoneAvailable)
        return findVoiceToSteal (soundToPlay, midiChannel, midiNoteNumber);

    return nullptr;
}

BKSamplerVoice* BKSynthesiser::findVoiceToSteal (BKSamplerSound<juce::AudioFormatReader>* soundToPlay,
                                                 int /*midiChannel*/, int midiNoteNumber) const
{
    // This voice-stealing algorithm applies the following heuristics:
    // - Re-use the oldest notes first
    // - Protect the lowest & topmost notes, even if sustained, but not if they've been released.

    // apparently you are trying to render audio without having any voices...
    jassert (! voices.isEmpty());

    // These are the voices we want to protect (ie: only steal if unavoidable)
    BKSamplerVoice* low = nullptr; // Lowest sounding note, might be sustained, but NOT in release phase
    BKSamplerVoice* top = nullptr; // Highest sounding note, might be sustained, but NOT in release phase

    // All major OSes use double-locking so this will be lock- and wait-free as long as the lock is not
    // contended. This is always the case if you do not call findVoiceToSteal on multiple threads at
    // the same time.
    const juce::ScopedLock sl (stealLock);

    // this is a list of voices we can steal, sorted by how long they've been running
    usableVoicesToStealArray.clear();

    for (auto* voice : voices)
    {
        if (voice->canPlaySound (soundToPlay))
        {
            jassert (voice->isVoiceActive()); // We wouldn't be here otherwise

            usableVoicesToStealArray.add (voice);

            // NB: Using a functor rather than a lambda here due to scare-stories about
            // compilers generating code containing heap allocations..
            struct Sorter
            {
                bool operator() (const BKSamplerVoice* a, const BKSamplerVoice* b) const noexcept { return a->wasStartedBefore (*b); }
            };

            std::sort (usableVoicesToStealArray.begin(), usableVoicesToStealArray.end(), Sorter());

            if (! voice->isPlayingButReleased()) // Don't protect released notes
            {
                auto note = voice->getCurrentlyPlayingNote();

                if (low == nullptr || note < low->getCurrentlyPlayingNote())
                    low = voice;

                if (top == nullptr || note > top->getCurrentlyPlayingNote())
                    top = voice;
            }
        }
    }

    // Eliminate pathological cases (ie: only 1 note playing): we always give precedence to the lowest note(s)
    if (top == low)
        top = nullptr;

    // The oldest note that's playing with the target pitch is ideal..
    for (auto* voice : usableVoicesToStealArray)
        if (voice->getCurrentlyPlayingNote() == midiNoteNumber)
            return voice;

    // Oldest voice that has been released (no finger on it and not held by sustain pedal)
    for (auto* voice : usableVoicesToStealArray)
        if (voice != low && voice != top && voice->isPlayingButReleased())
            return voice;

    // Oldest voice that doesn't have a finger on it:
    for (auto* voice : usableVoicesToStealArray)
        if (voice != low && voice != top && ! voice->isKeyDown())
            return voice;

    // Oldest voice that isn't protected
    for (auto* voice : usableVoicesToStealArray)
        if (voice != low && voice != top)
            return voice;

    // We've only got "protected" voices now: lowest note takes priority
    jassert (low != nullptr);

    // Duophonic mainSynth: give priority to the bass note:
    if (top != nullptr)
        return top;

    return low;
}
