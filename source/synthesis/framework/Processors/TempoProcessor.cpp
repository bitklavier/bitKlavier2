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
}

void TempoProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    setRateAndBufferSizeDetails (sampleRate, samplesPerBlock);
}

bool TempoProcessor::isBusesLayoutSupported (const juce::AudioProcessor::BusesLayout& layouts) const
{
    return true;
}

void TempoProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    //DBG (v.getParent().getParent().getProperty (IDs::name).toString() + "direct");

    /*
     * this updates all the AudioThread callbacks we might have in place
     * for instance, in TuningParametersView.cpp, we have lots of lambda callbacks from the UI
     *  they are all on the MessageThread, but if we wanted to have them synced to the block
     *      we would put them on the AudioThread and they would be heard here
     *  if we put them on the AudioThread, it would be important to have minimal actions in those
     *      callbacks, no UI stuff, etc, just updating params needed in the audio block here
     *      if we want to do other stuff for the same callback, we should have a second MessageThread callback
     *
     *  I'm not sure we have any of these for Tempo, but no harm in calling it, and for reference going forward
     */
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
