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
     * modulations and state changes
     */
    setupModulationMappings();

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

void SynchronicProcessor::ProcessMIDIBlock(juce::MidiBuffer& inMidiMessages, juce::MidiBuffer& outMidiMessages, int numSamples)
{
    /**
     * todo: not sure this the right way to handle this?
     */
    if (pausePlay) return;

    // start with a clean slate of noteOn specifications; assuming normal noteOns without anything special
    noteOnSpecMap.clear();

    // constrain number of clusters to numLayers
    //      numClusters in old bK is numLayers
    while (clusters.size() > state.params.numLayers->getCurrentValue())
    {
        clusters.remove(0);
    }

    //do this every block, for adaptive tempo updates
    thresholdSamples = state.params.clusterThreshold->getCurrentValue() * getSampleRate() * .001;

    /**
     * todo: figure out how to handle the stuff from Tempo, to set beatThresholdSamples
     */
    //    if (tempoPrep->getTempoSystem() == AdaptiveTempo1)
    //    {
    //        beatThresholdSamples = (tempoPrep->getBeatThresh() * synth->getSampleRate());
    //    }
    //    else
    //    {
    //        beatThresholdSamples = (tempoPrep->getBeatThresh() / tempoPrep->getSubdivisions() * synth->getSampleRate());
    //    }
    beatThresholdSamples = getSampleRate() * 60.0 / tempoTemp;

    for (auto key : keysDepressed)
    {
        juce::uint64 time = holdTimers.getUnchecked(key) + numSamples;
        holdTimers.setUnchecked(key, time);
    }

    //cluster management
    if (inCluster)
    {
        //moved beyond clusterThreshold time, done with cluster
        if (clusterThresholdTimer >= thresholdSamples)
        {
            inCluster = false;
        }

        //otherwise increment cluster timer
        else
        {
            clusterThresholdTimer += numSamples;
        }
    }

    bool play = false;

    for (int i = clusters.size(); --i >= 0;)
    {
        SynchronicCluster* cluster = clusters.getUnchecked(i);

        if (cluster->getShouldPlay()) // check if pulses are done and remove cluster if so
        {
            play = true;

            juce::Array<int> clusterNotes = cluster->getCluster();

            //DBG("cluster " + String(i) + ": " + intArrayToString(clusterNotes));

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

            /**
             * todo: add multipliers by Tempo periodMultiplier and General periodMultiplier
             */
            numSamplesBeat = beatThresholdSamples * state.params.beatLengthMultipliers.sliderVals[cluster->beatMultiplierCounter].load();

            //check to see if enough time has passed for next beat
            if (cluster->getPhasor() >= numSamplesBeat)
            {
                //update display of counters in UI

//                 DBG(" samplerate: " + String(sampleRate) +
//                 " length: "         + String(prep->sLengthMultipliers.value[lengthMultiplierCounter]) +
//                 " length counter: "  + String(lengthMultiplierCounter) +
//                 " accent: "         + String(prep->getAccentMultipliers()[accentMultiplierCounter]) +
//                 " accent counter: " + String(accentMultiplierCounter) +
//                 " transp: "         + "{ "+floatArrayToString(prep->getTransposition()[transpCounter]) + " }" +
//                 " transp counter: " + String(transpCounter) +
//                 " envelope on: "       + String((int)prep->getEnvelopesOn()[envelopeCounter]) +
//                 " envelope counter: " + String(envelopeCounter) +
//                 " ADSR :" + String(prep->getAttack(envelopeCounter)) + " " + String(prep->getDecay(envelopeCounter)) + " " + String(prep->getSustain(envelopeCounter)) + " " + String(prep->getRelease(envelopeCounter))
//                 );

                cluster->step(numSamplesBeat);
                //cluster->postStep();

                //figure out whether to play the cluster
                bool passCluster = false;

                //in the normal case, where cluster is within a range defined by clusterMin and Max
                int sClusterMin = state.params.clusterMinMaxParams.clusterMinParam->getCurrentValue();
                int sClusterMax = state.params.clusterMinMaxParams.clusterMaxParam->getCurrentValue();
                if(sClusterMin <= sClusterMax)
                {
                    if (clusterNotes.size() >= sClusterMin && clusterNotes.size() <= sClusterMax)
                        passCluster = true;
                }
                //the inverse case, where we only play cluster that are *outside* the range set by clusterMin and Max
                else
                {
                    if (clusterNotes.size() >= sClusterMin || clusterNotes.size() <= sClusterMax)
                        passCluster = true;
                }

                playCluster = passCluster;

                if(playCluster)
                {
                    for (int n=0; n < slimCluster.size(); n++)
                    {
                        /**
                         * todo: when the transposition multislider is 2d, need to update here
                         */
                        int newNote = slimCluster[n] + state.params.transpositions.sliderVals[cluster->transpCounter];
                        auto newmsg = juce::MidiMessage::noteOn (1, newNote, clusterVelocities.getUnchecked(slimCluster[n]));
                        if(state.params.sustainLengthMultipliers.sliderVals[cluster->lengthMultiplierCounter] > 0.)
                            outMidiMessages.addEvent(newmsg, 0);
                        else // backwards-playing note
                        {
                            /*
                             * note that the noteOnSpecMap will apply to all other notes == newNote for this block!
                             *  - shouldn't be an issue, unless note playback is very fast or block is very large
                             *      AND we get multiple noteOn msgs in the same block that want different noteOnSpecs
                             */
                            float newNoteDuration = fabs(state.params.sustainLengthMultipliers.sliderVals[cluster->lengthMultiplierCounter] * (60.0 / tempoTemp) * 1000.);
                            noteOnSpecMap[newNote].startDirection = Direction::backward;
                            noteOnSpecMap[newNote].startTime = newNoteDuration;
                            noteOnSpecMap[newNote].stopSameCurrentNote = false;

                            /*
                             * todo: go back to having forwardsSynth and backwardsSynth, to avoid noteOn/Off
                             *          clashes when sequencing forward/backward playing notes in close proximity
                             */
                            outMidiMessages.addEvent(newmsg, 0);
                        }

                        sustainedNotesTimers[newNote] = 0;
                    }
                }

                // step cluster data
                cluster->postStep();
            }

            /*
             * handle turning off notes here
             */
            juce::uint64 noteLength_samples = 0;
            /**
             * todo: handle tempo/general stuff...
             */
//            if (tempoPrep->getTempoSystem() == AdaptiveTempo1)
//            {
//                noteLength = (fabs(prep->sLengthMultipliers.value[cluster->getLengthMultiplierCounter()]) * tempoPrep->getBeatThreshMS());
//            }
//            else
//            {
//                noteLength = (fabs(prep->sLengthMultipliers.value[cluster->getLengthMultiplierCounter()]) * tempoPrep->getBeatThreshMS() / tempoPrep->getSubdivisions());
//            }
//            noteLength = (fabs(prep->sLengthMultipliers.value[cluster->getLengthMultiplierCounter()]) * tempoPrep->getBeatThreshMS() / tempoPrep->getSubdivisions());
            noteLength_samples = fabs(state.params.sustainLengthMultipliers.sliderVals[cluster->lengthMultiplierCounter]) * getSampleRate() * (60.0 / tempoTemp);
            //DBG("noteLength_samples = " + juce::String(noteLength_samples) + " and cluster->getPhasor() = " + juce::String(cluster->getPhasor()));

            for (auto tm = sustainedNotesTimers.begin(); tm != sustainedNotesTimers.end(); /* no increment here */)
            {
                if (tm->second > noteLength_samples)
                {
                    auto newmsg = juce::MidiMessage::noteOff (1, tm->first, clusterVelocities.getUnchecked(tm->first));

                    /**
                     * todo: need to check to make sure that there isn't this same
                     *       note in a different cluster that might then get cut off?
                     */
                    outMidiMessages.addEvent(newmsg, 0);

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

            //pass time until next beat
            cluster->incrementPhasor(numSamples);
            incrementSustainedNotesTimers(numSamples);

        }
    }

    playCluster = play;

    /**
     * finally process actual incoming MIDI messages
     */
    for (auto mi : inMidiMessages)
    {
        auto message = mi.getMessage();

        if(message.isNoteOn())
            keyPressed(message.getNoteNumber(), message.getVelocity(), message.getChannel());
        else if(message.isNoteOff())
            keyReleased(message.getNoteNumber(), message.getChannel());
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
//    processContinuousModulations(buffer);

    // process any mod changes to the multisliders
//    state.params.processStateChanges();

    /*
     * do the MIDI stuff here
     */

    /**
     * ProcessMIDIBlock takes all the input MIDI messages and writes to separate
     *      forwards and backwards midiBuffers to send to those respective synths
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
     * todo: perhaps have a fadeout param, followed by a buffer clear?
     * - these could be user settable, perhaps...
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
        // check to see if we have as many clusters as we are allowed, remove oldest if we are
        //      numClusters in old bK is numLayers
        if (clusters.size() >= state.params.numLayers->getCurrentValue())
        {
            clusters.remove(0); // remove first (oldest) cluster
        }

        // make the new one and add it to the array of clusters
        _cluster = new SynchronicCluster(&state.params);
        clusters.add(_cluster); // add to the array of clusters (what are called "layers" in the UI

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
    doPatternSync = false; // resetting pattern phases
    doBeatSync = false; // resetting beat phase
    doAddNotes = false; // adding notes to cluster
    doPausePlay = false; // targeting pause/play
    doClear = false;
    doDeleteOldest = false;
    doDeleteNewest = false;
    doRotate = false;

    switch(channel + (SynchronicTargetFirst))
    {
        case SynchronicTargetDefault:
            doCluster = true;
            break;

        case SynchronicTargetPatternSync:
            doPatternSync = true;
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
            doPausePlay = true;
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
    /**
     * todo: handle the targeting somewhere, holding these here for now
     */

    handleMidiTargetMessages(channel);

    auto sMode = state.params.pulseTriggeredBy->get();
    auto onOffMode = state.params.determinesCluster->get();

    holdTimers.set(noteNumber, 0);
//    lastKeyPressed = noteNumber;

    // add note to array of depressed notes
    keysDepressed.addIfNotAlreadyThere(noteNumber);

    // is this a new cluster?
    bool isNewCluster = false;

    // always work on the most recent cluster/layer
    SynchronicCluster* cluster = clusters.getLast();

    /*
     * doCluster => default Synchronic behavior
     */
    if (doCluster)
    {
        clusterKeysDepressed.addIfNotAlreadyThere(noteNumber);
        clusterVelocities.set(noteNumber, velocity);

        /**
         * todo: move pausePlay out of here if possible?
         */
        if(!pausePlay)
        {
            // Remove old clusters, deal with layers and NoteOffSync modes
            for (int i = clusters.size(); --i >= 0; )
            {
                if (!clusters[i]->getShouldPlay() && !inCluster)
                {
                    clusters.remove(i);
                    continue;
                }

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
                if(isNewCluster) cluster = clusters.getLast();

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

    // we should have a cluster now, but if not...
    if (cluster == nullptr) return;

    // since it's a new cluster, the next noteOff will be a first noteOff
    // this will be needed for keyReleased(), when in FirstNoteOffSync mode
    if (isNewCluster) nextOffIsFirst = true;


    // ** now trigger behaviors set by Keymap targeting **

    // synchronize beat, if targeting beat sync on noteOn or on both noteOn/Off
    if (doBeatSync)
    {
        /**
         * todo: figure out how to handle the stuff from Tempo, to set beatThresholdSamples
         */
        beatThresholdSamples = getSampleRate() * 60.0 / tempoTemp;

        /**
         * todo: check this
         *          and also add the General Settings and Tempo adjustments
         */
        juce::uint64 phasor = beatThresholdSamples * state.params.beatLengthMultipliers.sliderVals[cluster->beatMultiplierCounter].load();

        // reset beat timing
        cluster->setBeatPhasor(phasor);
    }

    // resetPatternPhase() starts patterns over, if targeting beat sync on noteOn or on both noteOn/Off
    if (doPatternSync)
    {
        cluster->resetPatternPhase();
    }

    // add notes to the cluster, if targeting beat sync on noteOn or on both noteOn/Off
    if (doAddNotes )
    {
        clusterVelocities.set(noteNumber, velocity);
        cluster->addNote(noteNumber);
    }

    /**
     * todo: figure out how best to handle pausePlay and consolidate as much as possible
     */
    // toggle pause/play, if targeting beat sync on noteOn or on both noteOn/Off
    if (doPausePlay)
    {
        if (pausePlay) pausePlay = false;
        else pausePlay = true;
    }

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
    SynchronicCluster* cluster = clusters.getLast();

    /**
     * todo: figure out how to handle the stuff from Tempo, to set beatThresholdSamples
     */
    beatThresholdSamples = getSampleRate() * 60.0 / tempoTemp;

    /*
     * doCluster => default Synchronic behavior
     */
    if (doCluster)
    {
        // remove key from cluster-targeted keys
        clusterKeysDepressed.removeAllInstancesOf(noteNumber);

        if(!pausePlay)
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
                        /**
                         * todo: check this
                         *          and also add the General Settings and Tempo adjustments
                         */
                        juce::uint64 phasor = beatThresholdSamples * state.params.beatLengthMultipliers.sliderVals[cluster->beatMultiplierCounter].load();
                        clusters[i]->setBeatPhasor (phasor);
                    }
                }

                // have done at least one noteOff, so next one will not be first one.
                nextOffIsFirst = false;
            }
        }
    }

    // we should have a cluster now, but if not...
    if (cluster == nullptr) return;

    // ** now trigger behaviors set by Keymap targeting **

    // synchronize beat, if targeting beat sync on noteOff or on both noteOn/Off
    if (doBeatSync)
    {
        //start right away
        /**
         * todo: check this and also add the General Settings and Tempo adjustments
         */
        juce::uint64 phasor = beatThresholdSamples * state.params.beatLengthMultipliers.sliderVals[cluster->beatMultiplierCounter].load();
        cluster->setBeatPhasor(phasor);      // resetBeatPhasor resets beat timing
        cluster->setShouldPlay(true);
    }

    // resetPatternPhase() starts patterns over, if targeting beat sync on noteOff or on both noteOn/Off
    if (doPatternSync)
    {
        cluster->resetPatternPhase();
        cluster->setShouldPlay(true);
    }

    // add notes to the cluster, if targeting beat sync on noteOff or on both noteOn/Off
    if (doAddNotes)
    {
        cluster->addNote(noteNumber);
    }

    // toggle pause/play, if targeting beat sync on noteOff or on both noteOn/Off
    if (doPausePlay)
    {
        if (pausePlay) pausePlay = false;
        else pausePlay = true;
    }

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