//
// Created by Davis Polito on 5/2/24.
//

#include "TempoProcessor.h"
#include "Synthesiser/Sample.h"
#include "common.h"
#include "synth_base.h"

TempoProcessor::TempoProcessor (SynthBase& parent, const juce::ValueTree& vt,juce::UndoManager* um) : PluginBase (parent, vt, um, tempoBusLayout())
{
    state.params.timeWindowMinMaxParams.stateChanges.defaultState = v.getOrCreateChildWithName(IDs::PARAM_DEFAULT,nullptr);
    parent.getStateBank().addParam (std::make_pair<std::string,
        bitklavier::ParameterChangeBuffer*> (v.getProperty (IDs::uuid).toString().toStdString() + "_" + "timewindowminmax",
        &(state.params.timeWindowMinMaxParams.stateChanges)));

    atTimer = 0;
    atLastTime = 0;
    atDelta = 0;
    atHistoryWriteIndex = 0;
    adaptiveTempoPeriodMultiplier = 1.;
    atDeltaHistory.fill(60000. / *state.params.tempoParam);
}

void TempoProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    setRateAndBufferSizeDetails (sampleRate, samplesPerBlock);
}

bool TempoProcessor::isBusesLayoutSupported (const juce::AudioProcessor::BusesLayout& layouts) const
{
    return true;
}

void TempoProcessor::ProcessMIDIBlock(juce::MidiBuffer& inMidiMessages, int numSamples)
{
    atTimer += numSamples;

    /*
     * process incoming MIDI messages, including the target messages
     */
    for (auto mi : inMidiMessages)
    {
        auto message = mi.getMessage();
        const int channel = message.getChannel();

        if (channel + (TempoTargetFirst) == TempoTargetModReset)
        {
            resetContinuousModulations();
            //resetStateModulations(); // no state mods for tempo currently
        }
        else if (channel + (TempoTargetFirst) == TempoTargetReset)
        {
            adaptiveReset();
        }
        else if(message.isNoteOn())
            atNewNote();
        else if(message.isNoteOff())
            atNewNoteOff();
    }
}

void TempoProcessor::atNewNote()
{
    if(state.params.tempoModeOptions->get() == TempoModeType::Adaptive2Time_Between_Notes)
        atCalculatePeriodMultiplier();
    atLastTime = atTimer;
}

void TempoProcessor::atNewNoteOff()
{
    if(state.params.tempoModeOptions->get() == TempoModeType::Adaptive2Sustain_Time)
        atCalculatePeriodMultiplier();
}

int TempoProcessor::getAtDelta()
{
    return atDelta = (atTimer - atLastTime) * 1000. / getSampleRate();
}

//really basic, using constrained moving average of time-between-notes (or note-length)
void TempoProcessor::atCalculatePeriodMultiplier()
{
    if(*state.params.historyParam > 0) {

        atDelta = (atTimer - atLastTime) / (0.001 * getSampleRate());

        //constrain be min and max times between notes
        if (atDelta > *state.params.timeWindowMinMaxParams.holdTimeMinParam && atDelta < *state.params.timeWindowMinMaxParams.holdTimeMaxParam )
        {
            //insert delta into history circular buffer
            atDeltaHistory[atHistoryWriteIndex] = atDelta;
            atHistoryWriteIndex = (atHistoryWriteIndex + 1) % 10;

            //calculate moving average and then tempo period multiplier
            int historySize = juce::jlimit (1, 10, (int) *state.params.historyParam);
            int totalDeltas = 0;
            for (int i = 0; i < historySize; ++i)
            {
                int index = (atHistoryWriteIndex - 1 - i + 10) % 10;
                totalDeltas += atDeltaHistory[index];
            }

            float movingAverage = (float) totalDeltas / (float) historySize;
            float beatThreshMS =  60000. / *state.params.tempoParam;

            adaptiveTempoPeriodMultiplier = movingAverage /
                                            beatThreshMS; // /
                                            //*state.params.subdivisionsParam;

            DBG("adaptiveTempoPeriodMultiplier = " + juce::String(adaptiveTempoPeriodMultiplier));
        }
    }
}

void TempoProcessor::adaptiveReset()
{
    int historySize = juce::jlimit (1, 10, (int) *state.params.historyParam);
    int defaultDelta = (int) (60000.0 / (*state.params.tempoParam));
    for (int i = 0; i < 10; i++)
    {
        atDeltaHistory[i] = defaultDelta;
    }
    atHistoryWriteIndex = 0;
    adaptiveTempoPeriodMultiplier = 1.;
}

void TempoProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{

    state.getParameterListeners().callAudioThreadBroadcasters();

    /*
     * modulation stuff
     */

    // first, the continuous modulations (simple knobs/sliders...)
    processContinuousModulations(buffer);
    state.params.timeWindowMinMaxParams.processStateChanges();

    if (state.params.tempoModeOptions->get() == TempoModeType::Host_Tempo) {
        if (auto* ph = getPlayHead()) {
            auto position = ph->getPosition();
            if (position.hasValue()) {
                auto bpm = position->getBpm();
                if (bpm.hasValue()) {
                    if (*bpm != state.params.tempoParam->get()) {
                        state.params.tempoParam->beginChangeGesture();
                        state.params.tempoParam->setParameterValue (*bpm);
                        state.params.tempoParam->endChangeGesture();
                    }
                }
            }
        }
    }

    if (state.params.tempoModeOptions->get() == TempoModeType::Adaptive2Sustain_Time || state.params.tempoModeOptions->get() == TempoModeType::Adaptive2Time_Between_Notes)
    {
        ProcessMIDIBlock(midiMessages, buffer.getNumSamples());
    }

    // since this is an instrument source; doesn't take audio in, other than mods handled above
    buffer.clear();
}

void TempoProcessor::processBlockBypassed (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{

}

/**
 * Serializers, for saving/loading complex params like the multisliders
 * - don't really need these for Tempo at the moment
 * - and the commented out rescaling stuff is also not needed for regular parasms
 *      - only for range params like velocity min/max....
 *      - leaving it here for now for reference
 */
template <typename Serializer>
typename Serializer::SerializedType TempoParams::serialize (const TempoParams& paramHolder)
{
    auto ser = chowdsp::ParamHolder::serialize<Serializer> (paramHolder);
    return ser;
}

template <typename Serializer>
void TempoParams::deserialize (typename Serializer::DeserializedType deserial, TempoParams& paramHolder)
{
    // // Pre-scan attributes for params that might have ranges saved beyond the pre-defined defaults
    // const int numAttrs = Serializer::getNumAttributes(deserial);
    //
    // auto expandRangeIfNeeded = [deserial](auto& param, const juce::String& name) {
    //     auto val = Serializer::template deserializeArithmeticType<float>(deserial, name);
    //     if (!std::isnan(val))
    //     {
    //         if (val < param->range.start)
    //             param->range.start = val;
    //         if (val > param->range.end)
    //             param->range.end = val;
    //     }
    // };
    //
    // for (int i = 0; i < numAttrs; ++i)
    // {
    //     auto name = Serializer::getAttributeName(deserial, i);
    //     if (name == juce::String("tempo"))
    //         expandRangeIfNeeded(paramHolder.tempoParam, name);
    //     else if (name == juce::String("subdivisions"))
    //         expandRangeIfNeeded(paramHolder.subdivisionsParam, name);
    //     else if (name == juce::String("history"))
    //         expandRangeIfNeeded(paramHolder.historyParam, name);
    // }

    /*
     * call the default deserializer, for the simple params
     */
    chowdsp::ParamHolder::deserialize<Serializer> (deserial, paramHolder);
}
