//
// Created by Joshua Warner on 6/27/24.
//

#include "TuningProcessor.h"
#include "synth_base.h"

// ************* TuningState ************* //

void TuningState::setKeyOffset (int midiNoteNumber, float val)
{
    if (midiNoteNumber >= 0 && midiNoteNumber < 128)
        absoluteTuningOffset[midiNoteNumber] = val;
}

void TuningState::setCircularKeyOffset (int midiNoteNumber, float val)
{
    if (midiNoteNumber >= 0 && midiNoteNumber < 12)
        circularTuningOffset[midiNoteNumber] = val;
}

void TuningState::setKeyOffset (int midiNoteNumber, float val, bool circular)
{
    if (circular)
        setCircularKeyOffset (midiNoteNumber, val);
    else
        setKeyOffset (midiNoteNumber, val);
}

void TuningState::processStateChanges()
{
    for (auto [index, change] : stateChanges.changeState)
    {
        static juce::var nullVar;
        auto val = change.getProperty (IDs::absoluteTuning);
        auto val1 = change.getProperty (IDs::circularTuning);
        if (val != nullVar)
        {
            absoluteTuningOffset = parseIndexValueStringToArrayAbsolute<128> (val.toString().toStdString());
        }
        else if (val1 != nullVar)
        {
            circularTuningOffset = parseFloatStringToArrayCircular<12> (val1.toString().toStdString());
            // absoluteTuningOffset = std::array<float,128>(val1.toString().toStdString());
        }
    }
}

std::array<float, 12> TuningState::rotateValuesByFundamental (std::array<float, 12> vals, int fundamental)
{
    int offset;
    if (fundamental <= 0)
        offset = 0;
    else
        offset = fundamental;
    std::array<float, 12> new_vals = { 0.f };
    for (int i = 0; i < 12; i++)
    {
        int index = ((i - offset) + 12) % 12;
        new_vals[i] = vals[index];
    }
    return new_vals;
}

void TuningState::setFundamental (int fund)
{
    //need to shift keyValues over by difference in fundamental
    int oldFund = fundamental;
    fundamental = fund;
    int offset = fund - oldFund;
    auto vals = circularTuningOffset;
    for (int i = 0; i < 12; i++)
    {
        int index = ((i - offset) + 12) % 12;
        circularTuningOffset[i] = vals[index];
    }
}

/**
     * helper function for the semitone width fundamental UI elements
     * @return the fundamental in midinote number value, given the octave and pitchclass name (so C4 will return 60)
     */
int TuningState::getSemitoneWidthFundamental()
{
    auto fund = semitoneWidthParams.reffundamental.get()->getIndex();
    auto oct = semitoneWidthParams.octave->getCurrentValueAsText().juce::String::getIntValue();
    return fund + (oct + 1) * 12;
}

double TuningState::getSemitoneWidth()
{
    return semitoneWidthParams.semitoneWidthSliderParam->getCurrentValue();
}

/**
     *
     * @param midiNoteNumber
     * @return new transposition to new midiNoteNumber based on semitone width setting (fractional midi value)
     * if semitone width is 100, then output = 0
     * otherwise the output will be transposed by the return value
     * for example: if the semitone width = 50, the semitone fundamental = 60, and midiNoteNumber = 61, the output will be -0.5
     */
double TuningState::getSemitoneWidthOffsetForMidiNote(double midiNoteNumber)
{
    double offset;
    if (fabs(getSemitoneWidth() - 100.) < 1.) offset = 0.; // return 0 for cases within a cent of 100
    else offset = .01 * (midiNoteNumber - getSemitoneWidthFundamental()) * (getSemitoneWidth() - 100.);
    return offset;
}

/**
     * BKSynth will use this to find the closest sample for a particular note
     *      need something like this to find the best sample for this midiNoteNumber
     *      it may be very far from the original midi key played because of the semitone width variable
     * @param noteNum
     * @param transp
     * @return
     */
int TuningState::getClosestKey(int noteNum, float transp, bool tuneTranspositions)
{
    //first check for when there is no need to adjust for semitone width (which is 99.9% of the time!)
    if (getSemitoneWidthOffsetForMidiNote(noteNum) == 0)
    {
        return (noteNum + transp);
    }

    double workingOffset;
    if (!tuneTranspositions) {
        workingOffset = getSemitoneWidthOffsetForMidiNote (noteNum); // only track semitone width changes for the played note, note the transposition
    }
    else {
        workingOffset = getSemitoneWidthOffsetForMidiNote (noteNum + transp); // track semitone width for transposition as well
    }

    return static_cast<int>(std::round(noteNum + workingOffset + transp));
}

/**
     * Get the tuning offset value, from "offset" slider
     * @return offset in fractional Midi note values
     */
double TuningState::getOverallOffset() { return offSet->getCurrentValue() * 0.01;}

/**
     * update the last frequency and the last interval, for use in the UI
     * @param lastFreq
     */
void TuningState::updateLastFrequency(double lastFreq)
{
    if (lastFreq != lastFrequencyHz) {
        lastIntervalCents = ftom(lastFreq, A4frequency) - ftom(lastFrequencyHz, A4frequency);
        lastMidiNote = ftom(lastFreq, A4frequency);
        lastFrequencyHz = lastFreq;

        lastNote->setParameterValue(lastMidiNote); // will update UI
    }
}

/**
     * getTargetFrequency() is the primary function for synthesizers to handle tuning
     *      should include static and dynamic tunings, and account for semitone width changes
     *      is called every block
     *      return fractional MIDI value, NOT cents
     *
     * @param currentlyPlayingNote
     * @param currentTransposition
     * @param tuneTranspositions
     * @return fractional MIDI value (not cents)
     */
double TuningState::getTargetFrequency (int currentlyPlayingNote, double currentTransposition, bool tuneTranspositions)
{
    //****************************** Spring Tuning Section ******************************//

    /* nothing to see here yet */

    //****************************** Adaptive Tuning Section ******************************//



    //****************************** Static Tuning Section ******************************//
    /**
         * The most common part of this function...
         *
         *
         * Regarding Transpositions (from transposition sliders, for instance):
         *
         * by default, transpositions are tuned literally, relative to the played note
         *      using whatever value, fractional or otherwise, that the user indicates
         *      and ignores the tuning system
         *      The played note is tuned according to the tuning system, but the transpositions are not
         *
         * if "tuneTranspositions" is set to true, then the transposed notes themselves are also tuned
         *      according to the current tuning system
         *
         * this should be the same behavior we had in the original bK, with "use Tuning" on transposition sliders
         *
         * all this becomes quite a bit more complicated when semitone width becomes a parameter and is not necessary 100 cents
         *      and especially so with transpositions (fro Direct, for instance), that might or might not "useTuning"
         *      all of the combination cases are handled separately below, mostly to make it all clearer to follow and debug
         *      (i had a single set of code that handled it all with out the separate cases, but it got very convoluted!)
         *
         */

    // do we really need this check?
    if (circularTuningOffset.empty())
    {
        double newOffset = (currentlyPlayingNote + currentTransposition);
        if (!absoluteTuningOffset.empty()) newOffset += absoluteTuningOffset[currentlyPlayingNote];
        newOffset *= .01;
        newOffset += getOverallOffset();
        return mtof (newOffset + (double) currentlyPlayingNote + currentTransposition, A4frequency);
    }

    // simple case: no transpositions and no semitone width adjustments
    if (currentTransposition == 0 && getSemitoneWidthOffsetForMidiNote(currentlyPlayingNote) == 0.)
    {
        double workingOffset = circularTuningOffset[(currentlyPlayingNote) % circularTuningOffset.size()] * .01;
        workingOffset += absoluteTuningOffset[currentlyPlayingNote] * .01;
        workingOffset += getOverallOffset();
        return mtof (workingOffset + (double) currentlyPlayingNote, A4frequency);
    }

    // next case: transpositions, but no semitone width adjustments
    if (currentTransposition != 0 && getSemitoneWidthOffsetForMidiNote(currentlyPlayingNote + currentTransposition) == 0)
    {
        if (!tuneTranspositions)
        {
            double workingOffset = circularTuningOffset[(currentlyPlayingNote) % circularTuningOffset.size()] * .01;
            workingOffset += absoluteTuningOffset[currentlyPlayingNote] * .01;
            workingOffset += getOverallOffset();
            return mtof (workingOffset + (double) currentlyPlayingNote + currentTransposition, A4frequency);
        }
        else
        {
            double workingOffset = circularTuningOffset[(currentlyPlayingNote + (int)std::round(currentTransposition)) % circularTuningOffset.size()] * .01;
            workingOffset += absoluteTuningOffset[currentlyPlayingNote + (int)std::round(currentTransposition)] * .01;
            workingOffset += getOverallOffset();
            return mtof (workingOffset + (double) currentlyPlayingNote + currentTransposition, A4frequency);
        }
    }

    // next case: semitone width changes, but no transpositions
    if (currentTransposition == 0. && getSemitoneWidthOffsetForMidiNote(currentlyPlayingNote) != 0.)
    {
        double midiNoteAdjustment = getSemitoneWidthOffsetForMidiNote(currentlyPlayingNote);
        int midiNoteNumberTemp = std::round(currentlyPlayingNote + midiNoteAdjustment);
        double workingOffset = (currentlyPlayingNote + midiNoteAdjustment) - midiNoteNumberTemp;
        workingOffset += circularTuningOffset[(midiNoteNumberTemp) % circularTuningOffset.size()] * .01;
        workingOffset += absoluteTuningOffset[midiNoteNumberTemp] * .01;
        workingOffset += getOverallOffset();
        return mtof (workingOffset + midiNoteNumberTemp, A4frequency);
    }

    // final case: semitone width changes AND transpositions
    if (currentTransposition != 0. && getSemitoneWidthOffsetForMidiNote(currentlyPlayingNote + currentTransposition) != 0.)
    {
        if (!tuneTranspositions)
        {
            double workingOffset = getSemitoneWidthOffsetForMidiNote(currentlyPlayingNote); // don't apply the tunings settings to the transpositions; tune transp relative to played note
            int tuningNote = std::round(currentlyPlayingNote + workingOffset);
            int midiNoteNumberTemp = std::round(currentlyPlayingNote + currentTransposition + workingOffset);
            workingOffset += currentlyPlayingNote + currentTransposition - midiNoteNumberTemp; // fractional offset
            workingOffset += circularTuningOffset[tuningNote % circularTuningOffset.size()] * .01;
            workingOffset += absoluteTuningOffset[tuningNote] * .01;
            workingOffset += getOverallOffset();
            return mtof (workingOffset + (double) midiNoteNumberTemp, A4frequency);
        }
        else
        {
            double workingOffset = getSemitoneWidthOffsetForMidiNote(currentlyPlayingNote + currentTransposition); // transposition is also impacted by semitone width in Tuning
            int midiNoteNumberTemp = std::round(currentlyPlayingNote + currentTransposition + workingOffset);
            workingOffset += currentlyPlayingNote + currentTransposition - midiNoteNumberTemp; // fractional offset
            workingOffset += circularTuningOffset[(midiNoteNumberTemp) % circularTuningOffset.size()] * .01;
            workingOffset += absoluteTuningOffset[midiNoteNumberTemp] * .01;
            workingOffset += getOverallOffset();
            return mtof (workingOffset + (double) midiNoteNumberTemp, A4frequency);
        }
    }

    DBG("should never reach this point!");
    jassert(true);

    /**
         * to add here:
         * - need to get A4frequency from gallery preferences
         *
         * - adaptive tunings 1 and 2
         * - spring tuning
         */
}

// ************* Tuning Processor ************* //

TuningProcessor::TuningProcessor (SynthBase& parent, const juce::ValueTree& v) : PluginBase (parent, v, nullptr, tuningBusLayout())
{
    parent.getStateBank().addParam (std::make_pair<std::string, bitklavier::ParameterChangeBuffer*> (v.getProperty (IDs::uuid).toString().toStdString() + "_" + "absoluteTuning", &(state.params.tuningState.stateChanges)));
    parent.getStateBank().addParam (std::make_pair<std::string, bitklavier::ParameterChangeBuffer*> (v.getProperty (IDs::uuid).toString().toStdString() + "_" + "circularTuning", &(state.params.tuningState.stateChanges)));
}

void TuningProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const auto spec = juce::dsp::ProcessSpec { sampleRate, (uint32_t) samplesPerBlock, (uint32_t) getMainBusNumInputChannels() };

    gain.prepare (spec);
    gain.setRampDurationSeconds (0.05);
}

void TuningProcessor::handleMidiEvent (const juce::MidiMessage& m)
{
    if (m.isNoteOn())
    {
        DBG("Tuning Processor Note On " + juce::String(m.getNoteNumber()) + " " + juce::String(m.getVelocity()));
    }
    else if (m.isNoteOff())
    {
        DBG("Tuning Processor Note Off " + juce::String(m.getNoteNumber()) + " " + juce::String(m.getVelocity()));
    }
}

void TuningProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    //DBG(juce::String(getState().params.tuningState.getOverallOffset()));
    auto midiIterator = midiMessages.findNextSamplePosition (0);

    std::for_each(midiIterator,midiMessages.cend(),
        [&] (const juce::MidiMessageMetadata& meta)
        {
            handleMidiEvent (meta.getMessage());
        });
}

template <typename Serializer>
typename Serializer::SerializedType TuningParams::serialize (const TuningParams& paramHolder)
{
    /*
     * first, call the default serializer, which gets all the simple params
     */
    auto ser = chowdsp::ParamHolder::serialize<Serializer> (paramHolder);

    /*
     * then serialize the more complex params
     */
    Serializer::template addChildElement<12> (ser, "circularTuning", paramHolder.tuningState.circularTuningOffset, arrayToString);
    Serializer::template addChildElement<128> (ser, "absoluteTuning", paramHolder.tuningState.absoluteTuningOffset, arrayToStringWithIndex);

    return ser;
}

template <typename Serializer>
void TuningParams::deserialize (typename Serializer::DeserializedType deserial, TuningParams& paramHolder)
{
    chowdsp::ParamHolder::deserialize<Serializer> (deserial, paramHolder);
    auto myStr = deserial->getStringAttribute ("circularTuning");
    paramHolder.tuningState.circularTuningOffset = parseFloatStringToArrayCircular<12> (myStr.toStdString());
    myStr = deserial->getStringAttribute ("absoluteTuning");
    paramHolder.tuningState.absoluteTuningOffset = parseIndexValueStringToArrayAbsolute<128> (myStr.toStdString());
    paramHolder.tuningState.fundamental = paramHolder.fundamental->getIndex();
}
