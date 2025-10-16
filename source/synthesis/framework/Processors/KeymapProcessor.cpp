//
// Created by Davis Polito on 6/19/24.
//

#include "KeymapProcessor.h"
#include "array_to_string.h"
#include "common.h"
#include "synth_base.h"

KeymapProcessor::KeymapProcessor (SynthBase& parent, const juce::ValueTree& vt ) : PluginBase (
                                                                                     parent,
                                                                                     vt,
                                                                                     nullptr,
                                                                                     keymapBusLayout()),
                                                                                 _midi (std::make_unique<MidiManager> (&keyboard_state, parent.manager, vt))
{
    state.params.velocityMinMax.stateChanges.defaultState = v.getOrCreateChildWithName(IDs::PARAM_DEFAULT,nullptr);

    parent.getStateBank().addParam (std::make_pair<std::string,
        bitklavier::ParameterChangeBuffer*> (v.getProperty (IDs::uuid).toString().toStdString() + "_" + "velocityminmax",
        &(state.params.velocityMinMax.stateChanges)));
}

static juce::String getMidiMessageDescription (const juce::MidiMessage& m)
{
    if (m.isNoteOn())
        return "Note on " + juce::MidiMessage::getMidiNoteName (m.getNoteNumber(), true, true, 3);
    if (m.isNoteOff())
        return "Note off " + juce::MidiMessage::getMidiNoteName (m.getNoteNumber(), true, true, 3);
    if (m.isProgramChange())
        return "Program change " + juce::String (m.getProgramChangeNumber());
    if (m.isPitchWheel())
        return "Pitch wheel " + juce::String (m.getPitchWheelValue());
    if (m.isAftertouch())
        return "After touch " + juce::MidiMessage::getMidiNoteName (m.getNoteNumber(), true, true, 3) + ": " + juce::String (m.getAfterTouchValue());
    if (m.isChannelPressure())
        return "Channel pressure " + juce::String (m.getChannelPressureValue());
    if (m.isAllNotesOff())
        return "All notes off";
    if (m.isAllSoundOff())
        return "All sound off";
    if (m.isMetaEvent())
        return "Meta event";

    if (m.isController())
    {
        juce::String name (juce::MidiMessage::getControllerName (m.getControllerNumber()));

        if (name.isEmpty())
            name = "[" + juce::String (m.getControllerNumber()) + "]";

        return "Controller " + name + ": " + juce::String (m.getControllerValue());
    }

    return juce::String::toHexString (m.getRawData(), m.getRawDataSize());
}

static juce::String printMidi (juce::MidiMessage& message, const juce::String& source)
{
    auto time = message.getTimeStamp(); //- startTime;

    auto hours = ((int) (time / 3600.0)) % 24;
    auto minutes = ((int) (time / 60.0)) % 60;
    auto seconds = ((int) time) % 60;
    auto millis = ((int) (time * 1000.0)) % 1000;

    auto timecode = juce::String::formatted ("%02d:%02d:%02d.%03d",
        hours,
        minutes,
        seconds,
        millis);

    auto description = getMidiMessageDescription (message);

    return juce::String (timecode + "  -  " + description + " (" + source + ")"); // [7]
}

void KeymapProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    //    const auto spec = juce::dsp::ProcessSpec { sampleRate, (uint32_t) samplesPerBlock, (uint32_t) getMainBusNumInputChannels() };
    _midi->midi_collector_.reset (sampleRate);
}

float KeymapProcessor::applyVelocityCurve(float velocity)
{
    /*
     **** Velocity Curving
     user settable parameters:
     --asymmetric warping coefficient (0, 10): default 1 (no warping)
     --symmetric warping coefficent (0, 5): default 1 (no warping)
     --scaling multipler (0, 10): default 1.
     --offset (-1, 1): default 0.
     --invert velocities, toggle: default off

     also, the user should be able to set extendRange (in dB), which is in BKPianoSampler::startNote()
     and will presumably need to pass through here.

     velocity curving doesn't actually extend the dynamic range (well, it could if scaling results
     in velocities > 1.), but rather just distributes the incoming velocities across the dynamic
     range with the sample layers. extendRange will extend the total dynamic of the sample set, and
     is set to 4dB by default at the moment (that's probably a reasonable default, and feels good for
     the Heavy set and other new bK sample libraries).
     */

    float asym_k            = state.params.velocityCurve_asymWarp->getCurrentValue();
    float sym_k             = state.params.velocityCurve_symWarp->getCurrentValue();
    float scale             = state.params.velocityCurve_scale->getCurrentValue();
    float offset            = state.params.velocityCurve_offset->getCurrentValue();
    bool velocityInvert     = state.params.velocityCurve_invert->get();

    // don't filter if not necessary
    if (juce::approximatelyEqual(asym_k, 1.0f) &&
        juce::approximatelyEqual(sym_k, 1.0f) &&
        juce::approximatelyEqual(scale, 1.0f) &&
        juce::approximatelyEqual(offset, 0.0f) &&
        velocityInvert == false)
    {
        return velocity;
    }

    float velocityCurved = bitklavier::utils::dt_warpscale(velocity, asym_k, sym_k, scale, offset);
    if (velocityInvert) velocityCurved = 1. - velocityCurved;

    if (velocityCurved < 0.) velocityCurved = 0.;
    else if (velocityCurved > 1.) velocityCurved = 1.;

    return velocityCurved;
}

bool KeymapProcessor::checkVelocityRange(float velocity)
{
    float velocityMin = state.params.velocityMinMax.velocityMinParam->getCurrentValue();
    float velocityMax = state.params.velocityMinMax.velocityMaxParam->getCurrentValue();

    //in normal case where velocityMin < velocityMax, we only pass if both are true
    if (velocityMax >= velocityMin)
    {
        if (velocity >= velocityMin && velocity <= velocityMax) return true;
        else return false;
    }

    //case where velocityMin > velocityMax, we pass if either is true
    if (velocity >= velocityMin || velocity <= velocityMax) return true;
    else return false;
}

void KeymapProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    state.params.velocityMinMax.processStateChanges();

    midiMessages.clear();
    int num_samples = buffer.getNumSamples();

    juce::MidiBuffer in_midi_messages;
    _midi->removeNextBlockOfMessages (in_midi_messages, num_samples);
    _midi->replaceKeyboardMessages (in_midi_messages, num_samples);

    for (auto message : in_midi_messages)
    {
        if (state.params.keyboard_state.keyStates.test (message.getMessage().getNoteNumber()))
        {
            if(message.getMessage().isNoteOn())
            {
                state.params.velocityMinMax.lastVelocityParam = message.getMessage().getVelocity();
                if (!checkVelocityRange(message.getMessage().getVelocity())) continue;
            }

            float oldvelocity = message.getMessage().getVelocity() / 127.0;
            float newvelocity = applyVelocityCurve(oldvelocity);

            if(newvelocity > 1.0) newvelocity = 1.0;
            if(newvelocity < 0.0) newvelocity = 0.0;

            auto newmsg = message.getMessage();
            newmsg.setVelocity(newvelocity);
            midiMessages.addEvent (newmsg, message.samplePosition);

            if(newmsg.isNoteOn())
            {
                state.params.invelocity = oldvelocity;
                state.params.warpedvelocity = newvelocity;
            }
        }
    }

    // print them out for now
    for (auto mi : midiMessages)
    {
        auto message = mi.getMessage();

        mi.samplePosition;
        mi.data;
        DBG (printMidi (message, "") +  " velocity = " + juce::String(mi.getMessage().getVelocity()));
    }

    // DBG("keymap");
}
void KeymapProcessor::processBlockBypassed (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    processBlock (buffer, midiMessages);
}
void KeymapProcessor::allNotesOff()
{
    _midi->allNotesOff();
}



template <typename Serializer>
typename Serializer::SerializedType KeymapParams::serialize (const KeymapParams& paramHolder)
{
    auto ser = chowdsp::ParamHolder::serialize<Serializer> (paramHolder);
    Serializer::template addChildElement<128> (ser, "keyOn", paramHolder.keyboard_state.keyStates, getOnKeyString);
    return ser;
}

template <typename Serializer>
void KeymapParams::deserialize (typename Serializer::DeserializedType deserial, KeymapParams& paramHolder)
{
    chowdsp::ParamHolder::deserialize<Serializer> (deserial, paramHolder);
    paramHolder.keyboard_state.keyStates = bitklavier::utils::stringToBitset (deserial->getStringAttribute ("keyOn"));
}

