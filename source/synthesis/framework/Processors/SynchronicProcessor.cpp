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

    for (int i = 0; i < 128; i++)
    {
        holdTimers.add(0);
    }

    clusterKeysDepressed = juce::Array<int>();
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
    // don't do anything if we are paused!
    if (pausePlay) return;

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

bool SynchronicProcessor::holdCheck(int noteNumber)
{
    juce::uint64 hold = holdTimers.getUnchecked(noteNumber) * (1000.0 / getSampleRate());
    DBG("holdCheck val = " + juce::String(holdTimers.getUnchecked(noteNumber)));

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

void SynchronicProcessor::keyPressed(int noteNumber)
{
    state.params;

    holdTimers.set(noteNumber, 0);
    lastKeyPressed = noteNumber;

//    bool doCluster = bVels->getUnchecked(TargetTypeSynchronic) >= 0.f; // primary Synchronic mode
//    bool doPatternSync = bVels->getUnchecked(TargetTypeSynchronicPatternSync) >= 0.f; // resetting pattern phases
//    bool doBeatSync = bVels->getUnchecked(TargetTypeSynchronicBeatSync) >= 0.f; // resetting beat phase
//    bool doAddNotes = bVels->getUnchecked(TargetTypeSynchronicAddNotes) >= 0.f; // adding notes to cluster
//    bool doPausePlay = bVels->getUnchecked(TargetTypeSynchronicPausePlay) >= 0.f; // targeting pause/play
//    bool doClear = bVels->getUnchecked(TargetTypeSynchronicClear) >= 0.f;
//    bool doDeleteOldest = bVels->getUnchecked(TargetTypeSynchronicDeleteOldest) >= 0.f;
//    bool doDeleteNewest = bVels->getUnchecked(TargetTypeSynchronicDeleteNewest) >= 0.f;
//    bool doRotate = bVels->getUnchecked(TargetTypeSynchronicRotate) >= 0.f;

    /**
     * todo: handle the targeting somewhere, holding these here for now
     */
    bool doCluster = true; // primary Synchronic mode
    bool doPatternSync = true; // resetting pattern phases
    bool doBeatSync = true; // resetting beat phase
    bool doAddNotes = true; // adding notes to cluster
    bool doPausePlay = true; // targeting pause/play
    bool doClear = true;
    bool doDeleteOldest = true;
    bool doDeleteNewest = true;
    bool doRotate = true;

    // add note to array of depressed notes
    keysDepressed.addIfNotAlreadyThere(noteNumber);

    /**
     * todo: do we need this?
     */
//    if (doCluster || doAddNotes)
//    {
//        float v = jmax(aVels->getUnchecked(TargetTypeSynchronic),
//            aVels->getUnchecked(TargetTypeSynchronicAddNotes));
//        clusterVelocities.set(noteNumber, v);
//    }

    // track the note's target, as set in Keymap
    /*
    Keymap Target modes:
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

    // is this a new cluster?
    bool isNewCluster = false;

    // add note to clusterKeysDepressed (keys targeting the main synchronic functionality)
    if (doCluster) clusterKeysDepressed.addIfNotAlreadyThere(noteNumber);

    // always work on the most recent cluster/layer
    SynchronicCluster* cluster = clusters.getLast();

    auto sMode = state.params.pulseTriggeredBy->get();
    auto onOffMode = state.params.determinesCluster->get();

    // do only if this note is targeted as a primary Synchronic note (TargetTypeSynchronic)
    if (doCluster && !pausePlay)
    {

        // Remove old clusters, deal with layers and NoteOffSync modes
        for (int i = clusters.size(); --i >= 0; )
        {
            if (!clusters[i]->getShouldPlay() && !inCluster)
            {
                clusters.remove(i);
                continue;
            }

            if(   (sMode == SynchronicPulseTriggerType::Last_NoteOff)
                || (sMode == Any_NoteOff))
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
        if (onOffMode == SynchronicClusterTriggerType::Key_On) // onOffMode.value is set by the "determines cluster"
        {
            // if we have a new cluster
            if (!inCluster || cluster == nullptr)
            {
                // check to see if we have as many clusters as we are allowed, remove oldest if we are

                //numClusters in old bK is numLayers
//                if (clusters.size() >= synchronic->prep->numClusters.value)
                if (clusters.size() >= state.params.numLayers->getCurrentValue())
                {
                    clusters.remove(0); // remove first (oldest) cluster
                }

                // make the new one and add it to the array of clusters
                cluster = new SynchronicCluster(&state.params);
                clusters.add(cluster); // add to the array of clusters (what are called "layers" in the UI

                // this is a new cluster!
                isNewCluster = true;
            }

            // add this played note to the cluster
            cluster->addNote(noteNumber);

            // yep, we are in a cluster!
            inCluster = true;

            // reset the timer for time between notes; we do this for every note added to a cluster
            clusterThresholdTimer = 0;

            // reset the beat phase and pattern phase, and start playing, depending on the mode
            if (sMode == SynchronicPulseTriggerType::Any_NoteOn)
            {
                cluster->setShouldPlay(true);
                cluster->setBeatPhasor(0);
                cluster->resetPatternPhase();
            }
            else if (sMode == SynchronicPulseTriggerType::First_NoteOn)
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

    // we should have a cluster now, but if not...
    if (cluster == nullptr) return;

    // since it's a new cluster, the next noteOff will be a first noteOff
    // this will be needed for keyReleased(), when in FirstNoteOffSync mode
    if (isNewCluster) nextOffIsFirst = true;


    // ** now trigger behaviors set by Keymap targeting **
    //
    // synchronize beat, if targeting beat sync on noteOn or on both noteOn/Off
//    if (doBeatSync && (prep->getTargetTypeSynchronicBeatSync() == NoteOn || prep->getTargetTypeSynchronicBeatSync() == Both))
    if (doBeatSync)
    {
//        TempoPreparation::Ptr tempoPrep = tempo->getTempo()->prep;

/**
 * todo: figure out how to handle the stuff from Tempo, to set beatThresholdSamples
 */

//        if (tempoPrep->getTempoSystem() == AdaptiveTempo1)
//        {
//            beatThresholdSamples = (tempoPrep->getBeatThresh() * synth->getSampleRate());
//        }
//        else
//        {
//            beatThresholdSamples = (tempoPrep->getBeatThresh() / tempoPrep->getSubdivisions() * synth->getSampleRate());
//        }

        beatThresholdSamples = getSampleRate() * 60.0 / tempoTemp;

        //start right away
//        juce::uint64 phasor = beatThresholdSamples *
//                        synchronic->prep->sBeatMultipliers.value[cluster->getBeatMultiplierCounter()] *
//                        general->getPeriodMultiplier() *
//                        tempo->getPeriodMultiplier();

        /**
         * todo: check this
         *          and also add the General Settings and Tempo adjustments
         */

        juce::uint64 phasor = beatThresholdSamples * state.params.beatLengthMultipliers.sliderVals[cluster->beatMultiplierCounter].load();

        cluster->setBeatPhasor(phasor);      // resetBeatPhasor resets beat timing
    }

    // resetPatternPhase() starts patterns over, if targeting beat sync on noteOn or on both noteOn/Off
//    if (doPatternSync && (prep->getTargetTypeSynchronicPatternSync() == NoteOn || prep->getTargetTypeSynchronicPatternSync() == Both))   cluster->resetPatternPhase();
    if (doPatternSync)   cluster->resetPatternPhase();

    // add notes to the cluster, if targeting beat sync on noteOn or on both noteOn/Off
//    if (doAddNotes && (prep->getTargetTypeSynchronicAddNotes() == NoteOn || prep->getTargetTypeSynchronicAddNotes() == Both))      cluster->addNote(noteNumber);
    if (doAddNotes )      cluster->addNote(noteNumber);

    // toggle pause/play, if targeting beat sync on noteOn or on both noteOn/Off
//    if (doPausePlay && (prep->getTargetTypeSynchronicPausePlay() == NoteOn || prep->getTargetTypeSynchronicPausePlay() == Both))
    if (doPausePlay)
    {
        if (pausePlay) pausePlay = false;
        else pausePlay = true;
    }

//    if (doClear && (prep->getTargetTypeSynchronicClear() == NoteOn || prep->getTargetTypeSynchronicClear() == Both))
    if (doClear)
    {
        clusters.clear();
    }

//    if (doDeleteOldest && (prep->getTargetTypeSynchronicDeleteOldest() == NoteOn || prep->getTargetTypeSynchronicDeleteOldest() == Both))
    if (doDeleteOldest)
    {
        if (!clusters.isEmpty()) clusters.remove(0);
    }

//    if (doDeleteNewest && (prep->getTargetTypeSynchronicDeleteNewest() == NoteOn || prep->getTargetTypeSynchronicDeleteNewest() == Both))
    if (doDeleteNewest)
    {
        if (!clusters.isEmpty()) clusters.remove(clusters.size() - 1);
    }

//    if (doRotate && (prep->getTargetTypeSynchronicRotate() == NoteOn || prep->getTargetTypeSynchronicRotate() == Both))
    if (doRotate )
    {
        if (!clusters.isEmpty())
        {
            clusters.move(clusters.size() - 1, 0);
        }
    }
}

void SynchronicProcessor::keyReleased(int noteNumber)
{
    SynchronicPreparation::Ptr prep = synchronic->prep;

    // aVels will be used for velocity calculations; bVels will be used for conditionals
    Array<float> *aVels, *bVels;
    // If this is an inverted key press, aVels and bVels are the same
    // We'll save and use the incoming velocity values
    if (fromPress)
    {
        aVels = bVels = &invertVelocities.getReference(noteNumber);
        for (int i = TargetTypeSynchronic; i <= TargetTypeSynchronicRotate; ++i)
        {
            aVels->setUnchecked(i, targetVelocities.getUnchecked(i));
        }
    }
    // If this an actual release, aVels will be the incoming velocities,
    // but bVels will use the values from the last press (keyReleased with fromPress=true)
    else
    {
        aVels = &targetVelocities;
        bVels = &velocities.getReference(noteNumber);
    }

    bool doCluster = bVels->getUnchecked(TargetTypeSynchronic) >= 0.f; // primary Synchronic mode
    bool doPatternSync = bVels->getUnchecked(TargetTypeSynchronicPatternSync) >= 0.f; // resetting pattern phases
    bool doBeatSync = bVels->getUnchecked(TargetTypeSynchronicBeatSync) >= 0.f; // resetting beat phase
    bool doAddNotes = bVels->getUnchecked(TargetTypeSynchronicAddNotes) >= 0.f; // adding notes to cluster
    bool doPausePlay = bVels->getUnchecked(TargetTypeSynchronicPausePlay) >= 0.f; // targeting pause/play
    bool doClear = bVels->getUnchecked(TargetTypeSynchronicClear) >= 0.f;
    bool doDeleteOldest = bVels->getUnchecked(TargetTypeSynchronicDeleteOldest) >= 0.f;
    bool doDeleteNewest = bVels->getUnchecked(TargetTypeSynchronicDeleteNewest) >= 0.f;
    bool doRotate = bVels->getUnchecked(TargetTypeSynchronicRotate) >= 0.f;

    // remove key from array of pressed keys
    keysDepressed.removeAllInstancesOf(noteNumber);

    // is this a new cluster?
    bool isNewCluster = false;

    // remove key from cluster-targeted keys
    if (doCluster) clusterKeysDepressed.removeAllInstancesOf(noteNumber);

    // do hold-time filtering (how long the key was held down)
    if (!holdCheck(noteNumber)) return;

    // always work on the most recent cluster/layer
    SynchronicCluster::Ptr cluster = clusters.getLast();

    TempoPreparation::Ptr tempoPrep = tempo->getTempo()->prep;
    if (tempoPrep->getTempoSystem() == AdaptiveTempo1)
    {
        beatThresholdSamples = (tempoPrep->getBeatThresh() * synth->getSampleRate());
    }
    else
    {
        beatThresholdSamples = (tempoPrep->getBeatThresh() / tempoPrep->getSubdivisions() * synth->getSampleRate());
    }

    // do only if this note is targeted as a primary Synchronic note (TargetTypeSynchronic)
    if (doCluster && !pausePlay)
    {
        // cluster management
        // OnOffMode determines whether the timing of keyOffs or keyOns determine whether notes are within the cluster threshold
        // in this case, we only want to do these things when keyOffs set the clusters
        if (synchronic->prep->onOffMode.value == KeyOff) // set in the "determines cluster" menu
        {
            if (!inCluster || cluster == nullptr)
            {
                if (clusters.size() >= synchronic->prep->numClusters.value)
                {
                    // remove first (oldest) cluster
                    clusters.remove(0);
                }

                // make the new cluster, add it to the array of clusters
                cluster = new SynchronicCluster(prep);
                clusters.add(cluster);

                // this is a new cluster!
                isNewCluster = true;
            }

            // add the note to the cluster
            cluster->addNote(noteNumber);

            // yes, we are in a cluster
            inCluster = true;

            // if it's a new cluster, the next noteOff will be a first noteOff
            // this will be needed for FirstNoteOffSync mode
            if (isNewCluster) nextOffIsFirst = true;

            // reset the timer for time between notes
            clusterThresholdTimer = 0;
        }

        // depending on the mode, and whether this is a first or last note, reset the beat and pattern phase and start playing
        if ((synchronic->prep->sMode.value == FirstNoteOffSync && nextOffIsFirst) ||
            (synchronic->prep->sMode.value == AnyNoteOffSync) ||
            (synchronic->prep->sMode.value == LastNoteOffSync && clusterKeysDepressed.size() == 0))
        {
            for (int i = clusters.size(); --i >= 0; )
            {
                if(clusters[i]->containsNote(noteNumber))
                {
                    clusters[i]->resetPatternPhase();
                    clusters[i]->setShouldPlay(true);

                    //start right away
                    uint64 phasor = beatThresholdSamples *
                                    synchronic->prep->sBeatMultipliers.value[cluster->getBeatMultiplierCounter()] *
                                    general->getPeriodMultiplier() *
                                    tempo->getPeriodMultiplier();

                    clusters[i]->setBeatPhasor(phasor);
                }
            }

            // have done at least one noteOff, so next one will not be first one.
            nextOffIsFirst = false;
        }
    }

    // we should have a cluster now, but if not...
    if (cluster == nullptr) return;

    // ** now trigger behaviors set by Keymap targeting **
    //
    // synchronize beat, if targeting beat sync on noteOff or on both noteOn/Off
    if (doBeatSync && (prep->getTargetTypeSynchronicBeatSync() == NoteOff || prep->getTargetTypeSynchronicBeatSync() == Both))
    {
        //start right away
        uint64 phasor = beatThresholdSamples *
                        synchronic->prep->sBeatMultipliers.value[cluster->getBeatMultiplierCounter()] *
                        general->getPeriodMultiplier() *
                        tempo->getPeriodMultiplier();

        cluster->setBeatPhasor(phasor);      // resetBeatPhasor resets beat timing
        cluster->setShouldPlay(true);
    }

    // resetPatternPhase() starts patterns over, if targeting beat sync on noteOff or on both noteOn/Off
    if (doPatternSync && (prep->getTargetTypeSynchronicPatternSync() == NoteOff || prep->getTargetTypeSynchronicPatternSync() == Both))
    {
        cluster->resetPatternPhase();
        cluster->setShouldPlay(true);
    }

    // add notes to the cluster, if targeting beat sync on noteOff or on both noteOn/Off
    if (doAddNotes && (prep->getTargetTypeSynchronicAddNotes() == NoteOff || prep->getTargetTypeSynchronicAddNotes() == Both))      cluster->addNote(noteNumber);

    // toggle pause/play, if targeting beat sync on noteOff or on both noteOn/Off
    if (doPausePlay && (prep->getTargetTypeSynchronicPausePlay() == NoteOff || prep->getTargetTypeSynchronicPausePlay() == Both))
    {
        if (pausePlay) pausePlay = false;
        else pausePlay = true;
    }

    if (doClear && (prep->getTargetTypeSynchronicClear() == NoteOff || prep->getTargetTypeSynchronicClear() == Both))
    {
        clusters.clear();
    }

    if (doDeleteOldest && (prep->getTargetTypeSynchronicDeleteOldest() == NoteOff || prep->getTargetTypeSynchronicDeleteOldest() == Both))
    {
        if (!clusters.isEmpty()) clusters.remove(0);
    }

    if (doDeleteNewest && (prep->getTargetTypeSynchronicDeleteNewest() == NoteOff || prep->getTargetTypeSynchronicDeleteNewest() == Both))
    {
        if (!clusters.isEmpty()) clusters.remove(clusters.size() - 1);
    }

    if (doRotate && (prep->getTargetTypeSynchronicRotate() == NoteOff || prep->getTargetTypeSynchronicRotate() == Both))
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