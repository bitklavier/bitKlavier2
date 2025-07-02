//
// Created by Joshua Warner on 6/27/24.
//

#include "TuningProcessor.h"
#include "synth_base.h"

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
        return mtof (newOffset + (double) currentlyPlayingNote + currentTransposition) * A4frequency / 440.;
    }

    // simple case: no transpositions and no semitone width adjustments
    if (currentTransposition == 0 && getSemitoneWidthOffsetForMidiNote(currentlyPlayingNote) == 0.)
    {
        double workingOffset = circularTuningOffset[(currentlyPlayingNote) % circularTuningOffset.size()] * .01;
        workingOffset += absoluteTuningOffset[currentlyPlayingNote] * .01;
        return mtof (workingOffset + (double) currentlyPlayingNote) * A4frequency / 440.;
    }

    // next case: transpositions, but no semitone width adjustments
    if (currentTransposition != 0 && getSemitoneWidthOffsetForMidiNote(currentlyPlayingNote + currentTransposition) == 0)
    {
        if (!tuneTranspositions)
        {
            double workingOffset = circularTuningOffset[(currentlyPlayingNote) % circularTuningOffset.size()] * .01;
            workingOffset += absoluteTuningOffset[currentlyPlayingNote] * .01;
            return mtof (workingOffset + (double) currentlyPlayingNote + currentTransposition) * A4frequency / 440.;
        }
        else
        {
            double workingOffset = circularTuningOffset[(currentlyPlayingNote + (int)std::round(currentTransposition)) % circularTuningOffset.size()] * .01;
            workingOffset += absoluteTuningOffset[currentlyPlayingNote + (int)std::round(currentTransposition)] * .01;
            return mtof (workingOffset + (double) currentlyPlayingNote + currentTransposition) * A4frequency / 440.;
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
        return mtof (workingOffset + midiNoteNumberTemp) * A4frequency / 440.;
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

            return mtof (workingOffset + (double) midiNoteNumberTemp) * A4frequency / 440.;
        }
        else
        {
            double workingOffset = getSemitoneWidthOffsetForMidiNote(currentlyPlayingNote + currentTransposition); // transposition is also impacted by semitone width in Tuning
            int midiNoteNumberTemp = std::round(currentlyPlayingNote + currentTransposition + workingOffset);
            workingOffset += currentlyPlayingNote + currentTransposition - midiNoteNumberTemp; // fractional offset
            workingOffset += circularTuningOffset[(midiNoteNumberTemp) % circularTuningOffset.size()] * .01;
            workingOffset += absoluteTuningOffset[midiNoteNumberTemp] * .01;

            return mtof (workingOffset + (double) midiNoteNumberTemp) * A4frequency / 440.;
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

void TuningProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    //DBG(juce::String(getState().params.tuningState.getOffset()));
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
