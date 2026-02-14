//
// Created by Davis Polito on 12/8/23.
//

#ifndef BITKLAVIER2_IDENTIFIERS_H
#define BITKLAVIER2_IDENTIFIERS_H
#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
namespace IDs
{
#define DECLARE_ID(name) const juce::Identifier name (#name);

    DECLARE_ID (GALLERY)
    DECLARE_ID (CONTROLS)
    DECLARE_ID (INPUT)
    DECLARE_ID (OUTPUT)
    DECLARE_ID (PIANO)
    DECLARE_ID (GLOBALPIANOSAMPLES)
    DECLARE_ID (PREPARATIONS)
    DECLARE_ID (BUSEQ)
    DECLARE_ID (BUSCOMPRESSOR)

    DECLARE_ID (name)

    DECLARE_ID (PREPARATION)
    DECLARE_ID (id)
    DECLARE_ID (type)
    // DECLARE_ID (x)
    // DECLARE_ID (y)
    DECLARE_ID (height)
    DECLARE_ID (width)
    DECLARE_ID (numIns)
    DECLARE_ID (numOuts)
    DECLARE_ID (nodeID)

    DECLARE_ID (CONNECTION)
    DECLARE_ID (CONNECTIONS)
    DECLARE_ID (src)
    DECLARE_ID (dest)
    DECLARE_ID (srcIdx)
    DECLARE_ID (destIdx)
    DECLARE_ID (isIn)
    DECLARE_ID (PORT)
    DECLARE_ID (chIdx)
    // Per-port placement tweak properties
    DECLARE_ID (yOffset)
    DECLARE_ID (TUNINGCONNECTION)
    DECLARE_ID (MODCONNECTION)
    DECLARE_ID (MODCONNECTIONS)
    DECLARE_ID (TEMPOCONNECTION)
    DECLARE_ID (SYNCHRONICCONNECTION)

    DECLARE_ID (assignment)

    DECLARE_ID (uuid)
    DECLARE_ID (midiInput)
    DECLARE_ID (midiDeviceId)
    DECLARE_ID (active)
    DECLARE_ID (midiPrefs)

    // Per-preparation default placement tweaks (used when per-port offset is not provided)
    DECLARE_ID (inputYOffset)
    DECLARE_ID (outputYOffset)

    DECLARE_ID (modulationproc)
    DECLARE_ID (ModulationConnection)

    DECLARE_ID (modAmt)
    DECLARE_ID (isBipolar)
    DECLARE_ID (isOffsetMod)
    DECLARE_ID (isMod)
    DECLARE_ID (modulationToggleMode)
    DECLARE_ID (parameter)
    DECLARE_ID (MODULATABLE_PARAMS)
    DECLARE_ID (MODULATABLE_PARAM)

    DECLARE_ID (absoluteTuning)
    DECLARE_ID (circularTuning)
    DECLARE_ID (isState)

    DECLARE_ID (PLUGIN)
    DECLARE_ID (x_y)
    DECLARE_ID (PARAM_DEFAULT)
    DECLARE_ID (RESETCONNECTION)
    DECLARE_ID (isActive)
    DECLARE_ID (MIDIFILTERCONNECTION)
    DECLARE_ID (selectedPianoIndex)
    DECLARE_ID (selectedPianoName)
    DECLARE_ID (sync);

    DECLARE_ID(mod0to1)

    DECLARE_ID(start)
    DECLARE_ID(end)
    DECLARE_ID(skew)

    DECLARE_ID(numModChans)
    DECLARE_ID(sliderval)

    DECLARE_ID(adsr_attack)
    DECLARE_ID(adsr_decay)
    DECLARE_ID(adsr_sustain)
    DECLARE_ID(adsr_release)
    DECLARE_ID(adsr_attackPower)
    DECLARE_ID(adsr_decayPower)
    DECLARE_ID(adsr_releasePower)
    DECLARE_ID(adsr_active)

    DECLARE_ID(combo_box_index)
    DECLARE_ID(fundamental)
    DECLARE_ID(tuningType)
    DECLARE_ID(tuningSystem)

    DECLARE_ID(direct)
    DECLARE_ID(blendronic)
    DECLARE_ID(reset)
    DECLARE_ID(midiFilter)
    DECLARE_ID(pianoMap)
    DECLARE_ID(midiTarget)
    DECLARE_ID(nostalgic)
    DECLARE_ID(synchronic)
    DECLARE_ID(resonance)
    DECLARE_ID(modulation)
    DECLARE_ID(tuning)
    DECLARE_ID(tempo)
    DECLARE_ID(vst)
    DECLARE_ID(keymap)
    DECLARE_ID(noConnection)
    DECLARE_ID(linkedPrep)
    DECLARE_ID(linkedType)
    DECLARE_ID(linkedPianoName)

    DECLARE_ID(soundset)
    DECLARE_ID(globalsoundset)
    DECLARE_ID(syncglobal)
    DECLARE_ID(soundfont_preset)

    DECLARE_ID(global_A440)
    DECLARE_ID(global_tempo_multiplier)
}

// order should match BKPreparationType in common.h
static const std::array<juce::Identifier, 13> preparationIDs {
    IDs::keymap,
    IDs::direct,
    IDs::synchronic,
    IDs::nostalgic,
    IDs::blendronic,
    IDs::resonance,
    IDs::tuning,
    IDs::tempo,
    IDs::midiFilter,
    IDs::midiTarget,
    IDs::modulation,
    IDs::reset,
    IDs::pianoMap
};

#undef DECLARE_ID
inline juce::ValueTree createUuidProperty (juce::ValueTree& v)
{
    if (!v.hasProperty (IDs::uuid))
        v.setProperty (IDs::uuid, juce::Uuid().toString(), nullptr);

    return v;
}

#endif //BITKLAVIER2_IDENTIFIERS_H
