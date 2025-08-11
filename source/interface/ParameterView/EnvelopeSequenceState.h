//
// Created by Dan Trueman on 8/7/25.
//

#ifndef BITKLAVIER0_ENVELOPESEQUENCESTATE_H
#define BITKLAVIER0_ENVELOPESEQUENCESTATE_H

#include "array_to_string.h"
#include "chowdsp_plugin_state/chowdsp_plugin_state.h"

#define MAXADSRS 12

struct EnvelopeSequenceState : bitklavier::StateChangeableParameter
{

    EnvelopeSequenceState() : bitklavier::StateChangeableParameter()
    {
        for (auto& attack : attacks) attack = 3.0f;
        for (auto& decay : decays) decay = 10.0f;
        for (auto& sustain : sustains) sustain = 1.0f;
        for (auto& release : releases) release = 50.0f;

        for (auto& attackPower : attackPowers) attackPower = 0.0f;
        for (auto& decayPower : decayPowers) decayPower = 0.0f;
        for (auto& releasePower : releasePowers) releasePower = 0.0f;
    }

    // primary ADSR param values
    std::array<std::atomic<float>, MAXADSRS> attacks;
    std::array<std::atomic<float>, MAXADSRS> decays;
    std::array<std::atomic<float>, MAXADSRS> sustains;
    std::array<std::atomic<float>, MAXADSRS> releases;

    // power param values
    std::array<std::atomic<float>, MAXADSRS> attackPowers;
    std::array<std::atomic<float>, MAXADSRS> decayPowers;
    std::array<std::atomic<float>, MAXADSRS> releasePowers;

    std::atomic<bool> updateUI;

    void processStateChanges() override
    {
        updateUI = false;

        for(auto [index, change] : stateChanges.changeState)
        {
            static juce::var nullVar;

            auto sval = change.getProperty (IDs::adsr_attack);
            if (sval != nullVar) {
                stringToAtomicArray(attacks, sval.toString(), 1.);
            }

            sval = change.getProperty (IDs::adsr_decay);
            if (sval != nullVar) {
                stringToAtomicArray(decays, sval.toString(), 1.);
            }

            sval = change.getProperty (IDs::adsr_sustain);
            if (sval != nullVar) {
                stringToAtomicArray(sustains, sval.toString(), 1.);
            }

            sval = change.getProperty (IDs::adsr_release);
            if (sval != nullVar) {
                stringToAtomicArray(releases, sval.toString(), 1.);
            }

            sval = change.getProperty (IDs::adsr_attackPower);
            if (sval != nullVar) {
                stringToAtomicArray(attackPowers, sval.toString(), 1.);
            }

            sval = change.getProperty (IDs::adsr_decayPower);
            if (sval != nullVar) {
                stringToAtomicArray(decayPowers, sval.toString(), 1.);
            }

            sval = change.getProperty (IDs::adsr_releasePower);
            if (sval != nullVar) {
                stringToAtomicArray(releasePowers, sval.toString(), 1.);
            }

            updateUI = true;
        }

        // must clear at the end, otherwise they'll get reapplied again and again
        stateChanges.changeState.clear();

    }
};

template <typename Serializer>
void serializeArrayADSRParam(
    typename Serializer::SerializedType& ser,
    const EnvelopeSequenceState& msliderParam,
    const juce::String& thisSliderID)
{
    // Define the specific string IDs for serialization
    juce::String _attacks = thisSliderID + "_attacks";
    juce::String _decays = thisSliderID + "_decays";
    juce::String _sustains = thisSliderID + "_sustains";
    juce::String _releases = thisSliderID + "_releases";
    juce::String _attackPowers = thisSliderID + "_attackPowers";
    juce::String _decayPowers = thisSliderID + "_decayPowers";
    juce::String _releasePowers = thisSliderID + "_releasePowers";
    juce::String _activeADSRs = thisSliderID + "_activeADSRs";

    Serializer::addChildElement(ser, _attacks, atomicArrayToString(msliderParam.attacks));
    Serializer::addChildElement(ser, _decays, atomicArrayToString(msliderParam.decays));
    Serializer::addChildElement(ser, _sustains, atomicArrayToString(msliderParam.sustains));
    Serializer::addChildElement(ser, _releases, atomicArrayToString(msliderParam.releases));
    Serializer::addChildElement(ser, _attackPowers, atomicArrayToString(msliderParam.attackPowers));
    Serializer::addChildElement(ser, _decayPowers, atomicArrayToString(msliderParam.decayPowers));
    Serializer::addChildElement(ser, _releasePowers, atomicArrayToString(msliderParam.releasePowers));
}


template <typename Serializer>
void deserializeArrayADSRParam(
    typename Serializer::DeserializedType deserial,
    EnvelopeSequenceState& msliderParam,
    const juce::String& thisSliderID)
{
    // Define the specific string IDs for serialization
    juce::String _attacks = thisSliderID + "_attacks";
    juce::String _decays = thisSliderID + "_decays";
    juce::String _sustains = thisSliderID + "_sustains";
    juce::String _releases = thisSliderID + "_releases";
    juce::String _attackPowers = thisSliderID + "_attackPowers";
    juce::String _decayPowers = thisSliderID + "_decayPowers";
    juce::String _releasePowers = thisSliderID + "_releasePowers";
    juce::String _activeADSRs = thisSliderID + "_activeADSRs";

    // Deserialize the slider values
    paramFromString(_attacks, deserial, msliderParam.attacks);
    paramFromString(_decays, deserial, msliderParam.decays);
    paramFromString(_sustains, deserial, msliderParam.sustains);
    paramFromString(_releases, deserial, msliderParam.releases);
    paramFromString(_attackPowers, deserial, msliderParam.attackPowers);
    paramFromString(_decayPowers, deserial, msliderParam.decayPowers);
    paramFromString(_releasePowers, deserial, msliderParam.releasePowers);
}

template <typename Serializer, typename T>
void paramFromString(juce::String inStr, typename Serializer::DeserializedType& inDeserial, std::array<std::atomic<T>, 12>& msliderParam)
{
    auto myStr = inDeserial->getStringAttribute(inStr);
    std::vector<T> sliderVals_vec = parseStringToVector<T>(myStr);
    populateAtomicArrayFromVector(msliderParam, 1.0f, sliderVals_vec);
}


#endif //BITKLAVIER0_ENVELOPESEQUENCESTATE_H
