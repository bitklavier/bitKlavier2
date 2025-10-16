//
// Created by Davis Polito on 5/2/24.
//

#include "NostalgicProcessor.h"
#include "Synthesiser/Sample.h"
#include "common.h"
#include "synth_base.h"

NostalgicProcessor::NostalgicProcessor (SynthBase& parent, const juce::ValueTree& vt)
    : PluginBase (parent, vt, nullptr, nostalgicBusLayout()),
    nostalgicSynth (new BKSynthesiser (state.params.reverseEnv, state.params.noteOnGain))

{
    for (int i = 0; i < 300; i++)
    {
        nostalgicSynth->addVoice (new BKSamplerVoice());
    }

    for (int i = 0; i < MaxMidiNotes; ++i)
    {
        noteOnSpecMap[i] = NoteOnSpec{};
    }

    updatedTransps.ensureStorageAllocated(50);
    velocities.ensureStorageAllocated(128);
    noteLengthTimers.ensureStorageAllocated(128);
    reverseTimers.ensureStorageAllocated(500);

    velocities.insertMultiple(0,0,128);
    noteLengthTimers.insertMultiple(0,0,128);
    noteLengthTimers.insertMultiple(0,0,500);

    state.params.transpose.stateChanges.defaultState = v.getOrCreateChildWithName(IDs::PARAM_DEFAULT,nullptr);
    state.params.waveDistUndertowParams.stateChanges.defaultState = v.getOrCreateChildWithName(IDs::PARAM_DEFAULT,nullptr);

    //add state change params here; this will add this to the set of params that are exposed to the state change mod system
    // not needed for audio-rate modulatable params
    parent.getStateBank().addParam (std::make_pair<std::string,
        bitklavier::ParameterChangeBuffer*> (v.getProperty (IDs::uuid).toString().toStdString() + "_" + "transpose",
        &(state.params.transpose.stateChanges)));
    state.params.holdTimeMinMaxParams.stateChanges.defaultState = v.getOrCreateChildWithName(IDs::PARAM_DEFAULT,nullptr);
    parent.getStateBank().addParam (std::make_pair<std::string,
        bitklavier::ParameterChangeBuffer*> (v.getProperty (IDs::uuid).toString().toStdString() + "_" + "holdtime_min_max",
        &(state.params.holdTimeMinMaxParams.stateChanges)));
    for (int i = 0; i < 128; i++){
        holdTimers.add(0);
    }
}

void NostalgicProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const auto spec = juce::dsp::ProcessSpec { sampleRate, (uint32_t) samplesPerBlock, (uint32_t) getMainBusNumInputChannels() };
    nostalgicSynth->setCurrentPlaybackSampleRate (sampleRate);
}

bool NostalgicProcessor::isBusesLayoutSupported (const juce::AudioProcessor::BusesLayout& layouts) const
{
    return true;
}

/*
 * grabs all the TransposeParams values and compiles them into a single array
 * the first slider is always represented, so we always have at least on value to return
 *
 * these operate at the synthesizer level, not the voice level, so need to be passed here
 * and not just looked at by individual voices in the synth
 *
 * this is all pretty inefficient, making copies of copies, but also very small arrays, so....
 */

void NostalgicProcessor::setTuning (TuningProcessor* tun)
{
    tuning = tun;
    nostalgicSynth->setTuning (&tuning->getState().params.tuningState);

}

void NostalgicProcessor::processContinuousModulations(juce::AudioBuffer<float>& buffer)
{
}

void NostalgicProcessor::updateMidiNoteTranspositions(int noteOnNumber)
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
 * - since Nostalgic uses the same transpositions, we just have them all set to the same values
 * - this is different in other preps like Resonance, where individual noteOn msgs will have their own transpositions
 */
void NostalgicProcessor::updateAllMidiNoteTranspositions()
{
    for (int i=0; i<MaxMidiNotes; i++)
    {
        updateMidiNoteTranspositions(i);
    }
}

void NostalgicProcessor::ProcessMIDIBlock(juce::MidiBuffer& inMidiMessages, juce::MidiBuffer& outMidiMessages, int numSamples)
{
    // increment the timers by number of samples in the MidiBuffer
    incrementTimers (numSamples);

    // check if any of the reverse timers have exceeded their target
    for (int i = reverseTimers.size() - 1; i >= 0; --i)
    {
        auto& reverseNote = reverseTimers.getReference(i);
        if (reverseNote.reverseTimerSamples > reverseNote.reverseDurationTargetSamples)
        {
            // play the undertow for that note
            if (reverseNote.undertowDurationMs > 0)
            {
                auto note = reverseNote.noteNumber;
                // noteOnSpecMap[note].clear();
                noteOnSpecMap[note].keyState = true;
                noteOnSpecMap[note].stopSameCurrentNote = false;
                noteOnSpecMap[note].startDirection = Direction::forward;
                noteOnSpecMap[note].startTime = reverseNote.waveDistanceMs;
                noteOnSpecMap[note].sustainTime = reverseNote.undertowDurationMs;
                noteOnSpecMap[note].envParams.attack = state.params.undertowEnv.attackParam->getCurrentValue() * .001; // BKADSR expects seconds, not ms
                noteOnSpecMap[note].envParams.decay = state.params.undertowEnv.decayParam->getCurrentValue() * .001;
                noteOnSpecMap[note].envParams.sustain = state.params.undertowEnv.sustainParam->getCurrentValue();
                noteOnSpecMap[note].envParams.release = state.params.undertowEnv.releaseParam->getCurrentValue() * .001;

                auto forwardOnMsg = juce::MidiMessage::noteOn (1, note, velocities[note]);
                outMidiMessages.addEvent(forwardOnMsg, 0);
            }
            // clean up
            reverseTimers.remove(i);
        }
    }

    // go through the incoming MIDI messages
    for (auto mi : inMidiMessages)
    {
        auto message = mi.getMessage();

        // TODO: we can make it an option for noteOn velocity to set noteOff velocity
        // store the velocity from the note on message
        if (message.isNoteOn ())
        {
            DBG("received MIDI note on");
            velocities.set(message.getNoteNumber(), message.getVelocity());
            noteLengthTimers.set(message.getNoteNumber(), message.getTimeStamp());
        }
        
        // if there's a note off message, play the associated reverse note
        if(message.isNoteOff())
        {
            int note = message.getNoteNumber();
            auto noteDurationSamples = noteLengthTimers[note] * state.params.noteLengthMultParam->getCurrentValue();
            auto noteDurationMs = noteDurationSamples * (1000.0 / getSampleRate());
            auto newNoteStart = noteDurationMs  + state.params.waveDistUndertowParams.waveDistanceParam->getCurrentValue();

            noteOnSpecMap[note].keyState = true;
            noteOnSpecMap[note].stopSameCurrentNote = false;
            noteOnSpecMap[note].startDirection = Direction::backward;
            noteOnSpecMap[note].startTime = newNoteStart;
            noteOnSpecMap[note].sustainTime = noteDurationMs;
            // noteOnSpecMap[note].loopMode = LoopMode::pingpong;
            noteOnSpecMap[note].envParams.attack = state.params.reverseEnv.attackParam->getCurrentValue() * .001; // BKADSR expects seconds, not ms
            noteOnSpecMap[note].envParams.decay = state.params.reverseEnv.decayParam->getCurrentValue() * .001;
            noteOnSpecMap[note].envParams.sustain = state.params.reverseEnv.sustainParam->getCurrentValue();
            noteOnSpecMap[note].envParams.release = state.params.reverseEnv.releaseParam->getCurrentValue() * .001;

            // we want to keep track of how long the reverse note is playing
            reverseTimers.add({note, 0, noteDurationSamples,
                state.params.waveDistUndertowParams.undertowParam->getCurrentValue(),
                state.params.waveDistUndertowParams.waveDistanceParam->getCurrentValue()});

            // play the reverse note
            auto reverseOnMsg = juce::MidiMessage::noteOn (1, note, velocities[note]);
            outMidiMessages.addEvent(reverseOnMsg, 0);

            // cleanup
            noteLengthTimers.set(note, 0.0f);
        }
    }

    // check the hold time, if it falls out of specified range, return
    // if (!holdCheck(noteNumber)) return;

    // figure out if notes are in a cluster
    // if cluster size < cluster min, return

    // get transposed sample if needed

    // figure out where in the sample we start the reverse note
        // current spot * note length multiplier + wave distance
        // play reverse with reverse envelope

    // once reverse note ends, play forward until undertow with env

}

void NostalgicProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    buffer.clear();
    // start with a clean slate of noteOn specifications; assuming normal noteOns without anything special
    for (auto& spec : noteOnSpecMap)
    {
        spec.clear();
    }
    state.getParameterListeners().callAudioThreadBroadcasters();
    processContinuousModulations(buffer);
    updateAllMidiNoteTranspositions();
    state.params.processStateChanges();
    int numSamples = buffer.getNumSamples();
    juce::MidiBuffer outMidi;
    ProcessMIDIBlock(midiMessages, outMidi, numSamples);
    bool useTuningForTranspositions = state.params.transpositionUsesTuning->get();
    if (nostalgicSynth->hasSamples())
    {
        nostalgicSynth->setBypassed (false);
        // nostalgicSynth->updateMidiNoteTranspositions (updatedTransps, useTuningForTranspositions);
        nostalgicSynth->setNoteOnSpecMap(noteOnSpecMap);
        nostalgicSynth->renderNextBlock (buffer, outMidi, 0, buffer.getNumSamples());
    }
    // handle the send
    int sendBufferIndex = getChannelIndexInProcessBlockBuffer (false, 2, 0);
    auto sendgainmult = bitklavier::utils::dbToMagnitude (state.params.outputSendGain->getCurrentValue());
    buffer.copyFrom(sendBufferIndex, 0, buffer.getReadPointer(0), numSamples, sendgainmult);
    buffer.copyFrom(sendBufferIndex+1, 0, buffer.getReadPointer(1), numSamples, sendgainmult);

    // final output gain stage, from rightmost slider in DirectParametersView
    auto outputgainmult = bitklavier::utils::dbToMagnitude (state.params.outputGain->getCurrentValue());
    buffer.applyGain(0, 0, numSamples, outputgainmult);
    buffer.applyGain(1, 0, numSamples, outputgainmult);

    // send level meter update
    std::get<0> (state.params.sendLevels) = buffer.getRMSLevel (sendBufferIndex, 0, numSamples);
    std::get<1> (state.params.sendLevels) = buffer.getRMSLevel (sendBufferIndex+1, 0, numSamples);

    // main level meter update
    std::get<0> (state.params.outputLevels) = buffer.getRMSLevel (0, 0, numSamples);
    std::get<1> (state.params.outputLevels) = buffer.getRMSLevel (1, 0, numSamples);

}


void NostalgicProcessor::processBlockBypassed (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
}

bool NostalgicProcessor::holdCheck(int noteNumber)
{
    juce::uint64 hold = holdTimers.getUnchecked(noteNumber) * (1000.0 / getSampleRate());

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

//increment timers for all active notes, and all currently reversing notes
void NostalgicProcessor::incrementTimers(int numSamples)
{

    // increment the timers by number of samples in the MidiBuffer
    for (int i = 0; i < noteLengthTimers.size(); i++)
    {
        if (noteLengthTimers[i] > 0) noteLengthTimers.set(i, noteLengthTimers[i] + numSamples);;
    }
    for (auto& timer : reverseTimers) // auto& = reference
    {
        timer.reverseTimerSamples += numSamples;
    }
    // for(int i = 0; i<undertowNotes.size(); i++)
    // {
    //     undertowNotes.getUnchecked(i)->incrementUndertowTimer(numSamples);
    // }
}
