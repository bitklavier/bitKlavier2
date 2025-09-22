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
     * todo: need to make sure that if the user tries to increase numLayers > 20 that this doesn't break
     */
    for (int i = 0; i < 20; i++)
    {
        clusters.add(new SynchronicCluster(&state.params));
    }

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

    keysDepressed = juce::Array<int>();
    clusterKeysDepressed = juce::Array<int>();
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

bool SynchronicProcessor::checkVelMinMax(int clusterNotesSize)
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

    // start with a clean slate of noteOn specifications; assuming normal noteOns without anything special
    noteOnSpecMap.clear();
    updatedTransps.clear();

    // constrain number of clusters to numLayers
    //      numClusters in old bK is numLayers
//    while (clusters.size() > state.params.numLayers->getCurrentValue())
//    {
//        clusters.remove(0);
//    }

    //do this every block, for adaptive tempo updates
    thresholdSamples = state.params.clusterThreshold->getCurrentValue() * getSampleRate() * .001;
    auto sMode = state.params.pulseTriggeredBy->get();

    /**
     * todo: adaptive tempo stuff as needed, along with General Settings
     */
    // from the attached Tempo preparation: number of samples per beat
    beatThresholdSamples = getBeatThresholdSeconds() * getSampleRate();

    // keep track of how long keys have been held down, for holdTime check
    for (auto key : keysDepressed)
    {
        juce::uint64 time = holdTimers.getUnchecked(key) + numSamples;
        holdTimers.setUnchecked(key, time);
    }

    //cluster management
    if (inCluster)
    {
        //moved beyond clusterThreshold time, done with cluster
        if (clusterThresholdTimer >= thresholdSamples) inCluster = false;

        //otherwise increment cluster timer
        else clusterThresholdTimer += numSamples;
    }

    for (int i = clusters.size(); --i >= 0;)
    {
        SynchronicCluster* cluster = clusters.getUnchecked(i);

        if (cluster->getShouldPlay())
        {

            // get the current cluster of notes, which we'll cook down to a slimCluster, with duplicate pitches removed
            juce::Array<int> clusterNotes = cluster->getCluster();

            //cap size of slimCluster, removing oldest notes
            juce::Array<int> tempCluster;
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
            if(tempCluster.size() > state.params.clusterThickness->getCurrentValue()) tempCluster.resize(state.params.clusterThickness->getCurrentValue());

            //remove duplicates from cluster, so we don't play the same note twice in a single pulse
            slimCluster.clearQuick();
            for(int i = 0; i < tempCluster.size(); i++)
            {
                slimCluster.addIfNotAlreadyThere(tempCluster.getUnchecked(i));
            }

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

                // check to see whether velocity is in range to play this cluster
                // if so, play it, if playNow is true (set just above)
                if (playNow && checkVelMinMax(clusterNotes.size()))
                {
                    // the slimCluster is the cluster of notes in teh metronome pulse with duplicate notes removed
                    for (int n=0; n < slimCluster.size(); n++)
                    {
                        /**
                         * todo: when the transposition multislider is 2d, need to update here
                         */
                        // prepare the transpositions
                        updatedTransps.addIfNotAlreadyThere(state.params.transpositions.sliderVals[cluster->transpCounter]);

                        // put together the midi message
                        int newNote = slimCluster[n];
                        float velocityMultiplier = state.params.accents.sliderVals[cluster->accentMultiplierCounter];
                        auto newmsg = juce::MidiMessage::noteOn (1, newNote, static_cast<juce::uint8>(velocityMultiplier * clusterVelocities.getUnchecked(newNote)));

                        // Synchronic uses its own ADSRs for each cluster, so we need to add these to the noteOnSpecMap that gets passed to BKSynth
                        //  these apply regardless of playback direction
                        noteOnSpecMap[newNote].envParams.attack = state.params.envelopeSequence.envStates.attacks[cluster->envelopeCounter] * .001; // BKADSR expects seconds, not ms
                        noteOnSpecMap[newNote].envParams.decay = state.params.envelopeSequence.envStates.decays[cluster->envelopeCounter] * .001;
                        noteOnSpecMap[newNote].envParams.sustain = state.params.envelopeSequence.envStates.sustains[cluster->envelopeCounter];
                        noteOnSpecMap[newNote].envParams.release = state.params.envelopeSequence.envStates.releases[cluster->envelopeCounter] * .001;
                        noteOnSpecMap[newNote].envParams.attackPower = state.params.envelopeSequence.envStates.attackPowers[cluster->envelopeCounter];
                        noteOnSpecMap[newNote].envParams.decayPower = state.params.envelopeSequence.envStates.decayPowers[cluster->envelopeCounter];
                        noteOnSpecMap[newNote].envParams.releasePower = state.params.envelopeSequence.envStates.releasePowers[cluster->envelopeCounter];

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
                            //float newNoteDuration = fabs(state.params.sustainLengthMultipliers.sliderVals[cluster->lengthMultiplierCounter] * (60.0 / tempoTemp) * 1000.);
                            float newNoteDuration = fabs(state.params.sustainLengthMultipliers.sliderVals[cluster->lengthMultiplierCounter] * getBeatThresholdSeconds() * 1000.);
                            noteOnSpecMap[newNote].startDirection = Direction::backward;
                            noteOnSpecMap[newNote].startTime = newNoteDuration;
                            noteOnSpecMap[newNote].stopSameCurrentNote = false;

                            // add it to the midiBuffer for BKSynth to process
                            outMidiMessages.addEvent(newmsg, 0);
                        }

                        // reset the timer for keeping track how long this note has been sustained
                        sustainedNotesTimers[newNote] = 0;
                    }
                }

                // increment the remaining counters, check if we should continue to play
                cluster->postStep();
            }

            /*
             * handle turning off notes here
             */
            juce::uint64 noteLength_samples = 0;
            /**
             * todo: handle tempo/general stuff...
             */
            noteLength_samples = fabs(state.params.sustainLengthMultipliers.sliderVals[cluster->lengthMultiplierCounter]) * getSampleRate() * getBeatThresholdSeconds();

            // check to see if the notes have been sustained their desired length
            for (auto tm = sustainedNotesTimers.begin(); tm != sustainedNotesTimers.end(); /* no increment here */)
            {
                if (tm->second > noteLength_samples)
                {
                    auto newmsg = juce::MidiMessage::noteOff (1, tm->first, clusterVelocities.getUnchecked(tm->first));
                    outMidiMessages.addEvent(newmsg, 0);

                    // remove note from the sustainedNotes timer map, since we don't need it anymore
                    // The erase() method returns the iterator to the next element.
                    tm = sustainedNotesTimers.erase(tm);
                }
                else
                {
                    // Manually increment the iterator to move to the next element
                    // because the current element was not erased.
                    ++tm;
                }
            }

            //pass time until next beat, increment phasor/timers
            cluster->incrementPhasor(numSamples);
            incrementSustainedNotesTimers(numSamples);

            // update current slider val for UI
            state.params.transpositions_current.store(cluster->transpCounter);
            state.params.accents_current.store(cluster->accentMultiplierCounter);
            state.params.sustainLengthMultipliers_current.store(cluster->lengthMultiplierCounter);
            state.params.beatLengthMultipliers_current.store(cluster->beatMultiplierCounter);
            state.params.envelopes_current.store(cluster->envelopeCounter);

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

    /**
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
        synchronicSynth->updateMidiNoteTranspositions (updatedTransps, useTuningForTranspositions);
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

bool SynchronicProcessor::updateCluster(SynchronicCluster* _cluster, int _noteNumber)
{
    bool newCluster = false;

    // if we have a new cluster
    if (!inCluster || _cluster == nullptr)
    {
        //        // check to see if we have as many clusters as we are allowed, remove oldest if we are
        //        //      numClusters in old bK is numLayers
        //        if (clusters.size() >= state.params.numLayers->getCurrentValue())
        //        {
        //            clusters.remove(0); // remove first (oldest) cluster
        //        }
        //
        //        /**
        //         * todo: note that we are doing this on the audio thread, which is allocating memory!
        //         *         - rework all this so we don't do that, as this is likely the cause of the thread safety issues we're having here
        //         *         - probably just create a large number of them, as we do BKSynthVoices, and keep track of which ones are active
        //         */
        //        // make the new one and add it to the array of clusters
        //        _cluster = new SynchronicCluster(&state.params);
        //        clusters.add(_cluster); // add to the array of clusters (what are called "layers" in the UI)

        /*
         * where i'm at: trying to figure out how to keep track of the currentCluster, capped at numLayers
         * right now for some reason it's allowing values greater than it should
         *  with layers set to 2, i get currentCluster values of 0, 1, 2....
         *  need to get this sorted for deleteOldest, rotate, etc...
         */
        for (int i = 0; i < state.params.numLayers->getCurrentValue(); ++i)
        {
            DBG("updateCluster i counter = " + juce::String(i));
            auto tempCluster = clusters.getUnchecked(i);
            if(!tempCluster->getShouldPlay())
            {
                currentCluster = i;
                DBG("breaking out");
                break;
            }
        }
        DBG("updateCluster currentCluster = " + juce::String(currentCluster));
        //if(currentCluster < state.params.numLayers->getCurrentValue() - 1) currentCluster++;
        _cluster = clusters.getUnchecked(currentCluster);
        _cluster->reset();

        // this is a new cluster!
        newCluster = true;
    }

    // add this played note to the cluster
    _cluster->addNote(_noteNumber);

    // yep, we are in a cluster!
    inCluster = true;

    // reset the timer for time between notes; we do this for every note added to a cluster
    clusterThresholdTimer = 0;

    return newCluster;
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
            for(auto& cl : clusters)
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

        case SynchronicTargetDeleteOldest:
            doDeleteOldest = true;
            break;

        case SynchronicTargetDeleteNewest:
            doDeleteNewest = true;
            break;

        case SynchronicTargetRotate:
            doRotate = true;
            break;
    }

    //DBG("handleMidiTargetMessages = " + juce::String(channel + (SynchronicTargetFirst)));
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

    // is this a new cluster?
    bool isNewCluster = false;

    // always work on the most recent cluster/layer
    //SynchronicCluster* cluster = clusters.getLast();
    /*
     * todo: replace by something that gets us the most recent
     */
    SynchronicCluster* cluster = clusters.getUnchecked(currentCluster);

    /*
     * ************** doCluster => default Synchronic behavior **************
     */
    if (doCluster)
    {
        clusterKeysDepressed.addIfNotAlreadyThere(noteNumber);
        clusterVelocities.set(noteNumber, velocity);

        if(!doPausePlay)
        {
            // Remove old clusters, deal with layers and NoteOffSync modes
            for (int i = clusters.size(); --i >= 0; )
            {
//                if (!clusters[i]->getShouldPlay() && !inCluster)
//                {
//                    clusters.remove(i);
//                    continue;
//                }

                if((sMode == Last_NoteOff) || (sMode == Any_NoteOff))
                {
                    if(clusters.size() == 1) clusters[0]->setShouldPlay(false);
                    else
                    {
                        if(clusters[i]->containsNote(noteNumber))
                        {
                            clusters[i]->removeNote(noteNumber);
                        }
                    }
                }
            }

            // OnOffMode determines whether the keyOffs or keyOns determine whether notes are within the cluster threshold
            // here, we only look at keyOns
            if (onOffMode == Key_On) // onOffMode.value is set by the "determines cluster"
            {
                // update cluster, create as needed
                isNewCluster = updateCluster(cluster, noteNumber);
                /*
                 * todo: figure this out so we just have current or update current?
                 */
                //if(isNewCluster) cluster = clusters.getLast();
                //if(isNewCluster) cluster = clusters.getUnchecked(currentCluster);

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

//    // if we don't have a cluster, then we're triggering something before we've made a cluster and should ignore
//    if (cluster == nullptr) return;

    // since it's a new cluster, the next noteOff will be a first noteOff
    // this will be needed for keyReleased(), when in FirstNoteOffSync mode
    if (isNewCluster) nextOffIsFirst = true;

    // synchronize beat, if targeting beat sync on noteOn or on both noteOn/Off
    if (doBeatSync)
    {
        beatThresholdSamples = getBeatThresholdSeconds() * getSampleRate();

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
        for (auto& ci : clusters)
        {
            ci->setBeatPhasor(phasor);
        }
    }

    // add notes to the cluster, if targeting beat sync on noteOn or on both noteOn/Off
    if (doAddNotes )
    {
        clusterVelocities.set(noteNumber, velocity);
        cluster->addNote(noteNumber);
    }

    /*
     * todo: need to redo all these so we are not relying on the size of clusters
     */
    if (doClear)
    {
        /*
         * todo: send all notes off here as well
         */
        clusters.clear();
    }

    if (doDeleteOldest)
    {
        if (!clusters.isEmpty()) clusters.remove(0);
    }

    if (doDeleteNewest)
    {
        if (!clusters.isEmpty()) clusters.remove(clusters.size() - 1);
    }

    if (doRotate )
    {
        if (!clusters.isEmpty())
        {
            clusters.move(clusters.size() - 1, 0);
        }
    }
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

    // always work on the most recent cluster/layer
//    SynchronicCluster* cluster = clusters.getLast();
    SynchronicCluster* cluster = clusters.getUnchecked(currentCluster);

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
            // OnOffMode determines whether the timing of keyOffs or keyOns determine whether notes are within the cluster threshold
            // in this case, we only want to do these things when keyOffs set the clusters
            if (onOffMode == SynchronicClusterTriggerType::Key_Off) // set in the "determines cluster" menu
            {
                // update cluster, create as needed
                isNewCluster = updateCluster (cluster, noteNumber);

                // if it's a new cluster, the next noteOff will be a first noteOff
                // this will be needed for FirstNoteOffSync mode
                if (isNewCluster)
                    nextOffIsFirst = true;
            }

            // depending on the mode, and whether this is a first or last note, reset the beat and pattern phase and start playing
            if ((sMode == SynchronicPulseTriggerType::First_NoteOff && nextOffIsFirst) || (sMode == SynchronicPulseTriggerType::Any_NoteOff) || (sMode == SynchronicPulseTriggerType::Last_NoteOff && clusterKeysDepressed.size() == 0))
            {
                for (int i = clusters.size(); --i >= 0;)
                {
                    if (clusters[i]->containsNote (noteNumber))
                    {
                        clusters[i]->resetPatternPhase();
                        clusters[i]->setShouldPlay (true);

                        //start right away
                        juce::uint64 phasor = beatThresholdSamples * state.params.beatLengthMultipliers.sliderVals[cluster->beatMultiplierCounter].load();
                        clusters[i]->setBeatPhasor (phasor);
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
        for (auto& ci : clusters)
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

    /*
     * todo: redo these without relying on clusters size and so on
     */
    if (doClear)
    {
        clusters.clear();
    }

    if (doDeleteOldest)
    {
        if (!clusters.isEmpty()) clusters.remove(0);
    }

    if (doDeleteNewest)
    {
        if (!clusters.isEmpty()) clusters.remove(clusters.size() - 1);
    }

    if (doRotate)
    {
        if (!clusters.isEmpty())
        {
            clusters.move(clusters.size() - 1, 0);
        }
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