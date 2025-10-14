//
// Created by Joshua Warner on 6/27/24.
//

#include "SynchronicProcessor.h"
#include "Synthesiser/Sample.h"
#include "common.h"
#include "synth_base.h"

SynchronicProcessor::SynchronicProcessor(SynthBase& parent, const juce::ValueTree& vt) :
      PluginBase (parent, vt, nullptr, synchronicBusLayout()),
      synchronicSynth (new BKSynthesiser (state.params.env, state.params.noteOnGain))
{
    // for testing
    bufferDebugger = new BufferDebugger();

    for (int i = 0; i < 300; i++)
    {
        synchronicSynth->addVoice (new BKSamplerVoice());
    }

    /*
     * todo: need to make sure that if the user tries to increase numLayers > MAX_CLUSTERS that this doesn't break
     */
    for (int i = 0; i < MAX_CLUSTERS; i++)
    {
        clusterLayers[i] = new SynchronicCluster(&state.params);
    }

    for (int i = 0; i < MaxMidiNotes; ++i)
    {
        noteOnSpecMap[i] = NoteOnSpec{};
        noteOnSpecMap[i].transpositions = {0.};
    }

//    updatedTransps.ensureStorageAllocated(50);
    slimCluster.ensureStorageAllocated(100);
    tempCluster.ensureStorageAllocated(100);
    clusterNotes.ensureStorageAllocated(128);
    keysDepressed = juce::Array<int>();
    keysDepressed.ensureStorageAllocated(100);
    clusterKeysDepressed = juce::Array<int>();
    clusterKeysDepressed.ensureStorageAllocated(100);

    /*
     * state-change parameter stuff (for multisliders)
     */
    state.params.transpositions.stateChanges.defaultState               = v.getOrCreateChildWithName(IDs::PARAM_DEFAULT,nullptr);
    state.params.accents.stateChanges.defaultState                      = v.getOrCreateChildWithName(IDs::PARAM_DEFAULT,nullptr);
    state.params.sustainLengthMultipliers.stateChanges.defaultState     = v.getOrCreateChildWithName(IDs::PARAM_DEFAULT,nullptr);
    state.params.beatLengthMultipliers.stateChanges.defaultState        = v.getOrCreateChildWithName(IDs::PARAM_DEFAULT,nullptr);

    parent.getStateBank().addParam (std::make_pair<std::string,
        bitklavier::ParameterChangeBuffer*> (v.getProperty (IDs::uuid).toString().toStdString() + "_" + "transpositions_",
        &(state.params.transpositions.stateChanges)));
    parent.getStateBank().addParam (std::make_pair<std::string,
        bitklavier::ParameterChangeBuffer*> (v.getProperty (IDs::uuid).toString().toStdString() + "_" + "accents_",
        &(state.params.accents.stateChanges)));
    parent.getStateBank().addParam (std::make_pair<std::string,
        bitklavier::ParameterChangeBuffer*> (v.getProperty (IDs::uuid).toString().toStdString() + "_" + "sustain_length_multipliers",
        &(state.params.sustainLengthMultipliers.stateChanges)));
    parent.getStateBank().addParam (std::make_pair<std::string,
        bitklavier::ParameterChangeBuffer*> (v.getProperty (IDs::uuid).toString().toStdString() + "_" + "beat_length_multipliers",
        &(state.params.beatLengthMultipliers.stateChanges)));

    state.params.clusterMinMaxParams.stateChanges.defaultState = v.getOrCreateChildWithName(IDs::PARAM_DEFAULT,nullptr);
    parent.getStateBank().addParam (std::make_pair<std::string,
        bitklavier::ParameterChangeBuffer*> (v.getProperty (IDs::uuid).toString().toStdString() + "_" + "cluster_min_max",
        &(state.params.clusterMinMaxParams.stateChanges)));

    state.params.holdTimeMinMaxParams.stateChanges.defaultState = v.getOrCreateChildWithName(IDs::PARAM_DEFAULT,nullptr);
    parent.getStateBank().addParam (std::make_pair<std::string,
        bitklavier::ParameterChangeBuffer*> (v.getProperty (IDs::uuid).toString().toStdString() + "_" + "holdtime_min_max",
        &(state.params.holdTimeMinMaxParams.stateChanges)));

    state.params.envelopeSequence.stateChanges.defaultState = v.getOrCreateChildWithName(IDs::PARAM_DEFAULT,nullptr);
    state.params.envelopeSequence.stateChanges.defaultState = v.getOrCreateChildWithName(IDs::PARAM_DEFAULT,nullptr);
    parent.getStateBank().addParam (std::make_pair<std::string,
        bitklavier::ParameterChangeBuffer*> (v.getProperty (IDs::uuid).toString().toStdString() + "_" + "envelope_sequence",
        &(state.params.envelopeSequence.stateChanges)));

    /*
     * Init Synchronic params
     */
    for (int i = 0; i < 128; i++)
    {
        holdTimers.add(0);
        clusterVelocities.add(0);
    }

    inCluster = false;
}

void SynchronicProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const auto spec = juce::dsp::ProcessSpec { sampleRate, (uint32_t) samplesPerBlock, (uint32_t) getMainBusNumInputChannels() };
    synchronicSynth->setCurrentPlaybackSampleRate (sampleRate);
}

bool SynchronicProcessor::isBusesLayoutSupported (const juce::AudioProcessor::BusesLayout& layouts) const
{
    return true;
}

void SynchronicProcessor::setTuning (TuningProcessor* tun)
{
    tuning = tun;
    synchronicSynth->setTuning (&tuning->getState().params.tuningState);
}

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

bool SynchronicProcessor::checkClusterMinMax (int clusterNotesSize)
{
    //figure out whether to play the cluster, based on clusterMin and Max settings
    bool passCluster = false;

    //in the normal case, where cluster is within a range defined by clusterMin and Max
    int sClusterMin = state.params.clusterMinMaxParams.clusterMinParam->getCurrentValue();
    int sClusterMax = state.params.clusterMinMaxParams.clusterMaxParam->getCurrentValue();

    if(sClusterMin <= sClusterMax)
    {
        if (clusterNotesSize >= sClusterMin && clusterNotesSize <= sClusterMax)
            passCluster = true;
    }
    //the inverse case, where we only play cluster that are *outside* the range set by clusterMin and Max
    else
    {
        if (clusterNotesSize >= sClusterMin || clusterNotesSize <= sClusterMax)
            passCluster = true;
    }

    return passCluster;
}

/**
 * leaving these unimplemented for now, maybe permanently
 * - had them in the old bK, but i'm not sure they are useful, trying to simplify as possible
 */
void SynchronicProcessor::removeOldestCluster()
{
//    clusters[0]->setShouldPlay(false);
//    std::rotate(clusters.begin(), std::next(clusters.begin()), clusters.end());
//    mostRecentCluster--;
//    if(mostRecentCluster < 0) mostRecentCluster = 0;
}

void SynchronicProcessor::removeNewestCluster()
{
//    clusters[mostRecentCluster]->setShouldPlay(false);
//    mostRecentCluster--;
//    if(mostRecentCluster < 0) mostRecentCluster = 0;
}

// rotate the clusters so that the oldest (0) becomes the most recent
void SynchronicProcessor::rotateClusters()
{
//    if (mostRecentCluster > 0 && mostRecentCluster < clusters.size()) {
//        // The sub-range to be rotated is defined by iterators from startIndex to endIndex.
//        // `first` is the beginning of the sub-range.
//        auto first = std::next(clusters.begin(), 0);
//
//        // `n_first` is the element that becomes the new first of the sub-range.
//        // For a single left rotation, this is the element immediately after `first`.
//        auto n_first = std::next(first);
//
//        // `last` is the end of the sub-range.
//        auto last = std::next(clusters.begin(), mostRecentCluster);
//
//        // Perform the rotation on the sub-range.
//        std::rotate(first, n_first, last);
//    }
}

void SynchronicProcessor::ProcessMIDIBlock(juce::MidiBuffer& inMidiMessages, juce::MidiBuffer& outMidiMessages, int numSamples)
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
            keyReleased(message.getNoteNumber(), message.getChannel());
    }

    if(doPausePlay) return;

    // trigger type
    auto sMode = state.params.pulseTriggeredBy->get();

    // start with a clean slate of noteOn specifications; assuming normal noteOns without anything special
    for (auto& spec : noteOnSpecMap)
    {
        spec.clear();
        //spec.transpositions = {0.};
    }

//    updatedTransps.clear();

    // keep track of how long keys have been held down, for holdTime check
    for (auto key : keysDepressed)
    {
        juce::uint64 time = holdTimers.getUnchecked(key) + numSamples;
        holdTimers.setUnchecked(key, time);
    }

    /*
     * cluster management
     * inCluster is true if the time since the last note played was less than clusterThreshold
     * - gathers notes into a single cluster that is played metronomically
    */
    thresholdSamples = state.params.clusterThreshold->getCurrentValue() * getSampleRate() * .001;
    if (inCluster)
    {
        //moved beyond clusterThreshold time, done with cluster
        if (clusterThresholdTimer >= thresholdSamples) inCluster = false;

        //otherwise increment cluster timer
        else clusterThresholdTimer += numSamples;
    }

    /**
     * todo: adaptive tempo stuff as needed, along with General Settings
     */
    // from the attached Tempo preparation: number of samples per beat
    // update this every block, for adaptive tempo updates
    beatThresholdSamples = getBeatThresholdSeconds() * getSampleRate();

    // all noteOn messages for all the clusters
    for (auto cluster : clusterLayers)
    {
        if (cluster->getShouldPlay() && !cluster->getIsOver())
        {
            // adjust samples per beat by beat length multiplier, for this beat
            numSamplesBeat = beatThresholdSamples * state.params.beatLengthMultipliers.sliderVals[cluster->beatMultiplierCounter].load();

            //check to see if enough time has passed for next beat
            if (cluster->getPhasor() >= numSamplesBeat)
            {
                // if patternSync has been set by a target message, reset the phase of all the counters
                if (cluster->doPatternSync)
                {
                    cluster->resetPatternPhase();
                    cluster->doPatternSync = false;
                }

                /*
                 * the skipFirst option can make things complicated
                 *  - when skipFirst == true, we skip the first value in the various patterns
                 *      -- usually because we want the first note to be what we play (probably heard in Direct)
                 *          and not have Synchronic repeat it or play basically in sync with it
                 *      -- this is actually the easy case
                 *  - when skipFirst == false we start at the beginning of the pattern, which is oddly the more complicated case
                 *  - and we need the behavior to make sense in the various trigger modes (noteOn or noteOff triggers)
                 */

                // we need this to deal with noteOn triggered cases where we don't want clusters to play immediately
                bool playNow = true;

                // easy case: if we are skipping the first pattern value, we always increment the pattern counters, in step()
                if(state.params.skipFirst.get()->get()) cluster->step(numSamplesBeat);

                // it gets messy when we are not skipping the first pattern value
                else
                {
                    // deal with the noteOn triggered cases
                    if(sMode == Any_NoteOn || sMode == First_NoteOn)
                    {
                        // we don't want to play a cluster immediately with the noteOn message
                        if(cluster->beatCounter == 0) playNow = false;

                        // we want the timing of the first played beat to align properly
                        else if (cluster->beatCounter == 1) cluster->setBeatPhasor(0);

                        // otherwise, step the parameter values as usual
                        else cluster->step(numSamplesBeat);
                    }

                    // then the noteOff triggered cases
                    else
                    {
                        // to get the timing of the next beat correct
                        //  - note that postStep() also takes care not to increment the beat multiplier counter
                        if (cluster->beatCounter == 0) cluster->setBeatPhasor(0);

                        // otherwise step the parameter values as usual
                        else cluster->step(numSamplesBeat);
                    }
                }

                // get the current cluster of notes, which we'll cook down to a slimCluster, with duplicate pitches removed
                clusterNotes = cluster->getCluster();

                //cap size of slimCluster, removing oldest notes
                tempCluster.clearQuick();
                for(int i = 0; i < clusterNotes.size(); i++) tempCluster.set(i, clusterNotes.getUnchecked(i));

                /*
                 * constrain thickness of cluster
                 *  why not use clusterMax for this? the intent is different:
                 *  - clusterMax: max number of keys pressed within clusterThresh, otherwise shut off pulses
                 *  - clusterCap: the most number of notes allowed in a cluster when playing pulses (clusterThickness in bK2)
                 *
                 *  an example: clusterMax=9, clusterCap=8; playing 9 notes simultaneously will result in cluster with 8 notes, but playing 10 notes will shut off pulse
                 *  another example: clusterMax=20, clusterCap=8; play a rapid ascending scale more than 8 and less than 20 notes, then stop; only last 8 notes will be in the cluster. If your scale exceeds 20 notes then it won't play.
                 */
                /**
                 * todo: redo this without using resize, which might release the ensured memory allocation and reallocate
                 *          - should be able to do it in the for loop above, stopping adding after clusterThickness has been reached
                 */
                if(tempCluster.size() > state.params.clusterThickness->getCurrentValue()) tempCluster.resize(state.params.clusterThickness->getCurrentValue());

                //remove duplicates from cluster, so we don't play the same note twice in a single pulse
                slimCluster.clearQuick();
                for(int i = 0; i < tempCluster.size(); i++)
                {
                    slimCluster.addIfNotAlreadyThere(tempCluster.getUnchecked(i));
                }

                // check to see whether number of notes played is within cluster min/max
                // if so, play it, if playNow is true (set just above)
                if (playNow && checkClusterMinMax (clusterNotes.size()))
                {
                    // the slimCluster is the cluster of notes in the metronome pulse with duplicate notes removed
                    for (int n=0; n < slimCluster.size(); n++)
                    {
                        /**
                         * todo: when the transposition multislider is 2d, need to update here
                         */
                        // prepare the transpositions
                        //updatedTransps.addIfNotAlreadyThere(state.params.transpositions.sliderVals[cluster->transpCounter]);

                        // put together the midi message
                        int newNote = slimCluster[n];
                        float newTransp = state.params.transpositions.sliderVals[cluster->transpCounter];
                        float velocityMultiplier = state.params.accents.sliderVals[cluster->accentMultiplierCounter];
                        auto newmsg = juce::MidiMessage::noteOn (1, newNote, static_cast<juce::uint8>(velocityMultiplier * clusterVelocities.getUnchecked(newNote)));

                        // Synchronic uses its own ADSRs for each cluster, so we need to add these to the noteOnSpecMap that gets passed to BKSynth
                        // - these apply regardless of playback direction
                        noteOnSpecMap[newNote].keyState = true;
                        noteOnSpecMap[newNote].envParams.attack = state.params.envelopeSequence.envStates.attacks[cluster->envelopeCounter] * .001; // BKADSR expects seconds, not ms
                        noteOnSpecMap[newNote].envParams.decay = state.params.envelopeSequence.envStates.decays[cluster->envelopeCounter] * .001;
                        noteOnSpecMap[newNote].envParams.sustain = state.params.envelopeSequence.envStates.sustains[cluster->envelopeCounter];
                        noteOnSpecMap[newNote].envParams.release = state.params.envelopeSequence.envStates.releases[cluster->envelopeCounter] * .001;
                        noteOnSpecMap[newNote].envParams.attackPower = state.params.envelopeSequence.envStates.attackPowers[cluster->envelopeCounter];
                        noteOnSpecMap[newNote].envParams.decayPower = state.params.envelopeSequence.envStates.decayPowers[cluster->envelopeCounter];
                        noteOnSpecMap[newNote].envParams.releasePower = state.params.envelopeSequence.envStates.releasePowers[cluster->envelopeCounter];
                        noteOnSpecMap[newNote].transpositions.clearQuick();
                        noteOnSpecMap[newNote].transpositions.add(newTransp);

                        // forward and backwards notes need to be handled differently, for BKSynth
                        if(state.params.sustainLengthMultipliers.sliderVals[cluster->lengthMultiplierCounter] > 0.)
                        {
                            // forward-playing note: add to the midiBuffer that gets passed to BKSynth
                            outMidiMessages.addEvent(newmsg, 0);
                        }
                        else
                        {
                            /*
                             * backwards-playing note
                             *
                             *  - for these we need to set values in noteOnSpec and put that in noteOnSpecMap for this midiNote
                             *  - noteOnSpecMap will get passed on to BKSynth so it can do what it needs to do for a backward note
                             *
                             * note that the noteOnSpecMap will apply to all other notes == newNote for this block!
                             *  - shouldn't be an issue, unless note playback is very fast or block is very large
                             *      AND we get multiple noteOn msgs in the same block that want different noteOnSpecs
                             */
                            float newNoteDuration = fabs(state.params.sustainLengthMultipliers.sliderVals[cluster->lengthMultiplierCounter] * getBeatThresholdSeconds() * 1000.);
                            noteOnSpecMap[newNote].keyState = true;
                            noteOnSpecMap[newNote].startDirection = Direction::backward;
                            noteOnSpecMap[newNote].startTime = newNoteDuration;
                            noteOnSpecMap[newNote].stopSameCurrentNote = false;

                            // add it to the midiBuffer for BKSynth to process
                            outMidiMessages.addEvent(newmsg, 0);
                        }

                        // reset the timer for keeping track how long this note has been sustained
                        // 1. Search the array to find the index where the tuple's first member matches newNote.
                        int foundIndex = -1;
                        for (int i = 0; i < cluster->sustainedNotesTimers.size(); ++i)
                        {
                            // Access the key (index 0) using getReference() for safety/efficiency
                            int keyInTuple = std::get<0>(cluster->sustainedNotesTimers.getReference(i));

                            if (keyInTuple == newNote)
                            {
                                foundIndex = i;
                                break;
                            }
                        }

                        // 2. Assign the new value ONLY if the key was found.
                        if (foundIndex != -1)
                        {
                            std::get<1>(cluster->sustainedNotesTimers.getReference(foundIndex)) = 0;
                        }
                    }
                }

                // increment the remaining counters, check if we should continue to play
                cluster->postStep();
            }

            // update current slider val for UI
            state.params.transpositions_current.store(cluster->transpCounter);
            state.params.accents_current.store(cluster->accentMultiplierCounter);
            state.params.sustainLengthMultipliers_current.store(cluster->lengthMultiplierCounter);
            state.params.beatLengthMultipliers_current.store(cluster->beatMultiplierCounter);
            state.params.envelopes_current.store(cluster->envelopeCounter);

        }

        //pass time until next beat, increment phasor/timers
        cluster->incrementPhasor(numSamples);
        cluster->incrementSustainedNotesTimers(numSamples);
    }

    /*
     * handle noteOff messaging here
     * - we need to do this when "over" is true, since there may be sustained notes for the last pulse in a layer
     *      - "over" is true when the number of beats played > num pulses; the sequence is over
     * - we also need to do it for all the regular notes, when "shouldPlay" is true
     * - because this is a complicated processBlock, we run through the clusters again, keeping the noteOn
     *      work (above) separate from the noteOff work; only 10 layers, so not a big CPU hit
     */
    for (auto cluster : clusterLayers)
    {
        if(cluster->getIsOver() || cluster->getShouldPlay())
        {
            // if no notes are playing in this cluster, we can move on, resetting if this cluster is over (exceeded num pulses)
            if(cluster->sustainedNotesTimers.size() == 0) {
                if (cluster->getIsOver()) cluster->reset();
                continue;
            }

            /**
             * todo: handle tempo/general stuff...
             */
            // get the sustain length that we need to compare to
            juce::uint64 noteLength_samples = 0;
            noteLength_samples = fabs(state.params.sustainLengthMultipliers.sliderVals[cluster->lengthMultiplierCounter]) * getSampleRate() * getBeatThresholdSeconds();

            // check to see if the notes have been sustained their desired length
            // 1. Convert to an index-based loop for safe removal
            int i = 0;
            while (i < cluster->sustainedNotesTimers.size())
            {
                auto& tm = cluster->sustainedNotesTimers.getReference(i);

                juce::uint64 currentlength = std::get<1>(tm);
                if (currentlength > noteLength_samples)
                {
                    // 2. Perform MIDI Note Off
                    // std::get<0>(tm) gets the note number (the int)
                    auto newmsg = juce::MidiMessage::noteOff (1, std::get<0>(tm), clusterVelocities.getUnchecked(std::get<0>(tm)));
                    outMidiMessages.addEvent(newmsg, 0);

                    // 3. Remove the element using the index
                    // The remove() call shifts all subsequent elements to the left,
                    // effectively moving the next element into the current position 'i'.
                    cluster->sustainedNotesTimers.remove(i);

                    // DO NOT increment 'i' here. The next element is now at the current index 'i'.
                }
                else
                {
                    // 4. If not removed, move to the next index.
                    ++i;
                }
            }
        }
    }
}

void SynchronicProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
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
     * do the MIDI stuff here
     */

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

    // use these to display buffer info to bufferDebugger
    bufferDebugger->capture("L", buffer.getReadPointer(0), numSamples, -1.f, 1.f);
    bufferDebugger->capture("R", buffer.getReadPointer(1), numSamples, -1.f, 1.f);

    /*
     * then the synthesizer process blocks
     */
    bool useTuningForTranspositions = state.params.transpositionUsesTuning->get();
    if (synchronicSynth->hasSamples())
    {
        synchronicSynth->setBypassed (false);
//        synchronicSynth->updateMidiNoteTranspositions (updatedTransps, useTuningForTranspositions);
        synchronicSynth->setNoteOnSpecMap(noteOnSpecMap);
        synchronicSynth->renderNextBlock (buffer, outMidi, 0, buffer.getNumSamples());
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
     * todo: handle noteOffs, otherwise nothing?
     */
}

bool SynchronicProcessor::holdCheck(int noteNumber)
{
    juce::uint64 hold = holdTimers.getUnchecked(noteNumber) * (1000.0 / getSampleRate());
    //DBG("holdCheck val = " + juce::String(holdTimers.getUnchecked(noteNumber)));

    auto holdmin = state.params.holdTimeMinMaxParams.holdTimeMinParam->getCurrentValue();
    auto holdmax = state.params.holdTimeMinMaxParams.holdTimeMaxParam->getCurrentValue();

    if(holdmin <= holdmax)
    {
        if (hold >= holdmin && hold <= holdmax)
        {
            return true;
        }
    }
    else
    {
        if (hold >= holdmin || hold <= holdmax)
        {
            return true;
        }
    }

    DBG("failed hold check");
    return false;
}

/**
 * updates what the current cluster is, and turns off older clusters
 * @return whether this is a newcluster or not (bool)
 */
bool SynchronicProcessor::updateCurrentCluster()
{
    bool ncluster = false;

    // if we have a new cluster
    if (!inCluster)
    {
        // move to the next layer
        currentLayerIndex++;
        if (currentLayerIndex >= clusterLayers.size()) currentLayerIndex = 0;

        // turn off oldest cluster
        int oldestClusterIndex = currentLayerIndex - std::round(state.params.numLayers->getCurrentValue());
        while (oldestClusterIndex < 0) oldestClusterIndex += clusterLayers.size();
        clusterLayers[oldestClusterIndex]->setIsOver(true); // tell the cluster that it's done, and should only send noteOffs for the currently sounding cluster

        DBG("num layers = " + juce::String(std::round(state.params.numLayers->getCurrentValue())));
        DBG("new cluster = " + juce::String(currentLayerIndex) + " and turning off cluster " + juce::String(oldestClusterIndex));

        ncluster = true;
    }

    return ncluster;
}

/**
 * sets the bools for this message based on channel, set in MidiTarget
 * @param channel
 */
void SynchronicProcessor::handleMidiTargetMessages(int channel)
{
    /*
    Synchronic Target modes:
       01. Synchronic: current functionality; launches clusters/layers, syncs, etc.... (doCluster)
       02. Pattern Sync: calls cluster->resetPhase(), regardless of aPrep->getMode(), last cluster/layer (doPatternSync)
            eventually, we could allow targeting of individual patterns
       03. Beat Sync: calls cluster->setPhasor(0), regardless of aPrep->getMode(), last cluster/layer (doSync)
       04. Pause/Play: stop/start incrementing phasor, all clusters (could also have Pause/Play Last, for last cluster only)
       05. Add Notes: calls cluster->addNote(noteNumber), last cluster/layer
       06. Remove Oldest Note?: call cluster->removeOldestNote(); a way of thinning a cluster
            could combine with Add Notes to transform a cluster
       07. Remove Newest Note?: call cluster->removeNewestNote()
       08. Delete Oldest Layer:
       09. Delete Newest Layer:
       10. Rotate Layers: newest becomes oldest, next newest becomes newest
       11. Remove All Layers: essentially a Stop function

            too many? i can imagine these being useful though
     */

    doCluster = false; // primary Synchronic mode
    doBeatSync = false; // resetting beat phase
    doAddNotes = false; // adding notes to cluster
    doClear = false;
    doDeleteOldest = false;
    doDeleteNewest = false;
    doRotate = false;
    /*
     * don't set doPausePlay to false here; toggle below
     * also for doPatternSync, which gets toggled internally below
     */

    switch(channel + (SynchronicTargetFirst))
    {
        case SynchronicTargetDefault:
            doCluster = true;
            break;

        case SynchronicTargetPatternSync:
            for(auto& cl : clusterLayers)
            {
                cl->doPatternSync = true;
            }
            break;

        case SynchronicTargetBeatSync:
            doBeatSync = true;
            break;

        case SynchronicTargetAddNotes:
            doAddNotes = true;
            break;

        case SynchronicTargetClear:
            doClear = true;
            break;

        case SynchronicTargetPausePlay:
            if (doPausePlay) doPausePlay = false;
            else doPausePlay = true;
            break;

/*
 * these are currently unimplemented
 */
//        case SynchronicTargetDeleteOldest:
//            doDeleteOldest = true;
//            break;
//
//        case SynchronicTargetDeleteNewest:
//            doDeleteNewest = true;
//            break;
//
//        case SynchronicTargetRotate:
//            doRotate = true;
//            break;
    }

    //DBG("handleMidiTargetMessages = " + juce::String(channel + (SynchronicTargetFirst)));
}

int SynchronicProcessor::findIndexOfCluster(SynchronicCluster* item) {
    for (int i = 0; i < clusterLayers.size(); ++i) {
        if (clusterLayers[i] == item) {
            return i;
        }
    }
    // Return a value indicating not found, e.g., arr.size() or throw an exception.
    // For this example, we'll assume the item is always present.
    return -1; // Or any other appropriate error value
}

void SynchronicProcessor::keyPressed(int noteNumber, int velocity, int channel)
{
    // set all the modes, handled below (like doCluster, etc...)
    // only one mode will be set for each keyPressed call
    handleMidiTargetMessages(channel);

    // modes for use later...
    auto sMode = state.params.pulseTriggeredBy->get();
    auto onOffMode = state.params.determinesCluster->get();

    // reset the timer for how long this key has been pressed
    holdTimers.set(noteNumber, 0);

    // add note to array of depressed notes
    keysDepressed.addIfNotAlreadyThere(noteNumber);

    // is this a new cluster? assume not to start
    bool isNewCluster = false;

    /*
     * ************** doCluster => default Synchronic behavior **************
     */
    if (doCluster)
    {
        clusterKeysDepressed.addIfNotAlreadyThere(noteNumber);
        clusterVelocities.set(noteNumber, velocity);

        if(!doPausePlay)
        {
            // OnOffMode determines whether the keyOffs or keyOns determine whether notes are within the cluster threshold
            // - here, we only look at keyOns
            if (onOffMode == Key_On) // onOffMode.value is set by the "determines cluster"
            {
                // update currentLayerIndex, turn off old layers, determine whether this is a new cluster or not
                isNewCluster = updateCurrentCluster();

                // get the current cluster
                auto cluster = clusterLayers[currentLayerIndex];

                // reset the timer for time between notes; we do this for every note added to a cluster
                clusterThresholdTimer = 0;

                // yep, we are in a cluster!
                inCluster = true;

                // add this played note to the cluster
                cluster->addNote(noteNumber);

                // reset the beat phase and pattern phase, and start playing, depending on the mode
                if (sMode == Any_NoteOn)
                {
                    cluster->setShouldPlay(true);
                    cluster->setBeatPhasor(0);
                    cluster->resetPatternPhase();
                }
                else if (sMode == First_NoteOn)
                {
                    cluster->setShouldPlay(true);

                    if (isNewCluster)
                    {
                        cluster->setBeatPhasor(0);
                        cluster->resetPatternPhase();
                    }
                }
            }
        }
    }


    /*
     * ************** now trigger behaviors set by Keymap targeting ****************
     */

    // since it's a new cluster, the next noteOff will be a first noteOff
    // this will be needed for keyReleased(), when in FirstNoteOffSync mode
    if (isNewCluster) nextOffIsFirst = true;

    // synchronize beat, if targeting beat sync on noteOn or on both noteOn/Off
    if (doBeatSync)
    {
        beatThresholdSamples = getBeatThresholdSeconds() * getSampleRate();

        // get the current cluster
        auto cluster = clusterLayers[currentLayerIndex];

        /*
         * for cases when BOTH beatSync and patternSync are selected in MidiTarget,
         * we need to reset the pattern counters here.
         */
        if (cluster->doPatternSync)
        {
            cluster->resetPatternPhase();
            cluster->doPatternSync = false;
        }

        /**
         * todo: check this
         *       - and also add the General Settings and Tempo adjustments
         */
        juce::uint64 phasor = beatThresholdSamples * state.params.beatLengthMultipliers.sliderVals[cluster->beatMultiplierCounter].load();

        // reset beat timing, for ALL clusters
        for (auto& ci : clusterLayers)
        {
            ci->setBeatPhasor(phasor);
        }
    }

    // add notes to the cluster, if targeting beat sync on noteOn or on both noteOn/Off
    if (doAddNotes )
    {
        // get the current cluster
        auto cluster = clusterLayers[currentLayerIndex];

        clusterVelocities.set(noteNumber, velocity);
        cluster->addNote(noteNumber);
    }

    if (doClear)
    {
        /*
         * todo: send all notes off here as well
         */
        //clusters.clear();
        for (auto cl : clusterLayers)
        {
            cl->setIsOver(true);
        }
    }

    /**
     * these are unimplemented, probably permanently
     */
//    if (doDeleteOldest)
//    {
//        //if (!clusters.isEmpty()) clusters.remove(0);
//        removeOldestCluster();
//    }
//
//    if (doDeleteNewest)
//    {
//        //if (!clusters.isEmpty()) clusters.remove(clusters.size() - 1);
//        removeNewestCluster();
//    }
//
//    if (doRotate )
//    {
//        rotateClusters();
//    }
}

void SynchronicProcessor::keyReleased(int noteNumber, int channel)
{

    handleMidiTargetMessages(channel);

    auto sMode = state.params.pulseTriggeredBy->get();
    auto onOffMode = state.params.determinesCluster->get();

    // remove key from array of pressed keys
    keysDepressed.removeAllInstancesOf(noteNumber);

    // is this a new cluster?
    bool isNewCluster = false;

    // do hold-time filtering (how long the key was held down)
    if (!holdCheck(noteNumber)) return;

    auto cluster = clusterLayers[currentLayerIndex];

    // the number of samples until the next beat
    beatThresholdSamples = getBeatThresholdSeconds() * getSampleRate();

    /*
     * ************** doCluster => default Synchronic behavior **************
     */
    if (doCluster)
    {
        // remove key from cluster-targeted keys
        clusterKeysDepressed.removeAllInstancesOf(noteNumber);

        if(!doPausePlay)
        {
            // cluster management
            // - OnOffMode determines whether the timing of keyOffs or keyOns determine whether notes are within the cluster threshold
            // - in this case, we only want to do these things when keyOffs set the clusters
            if (onOffMode == Key_Off) // set in the "determines cluster" menu
            {
                // update cluster, create as needed
                isNewCluster = updateCurrentCluster();
                auto cluster = clusterLayers[currentLayerIndex];

                // add this played note to the cluster
                cluster->addNote(noteNumber);

                // yep, we are in a cluster!
                inCluster = true;

                // reset the timer for time between notes; we do this for every note added to a cluster
                clusterThresholdTimer = 0;

                // if it's a new cluster, the next noteOff will be a first noteOff
                // - this will be needed for FirstNoteOffSync mode
                if (isNewCluster)
                    nextOffIsFirst = true;
            }

            // depending on the mode, and whether this is a first or last note, reset the beat and pattern phase and start playing
            if ((sMode == First_NoteOff && nextOffIsFirst) || (sMode == Any_NoteOff) || (sMode == Last_NoteOff && clusterKeysDepressed.size() == 0))
            {
                for (int i = clusterLayers.size(); --i >= 0;)
                {
                    if (clusterLayers[i]->containsNote (noteNumber))
                    {
                        clusterLayers[i]->resetPatternPhase();
                        clusterLayers[i]->setShouldPlay (true);

                        //start right away
                        juce::uint64 phasor = beatThresholdSamples * state.params.beatLengthMultipliers.sliderVals[cluster->beatMultiplierCounter].load();
                        clusterLayers[i]->setBeatPhasor (phasor);
                    }
                }

                // have done at least one noteOff, so next one will not be first one.
                nextOffIsFirst = false;
            }
        }
    }


    /*
     * ************** now trigger behaviors set by Keymap targeting ****************
     */

    // if we don't have a cluster, then we're triggering something before we've made a cluster and should ignore
    if (cluster == nullptr) return;

    // synchronize beat, if targeting beat sync on noteOff or on both noteOn/Off
    if (doBeatSync)
    {
        //start right away
        juce::uint64 phasor = beatThresholdSamples * state.params.beatLengthMultipliers.sliderVals[cluster->beatMultiplierCounter].load();

        // reset the phase for ALL clusters
        for (auto& ci : clusterLayers)
        {
            ci->setBeatPhasor(phasor);
            ci->setShouldPlay(true);
        }
    }

    // add notes to the cluster, if targeting beat sync on noteOff or on both noteOn/Off
    if (doAddNotes)
    {
        cluster->addNote(noteNumber);
    }

    if (doClear)
    {
        //clusters.clear();
        for (auto cl : clusterLayers)
        {
            cl->setIsOver(true);
        }
    }

    if (doDeleteOldest)
    {
        removeOldestCluster();
    }

    if (doDeleteNewest)
    {
        removeNewestCluster();
    }

    if (doRotate)
    {
        rotateClusters();
    }
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

    serializeArrayADSRParam<Serializer>(ser, paramHolder.envelopeSequence.envStates, "envelope_sequence");

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

    deserializeArrayADSRParam<Serializer> (deserial, paramHolder.envelopeSequence.envStates, "envelope_sequence");
}