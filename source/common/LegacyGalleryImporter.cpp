//
// LegacyGalleryImporter.cpp
// bitKlavier2
//

#include "LegacyGalleryImporter.h"
#include <juce_audio_basics/juce_audio_basics.h>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

uint32_t LegacyGalleryImporter::newNodeID()
{
    // Use a large pseudo-random uint32, avoiding 0
    auto r = static_cast<uint32_t> (juce::Random::getSystemRandom().nextInt());
    if (r == 0) r = 1;
    return r;
}

juce::String LegacyGalleryImporter::newUUID()
{
    return juce::Uuid().toDashedString().removeCharacters ("-");
}

juce::ValueTree LegacyGalleryImporter::makeModParam (const juce::String& paramName,
                                                      float start, float end,
                                                      float skew, float val)
{
    juce::ValueTree vt ("MODULATABLE_PARAM");
    vt.setProperty ("parameter", paramName, nullptr);
    vt.setProperty ("start",     start,     nullptr);
    vt.setProperty ("end",       end,       nullptr);
    vt.setProperty ("skew",      skew,      nullptr);
    vt.setProperty ("sliderval", val,       nullptr);
    return vt;
}

float LegacyGalleryImporter::gainToDb (float linearGain)
{
    if (linearGain <= 0.0f)
        return -80.0f;
    float db = juce::Decibels::gainToDecibels (linearGain);
    return juce::jlimit (-80.0f, 6.0f, db);
}

juce::String LegacyGalleryImporter::parseIndexedFloats (const juce::XmlElement& el,
                                                          const juce::String& prefix,
                                                          int maxCount)
{
    juce::String result;
    for (int i = 0; i < maxCount; ++i)
    {
        juce::String key = prefix + juce::String (i);
        if (! el.hasAttribute (key))
            break;
        if (i > 0) result += " ";
        result += el.getStringAttribute (key);
    }
    return result;
}

juce::String LegacyGalleryImporter::parseIndexedBools (const juce::XmlElement& el,
                                                         const juce::String& prefix,
                                                         int maxCount)
{
    juce::String result;
    for (int i = 0; i < maxCount; ++i)
    {
        juce::String key = prefix + juce::String (i);
        if (! el.hasAttribute (key))
            break;
        if (i > 0) result += " ";
        result += (el.getIntAttribute (key) != 0) ? "true" : "false";
    }
    return result;
}

juce::String LegacyGalleryImporter::tagForOldType (int oldType)
{
    switch (oldType)
    {
        case OldDirect:     return "direct";
        case OldSynchronic: return "synchronic";
        case OldNostalgic:  return "nostalgic";
        case OldTuning:     return "tuning";
        case OldResonance:  return "resonance";
        case OldKeymap:     return "keymap";
        case OldKeymapMain: return "keymap";
        case OldBlendronic: return "blendronic";
        case OldTempo:      return "tempo";
        default:            return {};
    }
}

int LegacyGalleryImporter::newTypeForOldType (int oldType)
{
    // Maps old layout type codes to new "type" attribute values.
    // New type IDs (from Identifiers.h preparationIDs array index):
    // 0=keymap, 1=direct, 2=synchronic, 3=nostalgic, 4=blendronic,
    // 5=resonance, 6=tuning, 7=tempo
    switch (oldType)
    {
        case OldDirect:     return 1;
        case OldSynchronic: return 2;
        case OldNostalgic:  return 3;
        case OldTuning:     return 6;
        case OldResonance:  return 5;
        case OldKeymap:     return 0;
        case OldKeymapMain: return 0;
        case OldBlendronic: return 4;
        case OldTempo:      return 7;
        default:            return -1;
    }
}

// ---------------------------------------------------------------------------
// Default bus elements
// ---------------------------------------------------------------------------

juce::ValueTree LegacyGalleryImporter::makeDefaultBusEQ()
{
    juce::ValueTree vt ("BUSEQ");
    vt.setProperty ("type",     15,            nullptr);
    vt.setProperty ("uuid",     newUUID(),     nullptr);
    vt.setProperty ("soundset", "syncglobal",  nullptr);
    vt.addChild (juce::ValueTree ("MODULATABLE_PARAMS"), -1, nullptr);
    return vt;
}

juce::ValueTree LegacyGalleryImporter::makeDefaultBusCompressor()
{
    juce::ValueTree vt ("BUSCOMPRESSOR");
    vt.setProperty ("type",     14,           nullptr);
    vt.setProperty ("uuid",     newUUID(),    nullptr);
    vt.setProperty ("soundset", "syncglobal", nullptr);

    juce::ValueTree mp ("MODULATABLE_PARAMS");
    // These defaults match the values in BasicPiano.bk2
    const float kDbMin = -80.0f, kDbMax = 6.0f, kDbSkew = 2.0f;
    const float kNearMute = 2.264976501464844e-6f; // effectively muted
    mp.addChild (makeModParam ("InputGain",  kDbMin, kDbMax, kDbSkew, kNearMute), -1, nullptr);
    mp.addChild (makeModParam ("Send",       kDbMin, kDbMax, kDbSkew, kNearMute), -1, nullptr);
    mp.addChild (makeModParam ("OutputGain", kDbMin, kDbMax, kDbSkew, kNearMute), -1, nullptr);
    mp.addChild (makeModParam ("loCutFreq",  20.0f, 20000.0f, 0.2f, 20.0f),      -1, nullptr);
    mp.addChild (makeModParam ("loCutSlope", 12.0f, 48.0f, 1.0f, 12.0f),         -1, nullptr);
    mp.addChild (makeModParam ("peak1Freq",  20.0f, 20000.0f, 0.2f, 200.0f),     -1, nullptr);
    mp.addChild (makeModParam ("peak1Gain",  -24.0f, 24.0f, 0.2f, 15.0f),        -1, nullptr);
    mp.addChild (makeModParam ("peak1Q",     0.1f, 10.0f, 0.2f, 1.0f),           -1, nullptr);
    mp.addChild (makeModParam ("peak1Slope", 12.0f, 48.0f, 1.0f, 12.0f),         -1, nullptr);
    mp.addChild (makeModParam ("peak2Freq",  20.0f, 20000.0f, 0.2f, 1000.0f),    -1, nullptr);
    mp.addChild (makeModParam ("peak2Gain",  -24.0f, 24.0f, 0.2f, 15.0f),        -1, nullptr);
    mp.addChild (makeModParam ("peak2Q",     0.1f, 10.0f, 0.2f, 1.0f),           -1, nullptr);
    mp.addChild (makeModParam ("peak2Slope", 12.0f, 48.0f, 1.0f, 12.0f),         -1, nullptr);
    mp.addChild (makeModParam ("peak3Freq",  20.0f, 20000.0f, 0.2f, 5000.0f),    -1, nullptr);
    mp.addChild (makeModParam ("peak3Gain",  -24.0f, 24.0f, 0.2f, 15.0f),        -1, nullptr);
    mp.addChild (makeModParam ("peak3Q",     0.1f, 10.0f, 0.2f, 1.0f),           -1, nullptr);
    mp.addChild (makeModParam ("peak3Slope", 12.0f, 48.0f, 1.0f, 12.0f),         -1, nullptr);
    mp.addChild (makeModParam ("hiCutFreq",  20.0f, 20000.0f, 0.6f, 20000.0f),   -1, nullptr);
    mp.addChild (makeModParam ("hiCutSlope", 12.0f, 48.0f, 1.0f, 12.0f),         -1, nullptr);
    vt.addChild (mp, -1, nullptr);
    return vt;
}

// ---------------------------------------------------------------------------
// Per-preparation converters
// ---------------------------------------------------------------------------

juce::ValueTree LegacyGalleryImporter::convertDirect (const juce::XmlElement& el,
                                                       uint32_t nodeID,
                                                       const juce::String& uuid,
                                                       int x, int y,
                                                       const juce::String& name,
                                                       int instanceIndex)
{
    juce::ValueTree vt ("direct");
    vt.setProperty ("type",     1,                              nullptr);
    vt.setProperty ("width",    147.0f,                         nullptr);
    vt.setProperty ("height",   75.0f,                          nullptr);
    vt.setProperty ("x_y",      juce::String(x) + ", " + juce::String(y), nullptr);
    vt.setProperty ("uuid",     uuid,                           nullptr);
    vt.setProperty ("soundset", "syncglobal",                   nullptr);
    vt.setProperty ("nodeID",   (int64_t) nodeID,               nullptr);
    vt.setProperty ("name",     name,                           nullptr);
    vt.setProperty ("numIns",   1,                              nullptr);
    vt.setProperty ("numOuts",  2,                              nullptr);

    const juce::XmlElement* params = el.getChildByName ("params");

    // Gain parameters — old format uses linear multipliers, new uses dB
    float mainDb    = -80.0f;
    float hammersDb = -80.0f;
    float resonDb   = 0.0f;
    if (params != nullptr)
    {
        mainDb    = gainToDb ((float) params->getDoubleAttribute ("gain",      0.0));
        hammersDb = gainToDb ((float) params->getDoubleAttribute ("hammerGain", 0.0));
        resonDb   = gainToDb ((float) params->getDoubleAttribute ("resGain",    0.0));
    }

    vt.setProperty ("Main",      mainDb,    nullptr);
    vt.setProperty ("Hammers",   hammersDb, nullptr);
    vt.setProperty ("Resonance", resonDb,   nullptr);
    vt.setProperty ("Pedal",     -6.0f,     nullptr);
    vt.setProperty ("Send",      0.0f,      nullptr);
    vt.setProperty ("OutputGain",0.0f,      nullptr);
    vt.setProperty ("resonanceLoaded", 1,   nullptr);
    vt.setProperty ("hammerLoaded",    1,   nullptr);
    vt.setProperty ("pedalLoaded",     1,   nullptr);

    // ADSR — old: f0=attack f1=decay f2=sustain f3=release
    float attack = 3.0f, decay = 10.0f, sustain = 1.0f, release = 50.0f;
    if (params != nullptr)
    {
        const juce::XmlElement* adsr = params->getChildByName ("ADSR");
        if (adsr != nullptr)
        {
            attack  = (float) adsr->getDoubleAttribute ("f0", 3.0);
            decay   = (float) adsr->getDoubleAttribute ("f1", 10.0);
            sustain = (float) adsr->getDoubleAttribute ("f2", 1.0);
            release = (float) adsr->getDoubleAttribute ("f3", 50.0);
        }
    }
    vt.setProperty ("attack",       attack,  nullptr);
    vt.setProperty ("decay",        decay,   nullptr);
    vt.setProperty ("sustain",      sustain, nullptr);
    vt.setProperty ("release",      release, nullptr);
    vt.setProperty ("attackpower",  0.0f,    nullptr);
    vt.setProperty ("decaypower",   0.0f,    nullptr);
    vt.setProperty ("releasepower", 0.0f,    nullptr);
    vt.setProperty ("hold",         0.0f,    nullptr);
    vt.setProperty ("delay",        0.0f,    nullptr);
    vt.setProperty ("notify",       1,       nullptr);

    // Transposition offsets (t0..t11).
    // sliderValsSize = number of f* attributes actually present in <transposition>.
    // This controls how many transposition slots are active.
    int transpCount = 1;
    if (params != nullptr)
    {
        const juce::XmlElement* transp = params->getChildByName ("transposition");
        if (transp != nullptr)
        {
            transpCount = 0;
            for (int i = 0; i < 12; ++i)
            {
                juce::String key = "f" + juce::String (i);
                if (! transp->hasAttribute (key)) break;
                vt.setProperty ("t" + juce::String (i),
                                (float) transp->getDoubleAttribute (key, 0.0),
                                nullptr);
                ++transpCount;
            }
            if (transpCount == 0) transpCount = 1;
        }
    }
    vt.setProperty ("sliderValsSize", (float) transpCount, nullptr);
    vt.setProperty ("UseTuning",      0,    nullptr);

    // MODULATABLE_PARAMS
    const float kDbMin = -80.0f, kDbMax = 6.0f, kDbSkew = 2.0f;
    juce::ValueTree mp ("MODULATABLE_PARAMS");
    mp.addChild (makeModParam ("Main",        kDbMin, kDbMax, kDbSkew, mainDb),    -1, nullptr);
    mp.addChild (makeModParam ("Hammers",     kDbMin, kDbMax, kDbSkew, hammersDb), -1, nullptr);
    mp.addChild (makeModParam ("Resonance",   kDbMin, kDbMax, kDbSkew, resonDb),   -1, nullptr);
    mp.addChild (makeModParam ("Pedal",       kDbMin, kDbMax, kDbSkew, -6.0f),     -1, nullptr);
    mp.addChild (makeModParam ("Send",        kDbMin, kDbMax, kDbSkew, 0.0f),      -1, nullptr);
    mp.addChild (makeModParam ("OutputGain",  kDbMin, kDbMax, kDbSkew, 0.0f),      -1, nullptr);
    mp.addChild (makeModParam ("attack",      0.0f, 10000.0f, 0.2313782125711441f, attack),  -1, nullptr);
    mp.addChild (makeModParam ("decay",       0.0f, 1000.0f,  1.0f, decay),        -1, nullptr);
    mp.addChild (makeModParam ("sustain",     0.0f, 1.0f,     1.0f, sustain),      -1, nullptr);
    mp.addChild (makeModParam ("release",     0.0f, 10000.0f, 0.2313782125711441f, release), -1, nullptr);
    mp.addChild (makeModParam ("attackpower", -10.0f, 10.0f,  1.0f, 0.0f),         -1, nullptr);
    mp.addChild (makeModParam ("delay",       0.0f, 1000.0f,  1.0f, 0.0f),         -1, nullptr);
    vt.addChild (mp, -1, nullptr);

    // PARAM_DEFAULT (empty)
    vt.addChild (juce::ValueTree ("PARAM_DEFAULT"), -1, nullptr);

    // PORT elements
    {
        juce::ValueTree port ("PORT");
        port.setProperty ("nodeID", (int64_t) nodeID, nullptr);
        port.setProperty ("chIdx",  4096,              nullptr);
        port.setProperty ("isIn",   1,                 nullptr);
        vt.addChild (port, -1, nullptr);
    }
    {
        juce::ValueTree port ("PORT");
        port.setProperty ("nodeID", (int64_t) nodeID, nullptr);
        port.setProperty ("chIdx",  0,                 nullptr);
        port.setProperty ("isIn",   0,                 nullptr);
        vt.addChild (port, -1, nullptr);
    }
    {
        juce::ValueTree port ("PORT");
        port.setProperty ("nodeID", (int64_t) nodeID, nullptr);
        port.setProperty ("chIdx",  2,                 nullptr);
        port.setProperty ("isIn",   0,                 nullptr);
        vt.addChild (port, -1, nullptr);
    }

    return vt;
}

// ---------------------------------------------------------------------------

juce::ValueTree LegacyGalleryImporter::convertSynchronic (const juce::XmlElement& el,
                                                            uint32_t nodeID,
                                                            const juce::String& uuid,
                                                            int x, int y,
                                                            const juce::String& name,
                                                            int instanceIndex)
{
    juce::ValueTree vt ("synchronic");
    vt.setProperty ("type",     2,                              nullptr);
    vt.setProperty ("width",    156.0f,                         nullptr);
    vt.setProperty ("height",   79.2f,                          nullptr);
    vt.setProperty ("x_y",     juce::String(x) + ", " + juce::String(y), nullptr);
    vt.setProperty ("uuid",     uuid,                           nullptr);
    vt.setProperty ("soundset", "syncglobal",                   nullptr);
    vt.setProperty ("nodeID",   (int64_t) nodeID,               nullptr);
    vt.setProperty ("name",     name,                           nullptr);
    vt.setProperty ("numIns",   1,                              nullptr);
    vt.setProperty ("numOuts",  2,                              nullptr);

    const juce::XmlElement* params = el.getChildByName ("params");

    // numPulses (old: numBeats)
    float numPulses = 20.0f;
    float cMin      = 500.0f;
    int   skipFirst = 1;
    int   pulseTrig = 1; // 0=NoteOn 1=FirstNoteOn in new format enum
    float cThickness = 8.0f;
    float numLayers  = 1.0f;

    if (params != nullptr)
    {
        int numBeats = params->getIntAttribute ("numBeats", 20);
        numPulses    = (float) numBeats;
        cMin         = (float) params->getDoubleAttribute ("clusterThresh", 500.0);
        int beatsToSkip = params->getIntAttribute ("beatsToSkip", 0);
        // beatsToSkip < 0 means skip first in old format
        skipFirst    = (beatsToSkip != 0) ? 1 : 0;
        // mode: 0=noteOn driven, 1=firstNoteOn in old; new pulseTriggeredBy: 1=FirstNoteOn
        int mode     = params->getIntAttribute ("mode", 0);
        pulseTrig    = (mode == 0) ? 0 : 1;
    }

    vt.setProperty ("numPulses",       numPulses,  nullptr);
    vt.setProperty ("numLayers",       numLayers,  nullptr);
    vt.setProperty ("cThickness",      cThickness, nullptr);
    vt.setProperty ("cMin",            cMin,       nullptr);
    vt.setProperty ("Send",            0.0f,       nullptr);
    vt.setProperty ("OutputGain",      0.0f,       nullptr);
    vt.setProperty ("noteOnGain",      0.0f,       nullptr);
    vt.setProperty ("pulseTriggeredBy",pulseTrig,  nullptr);
    vt.setProperty ("determinesCluster", 0,        nullptr);
    vt.setProperty ("skipFirst",        skipFirst, nullptr);
    vt.setProperty ("UseTuning",        0,         nullptr);
    vt.setProperty ("updateUIState",    0,         nullptr);
    vt.setProperty ("clustermin",       1.0f,      nullptr);
    vt.setProperty ("clustermax",       100000.0f, nullptr);
    vt.setProperty ("LastCluster",      1.0f,      nullptr);
    vt.setProperty ("holdTimeMinParam", 0.0f,      nullptr);
    vt.setProperty ("holdTimeMaxParam", 12000.0f,  nullptr);
    vt.setProperty ("lastHoldTimeParam",0.0f,      nullptr);

    // ADSR (single envelope from params, used for all beats initially)
    float attack = 3.0f, decay = 3.0f, sustain = 1.0f, release = 30.0f;

    // Per-beat envelope sequences — read from ADSRs child
    juce::String seqAttacks, seqDecays, seqSustains, seqReleases;
    juce::String seqAttPow, seqDecPow, seqRelPow;

    if (params != nullptr)
    {
        const juce::XmlElement* adsrs = params->getChildByName ("ADSRs");
        if (adsrs != nullptr)
        {
            for (int i = 0; i < 12; ++i)
            {
                const juce::XmlElement* ei = adsrs->getChildByName ("e" + juce::String(i));
                float a = 3.0f, d = 3.0f, s = 1.0f, r = 30.0f;
                if (ei != nullptr)
                {
                    a = (float) ei->getDoubleAttribute ("f0", 3.0);
                    d = (float) ei->getDoubleAttribute ("f1", 3.0);
                    s = (float) ei->getDoubleAttribute ("f2", 1.0);
                    r = (float) ei->getDoubleAttribute ("f3", 30.0);
                }
                if (i > 0) { seqAttacks  += " "; seqDecays   += " ";
                             seqSustains += " "; seqReleases += " ";
                             seqAttPow   += " "; seqDecPow   += " ";
                             seqRelPow   += " "; }
                seqAttacks  += juce::String (a);
                seqDecays   += juce::String (d);
                seqSustains += juce::String (s);
                seqReleases += juce::String (r);
                seqAttPow   += "0";
                seqDecPow   += "0";
                seqRelPow   += "0";
                if (i == 0) { attack = a; decay = d; sustain = s; release = r; }
            }
        }
        else
        {
            // Populate all 12 with defaults
            for (int i = 0; i < 12; ++i)
            {
                if (i > 0) { seqAttacks += " "; seqDecays += " ";
                             seqSustains += " "; seqReleases += " ";
                             seqAttPow += " "; seqDecPow += " "; seqRelPow += " "; }
                seqAttacks  += "3"; seqDecays   += "3";
                seqSustains += "1"; seqReleases += "30";
                seqAttPow   += "0"; seqDecPow   += "0"; seqRelPow += "0";
            }
        }
    }

    vt.setProperty ("attack",   attack,  nullptr);
    vt.setProperty ("decay",    decay,   nullptr);
    vt.setProperty ("sustain",  sustain, nullptr);
    vt.setProperty ("release",  release, nullptr);
    vt.setProperty ("attackpower",  0.0f, nullptr);
    vt.setProperty ("decaypower",   0.0f, nullptr);
    vt.setProperty ("releasepower", 0.0f, nullptr);
    vt.setProperty ("hold",  0.0f, nullptr);
    vt.setProperty ("delay", 0.0f, nullptr);
    vt.setProperty ("notify", 1, nullptr);

    vt.setProperty ("envelope_sequence_attacks",      seqAttacks,  nullptr);
    vt.setProperty ("envelope_sequence_decays",       seqDecays,   nullptr);
    vt.setProperty ("envelope_sequence_sustains",     seqSustains, nullptr);
    vt.setProperty ("envelope_sequence_releases",     seqReleases, nullptr);
    vt.setProperty ("envelope_sequence_attackPowers", seqAttPow,   nullptr);
    vt.setProperty ("envelope_sequence_decayPowers",  seqDecPow,   nullptr);
    vt.setProperty ("envelope_sequence_releasePowers",seqRelPow,   nullptr);

    // Accent multipliers (old: accentMultipliers f0..fN)
    juce::String accVals, accStates;
    if (params != nullptr)
    {
        const juce::XmlElement* accEl = params->getChildByName ("accentMultipliers");
        const juce::XmlElement* accStEl = params->getChildByName ("accentMultipliersStates");
        if (accEl != nullptr)
        {
            accVals   = parseIndexedFloats (*accEl,  "f", 12);
            if (accStEl != nullptr)
                accStates = parseIndexedBools (*accStEl, "b", 12);
        }
    }
    if (accVals.isEmpty())   accVals   = "1.0";
    if (accStates.isEmpty()) accStates = "true";

    vt.setProperty ("accentsSliderValsSize",   (int) juce::StringArray::fromTokens(accVals, " ", "").size(), nullptr);
    vt.setProperty ("accentsSliderVals",        accVals,   nullptr);
    vt.setProperty ("accentsActiveValsSize",   (int) juce::StringArray::fromTokens(accStates, " ", "").size(), nullptr);
    vt.setProperty ("accentsActiveVals",        accStates, nullptr);

    // Sustain/length multipliers (old: lengthMultipliers)
    juce::String slVals, slStates;
    if (params != nullptr)
    {
        const juce::XmlElement* slEl  = params->getChildByName ("lengthMultipliers");
        const juce::XmlElement* slStEl = params->getChildByName ("lengthMultipliersStates");
        if (slEl != nullptr)
        {
            slVals   = parseIndexedFloats (*slEl,   "f", 12);
            if (slStEl != nullptr)
                slStates = parseIndexedBools (*slStEl, "b", 12);
        }
    }
    if (slVals.isEmpty())   slVals   = "1.0";
    if (slStates.isEmpty()) slStates = "true";

    vt.setProperty ("sustain_length_multipliersSliderValsSize",  (int) juce::StringArray::fromTokens(slVals, " ", "").size(), nullptr);
    vt.setProperty ("sustain_length_multipliersSliderVals",       slVals,   nullptr);
    vt.setProperty ("sustain_length_multipliersActiveValsSize",  (int) juce::StringArray::fromTokens(slStates, " ", "").size(), nullptr);
    vt.setProperty ("sustain_length_multipliersActiveVals",       slStates, nullptr);
    // Also duplicate as the new "sustain_length_multipliers*" group
    vt.setProperty ("sustain_length_multipliersVals",   slVals,   nullptr);
    vt.setProperty ("sustain_length_multipliersSize",   (int) juce::StringArray::fromTokens(slVals, " ", "").size(), nullptr);
    vt.setProperty ("sustain_length_multipliersStates", slStates, nullptr);
    vt.setProperty ("sustain_length_multipliersStatesSize", (int) juce::StringArray::fromTokens(slStates, " ", "").size(), nullptr);

    // Beat length multipliers (old: beatMultipliers)
    juce::String blVals, blStates;
    if (params != nullptr)
    {
        const juce::XmlElement* blEl  = params->getChildByName ("beatMultipliers");
        const juce::XmlElement* blStEl = params->getChildByName ("beatMultipliersStates");
        if (blEl != nullptr)
        {
            blVals   = parseIndexedFloats (*blEl,   "f", 12);
            if (blStEl != nullptr)
                blStates = parseIndexedBools (*blStEl, "b", 12);
        }
    }
    if (blVals.isEmpty())   blVals   = "1.0";
    if (blStates.isEmpty()) blStates = "true";

    vt.setProperty ("beat_length_multipliersSliderValsSize",  (int) juce::StringArray::fromTokens(blVals, " ", "").size(), nullptr);
    vt.setProperty ("beat_length_multipliersSliderVals",       blVals,   nullptr);
    vt.setProperty ("beat_length_multipliersActiveValsSize",  (int) juce::StringArray::fromTokens(blStates, " ", "").size(), nullptr);
    vt.setProperty ("beat_length_multipliersActiveVals",       blStates, nullptr);
    vt.setProperty ("beat_length_multipliersVals",   blVals,   nullptr);
    vt.setProperty ("beat_length_multipliersSize",   (int) juce::StringArray::fromTokens(blVals, " ", "").size(), nullptr);
    vt.setProperty ("beat_length_multipliersStates", blStates, nullptr);
    vt.setProperty ("beat_length_multipliersStatesSize", (int) juce::StringArray::fromTokens(blStates, " ", "").size(), nullptr);

    // Transpositions — first transposition offset only (old: transpOffsets/t0 f0)
    vt.setProperty ("transpositionsSliderValsSize",    1, nullptr);
    vt.setProperty ("transpositionsSliderVals",        "0.000000/", nullptr);
    vt.setProperty ("transpositionsActiveValsSize",   12, nullptr);
    vt.setProperty ("transpositionsActiveVals",        "true false false false false false false false false false false false ", nullptr);
    vt.setProperty ("transpositionsVals",              "0.000000/", nullptr);
    vt.setProperty ("transpositionsSize",              1, nullptr);
    vt.setProperty ("transpositionsStates",            "true false false false false false false false false false false false", nullptr);
    vt.setProperty ("transpositionsStatesSize",        12, nullptr);

    vt.setProperty ("holdtimemin", 0.0f,    nullptr);
    vt.setProperty ("holdtimemax", 22143.0f,nullptr);
    vt.setProperty ("currentlyEditing", 0.0f, nullptr);

    // Envelope flags (which beats are active)
    for (int i = 0; i < 12; ++i)
        vt.setProperty ("envelope" + juce::String(i), (i == 0) ? 1 : 0, nullptr);

    // MODULATABLE_PARAMS
    juce::ValueTree mp ("MODULATABLE_PARAMS");
    mp.addChild (makeModParam ("numPulses",  1.0f, 100.0f, 0.9855645895004272f, numPulses), -1, nullptr);
    mp.addChild (makeModParam ("numLayers",  1.0f, 10.0f,  0.854755699634552f, numLayers),  -1, nullptr);
    mp.addChild (makeModParam ("cThickness", 1.0f, 20.0f,  0.9276416301727295f, cThickness),-1, nullptr);
    mp.addChild (makeModParam ("cMin",       20.0f, 2000.0f,0.9855645895004272f, cMin),     -1, nullptr);
    mp.addChild (makeModParam ("Send",       -80.0f, 6.0f, 2.0f, 0.0f),                     -1, nullptr);
    mp.addChild (makeModParam ("OutputGain", -80.0f, 6.0f, 2.0f, 0.0f),                     -1, nullptr);
    mp.addChild (makeModParam ("noteOnGain", -80.0f, 6.0f, 2.0f, 0.0f),                     -1, nullptr);
    mp.addChild (makeModParam ("attack",    0.0f, 10000.0f, 0.2313782125711441f, attack),    -1, nullptr);
    mp.addChild (makeModParam ("decay",     0.0f, 1000.0f,  1.0f, decay),                   -1, nullptr);
    mp.addChild (makeModParam ("sustain",   0.0f, 1.0f,     1.0f, sustain),                 -1, nullptr);
    mp.addChild (makeModParam ("release",   0.0f, 10000.0f, 0.2313782125711441f, release),  -1, nullptr);
    mp.addChild (makeModParam ("attackpower", -10.0f, 10.0f, 1.0f, 0.0f),                   -1, nullptr);
    mp.addChild (makeModParam ("delay",     0.0f, 1000.0f, 1.0f, 0.0f),                     -1, nullptr);
    vt.addChild (mp, -1, nullptr);

    // PARAM_DEFAULT
    juce::ValueTree pd ("PARAM_DEFAULT");
    pd.setProperty ("transpositionsVals",   "0.000000/", nullptr);
    pd.setProperty ("transpositionsSize",   1, nullptr);
    pd.setProperty ("transpositionsStates", "true false false false false false false false false false false false", nullptr);
    pd.setProperty ("transpositionsStatesSize", 12, nullptr);
    pd.setProperty ("accentsVals",          accVals, nullptr);
    pd.setProperty ("accentsSize",          (int) juce::StringArray::fromTokens(accVals, " ", "").size(), nullptr);
    pd.setProperty ("accentsStates",        accStates, nullptr);
    pd.setProperty ("accentsStatesSize",    (int) juce::StringArray::fromTokens(accStates, " ", "").size(), nullptr);
    pd.setProperty ("sustainlength_multipliersVals",   slVals, nullptr);
    pd.setProperty ("sustainlength_multipliersSize",   (int) juce::StringArray::fromTokens(slVals, " ", "").size(), nullptr);
    pd.setProperty ("sustainlength_multipliersStates", slStates, nullptr);
    pd.setProperty ("sustainlength_multipliersStatesSize", (int) juce::StringArray::fromTokens(slStates, " ", "").size(), nullptr);
    pd.setProperty ("beatlength_multipliersVals",   blVals, nullptr);
    pd.setProperty ("beatlength_multipliersSize",   (int) juce::StringArray::fromTokens(blVals, " ", "").size(), nullptr);
    pd.setProperty ("beatlength_multipliersStates", blStates, nullptr);
    pd.setProperty ("beatlength_multipliersStatesSize", (int) juce::StringArray::fromTokens(blStates, " ", "").size(), nullptr);
    pd.setProperty ("clustermin",    1.0f,     nullptr);
    pd.setProperty ("clustermax",    100000.0f,nullptr);
    pd.setProperty ("holdtimemin",   0.0f,     nullptr);
    pd.setProperty ("holdtimemax",   22143.0f, nullptr);
    vt.addChild (pd, -1, nullptr);

    // PORTs
    for (auto [chIdx, isIn] : std::initializer_list<std::pair<int,int>> {{4096,1},{0,0},{2,0}})
    {
        juce::ValueTree port ("PORT");
        port.setProperty ("nodeID", (int64_t) nodeID, nullptr);
        port.setProperty ("chIdx",  chIdx,             nullptr);
        port.setProperty ("isIn",   isIn,              nullptr);
        vt.addChild (port, -1, nullptr);
    }

    return vt;
}

// ---------------------------------------------------------------------------

juce::ValueTree LegacyGalleryImporter::convertNostalgic (const juce::XmlElement& el,
                                                          uint32_t nodeID,
                                                          const juce::String& uuid,
                                                          int x, int y,
                                                          const juce::String& name,
                                                          int instanceIndex)
{
    juce::ValueTree vt ("nostalgic");
    vt.setProperty ("type",    3,                               nullptr);
    vt.setProperty ("width",   156.0f,                          nullptr);
    vt.setProperty ("height",  79.2f,                           nullptr);
    vt.setProperty ("x_y",    juce::String(x) + ", " + juce::String(y), nullptr);
    vt.setProperty ("uuid",    uuid,                            nullptr);
    vt.setProperty ("soundset","syncglobal",                    nullptr);
    vt.setProperty ("nodeID",  (int64_t) nodeID,                nullptr);
    vt.setProperty ("name",    name,                            nullptr);
    vt.setProperty ("numIns",  1,                               nullptr);
    vt.setProperty ("numOuts", 2,                               nullptr);

    const juce::XmlElement* params = el.getChildByName ("params");

    float main = 0.0f, send = 0.0f, outputGain = 0.0f;
    float lengthMult = 1.0f, beatsToSkip = 3.0f;
    float clusterMin = 1.0f, clusterThresh = 150.0f;
    int   mode = 1;

    if (params != nullptr)
    {
        main        = gainToDb ((float) params->getDoubleAttribute ("gain", 0.0));
        lengthMult  = (float) params->getDoubleAttribute ("lengthMultiplier", 1.0);
        beatsToSkip = (float) params->getDoubleAttribute ("beatsToSkip", 3.0);
        clusterMin  = (float) params->getDoubleAttribute ("clusterMin", 1.0);
        clusterThresh = (float) params->getDoubleAttribute ("clusterThreshold", 150.0);
        mode        = params->getIntAttribute ("mode", 1);
    }

    vt.setProperty ("Main",         main,         nullptr);
    vt.setProperty ("Send",         send,         nullptr);
    vt.setProperty ("OutputGain",   outputGain,   nullptr);
    vt.setProperty ("noteOnGainNostalgic", 0.0f,  nullptr);
    vt.setProperty ("lengthMultiplier",  lengthMult,   nullptr);
    vt.setProperty ("beatsToSkip",       beatsToSkip,  nullptr);
    vt.setProperty ("ClusterMin",        clusterMin,   nullptr);
    vt.setProperty ("ClusterThresh",     clusterThresh,nullptr);
    vt.setProperty ("mode",              mode,         nullptr);
    vt.setProperty ("holdMin",  0.0f,     nullptr);
    vt.setProperty ("holdMax",  12000.0f, nullptr);
    vt.setProperty ("keyOnReset",   0,    nullptr);
    vt.setProperty ("UseTuning",    0,    nullptr);
    vt.setProperty ("notify",       1,    nullptr);

    // Reverse ADSR
    float rAttack = 30.0f, rDecay = 3.0f, rSustain = 1.0f, rRelease = 50.0f;
    if (params != nullptr)
    {
        const juce::XmlElement* radsr = params->getChildByName ("reverseADSR");
        if (radsr != nullptr)
        {
            rAttack  = (float) radsr->getDoubleAttribute ("f0", 30.0);
            rDecay   = (float) radsr->getDoubleAttribute ("f1", 3.0);
            rSustain = (float) radsr->getDoubleAttribute ("f2", 1.0);
            rRelease = (float) radsr->getDoubleAttribute ("f3", 50.0);
        }
    }
    vt.setProperty ("reverseAttack",  rAttack,  nullptr);
    vt.setProperty ("reverseDecay",   rDecay,   nullptr);
    vt.setProperty ("reverseSustain", rSustain, nullptr);
    vt.setProperty ("reverseRelease", rRelease, nullptr);

    // Undertow ADSR
    float uAttack = 50.0f, uDecay = 3.0f, uSustain = 1.0f, uRelease = 2000.0f;
    if (params != nullptr)
    {
        const juce::XmlElement* uadsr = params->getChildByName ("undertowADSR");
        if (uadsr != nullptr)
        {
            uAttack  = (float) uadsr->getDoubleAttribute ("f0", 50.0);
            uDecay   = (float) uadsr->getDoubleAttribute ("f1", 3.0);
            uSustain = (float) uadsr->getDoubleAttribute ("f2", 1.0);
            uRelease = (float) uadsr->getDoubleAttribute ("f3", 2000.0);
        }
    }
    vt.setProperty ("undertowAttack",  uAttack,  nullptr);
    vt.setProperty ("undertowDecay",   uDecay,   nullptr);
    vt.setProperty ("undertowSustain", uSustain, nullptr);
    vt.setProperty ("undertowRelease", uRelease, nullptr);

    // MODULATABLE_PARAMS
    juce::ValueTree mp ("MODULATABLE_PARAMS");
    mp.addChild (makeModParam ("Main",         -80.0f, 6.0f, 2.0f, main),         -1, nullptr);
    mp.addChild (makeModParam ("Send",         -80.0f, 6.0f, 2.0f, send),         -1, nullptr);
    mp.addChild (makeModParam ("OutputGain",   -80.0f, 6.0f, 2.0f, outputGain),   -1, nullptr);
    mp.addChild (makeModParam ("noteOnGainNostalgic", -80.0f, 6.0f, 2.0f, 0.0f),  -1, nullptr);
    mp.addChild (makeModParam ("NoteLengthMultiplier", 0.0f, 10.0f, 2.0f, lengthMult), -1, nullptr);
    mp.addChild (makeModParam ("BeatsToSkip",  0.0f, 10.0f, 1.0f, beatsToSkip),   -1, nullptr);
    mp.addChild (makeModParam ("ClusterMin",   1.0f, 10.0f, 1.0f, clusterMin),    -1, nullptr);
    mp.addChild (makeModParam ("ClusterThresh",0.0f, 1000.0f, 2.0f, clusterThresh),-1, nullptr);
    vt.addChild (mp, -1, nullptr);
    vt.addChild (juce::ValueTree ("PARAM_DEFAULT"), -1, nullptr);

    // PORTs
    for (auto [chIdx, isIn] : std::initializer_list<std::pair<int,int>> {{4096,1},{0,0},{2,0}})
    {
        juce::ValueTree port ("PORT");
        port.setProperty ("nodeID", (int64_t) nodeID, nullptr);
        port.setProperty ("chIdx",  chIdx,             nullptr);
        port.setProperty ("isIn",   isIn,              nullptr);
        vt.addChild (port, -1, nullptr);
    }

    return vt;
}

// ---------------------------------------------------------------------------

juce::ValueTree LegacyGalleryImporter::convertTuning (const juce::XmlElement& el,
                                                        uint32_t nodeID,
                                                        const juce::String& uuid,
                                                        int x, int y,
                                                        const juce::String& name,
                                                        int instanceIndex)
{
    juce::ValueTree vt ("tuning");
    vt.setProperty ("type",    6,                               nullptr);
    vt.setProperty ("width",   75.0f,                           nullptr);
    vt.setProperty ("height",  147.0f,                          nullptr);
    vt.setProperty ("x_y",    juce::String(x) + ", " + juce::String(y), nullptr);
    vt.setProperty ("uuid",    uuid,                            nullptr);
    vt.setProperty ("soundset","syncglobal",                    nullptr);
    vt.setProperty ("nodeID",  (int64_t) nodeID,                nullptr);
    vt.setProperty ("name",    name,                            nullptr);
    vt.setProperty ("numIns",  1,                               nullptr);

    const juce::XmlElement* params = el.getChildByName ("params");

    int   tuningSystem = 1; // 1 = Equal Temperament in new format
    int   fundamental  = 0;
    float offset       = 0.0f;
    float rate = 100.0f, drag = 0.98f;
    float tetherStiffness = 0.5f, intervalStiffness = 0.5f;
    float tetherWeightGlobal = 0.5f, tetherWeightSecGlobal = 0.1f;

    // Map old scale index to new tuningSystem index.
    // TuningSystemNames vector (tuning_systems.h) defines the new indices:
    // 0=Equal_Temperament, 1=Partial, 2=Just, 3=Duodene, 4=Otonal, 5=Utonal,
    // 6=Custom, 7=Pythagorean, 8=Grammateus, 9=Kirnberger_II, 10=Kirnberger_III,
    // 11=Werkmeister_III, 12=Quarter2Comma_Meantone, ...
    //
    // Old scale values (from old bitKlavier source):
    // 0=ET, 1=Partial, 2=ET, 3=JI, 4=Pythagorean, 5=Quarter-comma Meantone,
    // 6=Meantone (general), 7=Werckmeister III, 8=Custom
    int oldScale = 2; // Equal Temperament default

    if (params != nullptr)
    {
        oldScale    = params->getIntAttribute ("scale", 2);
        fundamental = params->getIntAttribute ("fundamental", 0);
        offset      = (float) params->getDoubleAttribute ("offset", 0.0);

        switch (oldScale)
        {
            case 0:  tuningSystem = 0;  break; // ET
            case 1:  tuningSystem = 1;  break; // Partial
            case 2:  tuningSystem = 0;  break; // ET
            case 3:  tuningSystem = 2;  break; // Just
            case 4:  tuningSystem = 7;  break; // Pythagorean
            case 5:  tuningSystem = 12; break; // Quarter-comma Meantone
            case 6:  tuningSystem = 12; break; // Meantone (closest match)
            case 7:  tuningSystem = 11; break; // Werckmeister III
            case 8:  tuningSystem = 6;  break; // Custom
            default: tuningSystem = 0;  break; // ET fallback
        }

        const juce::XmlElement* st = params->getChildByName ("springtuning");
        if (st != nullptr)
        {
            rate               = (float) st->getDoubleAttribute ("rate",              100.0);
            drag               = (float) st->getDoubleAttribute ("drag",              0.98);
            tetherStiffness    = (float) st->getDoubleAttribute ("tetherStiffness",   0.5);
            intervalStiffness  = (float) st->getDoubleAttribute ("intervalStiffness", 0.5);
            tetherWeightGlobal = (float) st->getDoubleAttribute ("tetherWeightGlobal", 0.5);
            tetherWeightSecGlobal = (float) st->getDoubleAttribute ("tetherWeightSecondaryGlobal", 0.1);
        }
    }

    vt.setProperty ("fundamental",    fundamental,    nullptr);
    vt.setProperty ("tuningSystem",   tuningSystem,   nullptr);
    vt.setProperty ("tuningType",     0,              nullptr);
    vt.setProperty ("lastNote",       0.0f,           nullptr);
    vt.setProperty ("semitonewidth",  100.0f,         nullptr);
    vt.setProperty ("reffundamental", 0,              nullptr);
    vt.setProperty ("octave",         4,              nullptr);
    vt.setProperty ("offset",         offset,         nullptr);
    vt.setProperty ("active",         1,              nullptr);

    // Spring tuning params
    vt.setProperty ("rate",                        rate,                  nullptr);
    vt.setProperty ("drag",                        drag,                  nullptr);
    vt.setProperty ("tetherStiffness",             tetherStiffness,       nullptr);
    vt.setProperty ("intervalStiffness",           intervalStiffness,     nullptr);
    vt.setProperty ("tetherWeightGlobal",          tetherWeightGlobal,    nullptr);
    vt.setProperty ("tetherWeightSecondaryGlobal", tetherWeightSecGlobal, nullptr);
    vt.setProperty ("fundamentalSetsTether",       1,                     nullptr);
    vt.setProperty ("tAdaptiveClusterThresh",      0.0f,                  nullptr);
    vt.setProperty ("tAdaptiveHistory",            1.0f,                  nullptr);
    vt.setProperty ("tAdaptiveIntervalScale",      1,                     nullptr);
    vt.setProperty ("tAdaptiveAnchorScale",        0,                     nullptr);
    vt.setProperty ("tAdaptiveAnchorFundamental",  0,                     nullptr);
    vt.setProperty ("tCurrentAdaptiveFundamental", 0,                     nullptr);
    vt.setProperty ("tAdaptiveInversional",        0,                     nullptr);
    vt.setProperty ("tReset",                      0,                     nullptr);
    vt.setProperty ("scaleId",                     2,                     nullptr);
    vt.setProperty ("intervalFundamental",         16,                    nullptr);
    vt.setProperty ("scaleIdTether",               0,                     nullptr);
    vt.setProperty ("tetherFundamental",           0,                     nullptr);
    vt.setProperty ("tCurrentSpringTuningFundamental", 0,                 nullptr);
    vt.setProperty ("tMapToInternal",              0,                     nullptr);

    // intervalWeight1..12 (from old springs s0..s11)
    juce::String springVals = "0.5 0.5 0.5 0.5 0.5 0.5 0.5 0.5 0.5 0.5 0.5 0.5";
    if (params != nullptr)
    {
        const juce::XmlElement* st = params->getChildByName ("springtuning");
        if (st != nullptr)
        {
            const juce::XmlElement* springs = st->getChildByName ("springs");
            if (springs != nullptr)
            {
                juce::String parsed = parseIndexedFloats (*springs, "s", 12);
                if (parsed.isNotEmpty()) springVals = parsed;
            }
        }
    }
    {
        juce::StringArray vals;
        vals.addTokens (springVals, " ", "");
        for (int i = 0; i < 12; ++i)
        {
            float v = (i < vals.size()) ? vals[i].getFloatValue() : 0.5f;
            vt.setProperty ("intervalWeight" + juce::String(i + 1), v, nullptr);
        }
    }
    // useLocalOrFundamental1..12 defaults
    for (int i = 1; i <= 12; ++i)
        vt.setProperty ("useLocalOrFundamental" + juce::String(i), 1, nullptr);

    // Custom scale → circularTuning (old f0..f11 are in semitones, new uses cents)
    juce::String circTuning = "0 0 0 0 0 0 0 0 0 0 0 0";
    if (params != nullptr)
    {
        const juce::XmlElement* cs = params->getChildByName ("customScale");
        if (cs != nullptr)
        {
            juce::String parts;
            for (int i = 0; i < 12; ++i)
            {
                juce::String key = "f" + juce::String(i);
                if (! cs->hasAttribute (key)) break;
                float semitones = (float) cs->getDoubleAttribute (key, 0.0);
                float cents = semitones * 100.0f;
                if (i > 0) parts += " ";
                parts += juce::String (cents, 4);
            }
            if (parts.isNotEmpty()) circTuning = parts;
        }
    }
    vt.setProperty ("circularTuning",        circTuning, nullptr);
    vt.setProperty ("circularTuning_custom", circTuning, nullptr);
    vt.setProperty ("absoluteTuning",        "",         nullptr);

    // Scala defaults
    vt.setProperty ("scalaScale", "! 12 Tone Equal Temperament.scl\n!\n12 Tone Equal Temperament | ED2-12\n 12\n!\n 100.00000\n 200.00000\n 300.00000\n 400.00000\n 500.00000\n 600.00000\n 700.00000\n 800.00000\n 900.00000\n 1000.00000\n 1100.00000\n 2/1\n", nullptr);
    vt.setProperty ("scalaKBM",   "! Default KBM file\n0\n0\n127\n60\n69\n440\n0\n", nullptr);

    // MODULATABLE_PARAMS
    juce::ValueTree mp ("MODULATABLE_PARAMS");
    mp.addChild (makeModParam ("offset",         -100.0f, 100.0f, 1.0f, offset),     -1, nullptr);
    mp.addChild (makeModParam ("semitonewidth",  -100.0f, 200.0f, 1.709511518478394f, 100.0f), -1, nullptr);
    mp.addChild (makeModParam ("tAdaptiveHistory",    1.0f, 8.0f,  1.0f, 1.0f),       -1, nullptr);
    mp.addChild (makeModParam ("tAdaptiveClusterThresh", 0.0f, 1000.0f, 1.0f, 0.0f), -1, nullptr);
    mp.addChild (makeModParam ("rate",           5.0f, 400.0f, 0.4864160418510437f, rate),    -1, nullptr);
    mp.addChild (makeModParam ("drag",           0.0f, 1.0f, 1.0f, drag),             -1, nullptr);
    mp.addChild (makeModParam ("intervalStiffness", 0.0f, 1.0f, 1.0f, intervalStiffness), -1, nullptr);
    mp.addChild (makeModParam ("tetherStiffness",   0.0f, 1.0f, 1.0f, tetherStiffness),   -1, nullptr);
    mp.addChild (makeModParam ("tetherWeightGlobal", 0.0f, 1.0f, 1.0f, tetherWeightGlobal), -1, nullptr);
    mp.addChild (makeModParam ("tetherWeightSecondaryGlobal", 0.0f, 1.0f, 1.0f, tetherWeightSecGlobal), -1, nullptr);
    for (int i = 1; i <= 12; ++i)
        mp.addChild (makeModParam ("intervalWeight" + juce::String(i), 0.0f, 1.0f, 1.0f, 0.5f), -1, nullptr);
    vt.addChild (mp, -1, nullptr);

    // PARAM_DEFAULT
    juce::ValueTree pd ("PARAM_DEFAULT");
    pd.setProperty ("fundamental",   fundamental,  nullptr);
    pd.setProperty ("tuningSystem",  tuningSystem, nullptr);
    pd.setProperty ("tuningType",    0,            nullptr);
    pd.setProperty ("absoluteTuning","",           nullptr);
    pd.setProperty ("circularTuning", circTuning,  nullptr);
    vt.addChild (pd, -1, nullptr);

    // PORT (tuning only has input port)
    {
        juce::ValueTree port ("PORT");
        port.setProperty ("nodeID", (int64_t) nodeID, nullptr);
        port.setProperty ("chIdx",  4096,              nullptr);
        port.setProperty ("isIn",   1,                 nullptr);
        vt.addChild (port, -1, nullptr);
    }

    return vt;
}

// ---------------------------------------------------------------------------

juce::ValueTree LegacyGalleryImporter::convertTempo (const juce::XmlElement& el,
                                                       uint32_t nodeID,
                                                       const juce::String& uuid,
                                                       int x, int y,
                                                       const juce::String& name,
                                                       int instanceIndex)
{
    juce::ValueTree vt ("tempo");
    vt.setProperty ("type",    7,                               nullptr);
    vt.setProperty ("width",   79.2f,                           nullptr);
    vt.setProperty ("height",  156.0f,                          nullptr);
    vt.setProperty ("x_y",    juce::String(x) + ", " + juce::String(y), nullptr);
    vt.setProperty ("uuid",    uuid,                            nullptr);
    vt.setProperty ("soundset","syncglobal",                    nullptr);
    vt.setProperty ("nodeID",  (int64_t) nodeID,                nullptr);
    vt.setProperty ("name",    name,                            nullptr);
    vt.setProperty ("numIns",  1,                               nullptr);

    const juce::XmlElement* params = el.getChildByName ("params");

    float tempo        = 120.0f;
    float subdivisions = 1.0f;
    float history      = 1.0f;

    if (params != nullptr)
    {
        tempo        = (float) params->getDoubleAttribute ("tempo",        120.0);
        subdivisions = (float) params->getDoubleAttribute ("subdivisions", 1.0);
        history      = (float) params->getDoubleAttribute ("at1History",   1.0);
    }

    vt.setProperty ("tempo",        tempo,        nullptr);
    vt.setProperty ("subdivisions", subdivisions, nullptr);
    vt.setProperty ("history",      history,      nullptr);
    vt.setProperty ("determinesCluster", 0,       nullptr);
    vt.setProperty ("holdtimemin",  0.0f,         nullptr);
    vt.setProperty ("holdtimemax",  12000.0f,     nullptr);
    vt.setProperty ("holdTimeMinParam",  0.0f,    nullptr);
    vt.setProperty ("holdTimeMaxParam",  12000.0f,nullptr);
    vt.setProperty ("lastHoldTimeParam", 0.0f,    nullptr);

    // MODULATABLE_PARAMS
    juce::ValueTree mp ("MODULATABLE_PARAMS");
    mp.addChild (makeModParam ("tempo",        40.0f, 1120.0f, 1.0f, tempo),        -1, nullptr);
    mp.addChild (makeModParam ("subdivisions", 0.009999999776482582f, 32.0f, 0.9995490908622742f, subdivisions), -1, nullptr);
    mp.addChild (makeModParam ("history",      1.0f, 10.0f, 0.854755699634552f, history), -1, nullptr);
    vt.addChild (mp, -1, nullptr);

    // PARAM_DEFAULT
    juce::ValueTree pd ("PARAM_DEFAULT");
    pd.setProperty ("holdtimemin", 0.0f,    nullptr);
    pd.setProperty ("holdtimemax", 12000.0f,nullptr);
    vt.addChild (pd, -1, nullptr);

    // PORT (input only)
    {
        juce::ValueTree port ("PORT");
        port.setProperty ("nodeID", (int64_t) nodeID, nullptr);
        port.setProperty ("chIdx",  4096,              nullptr);
        port.setProperty ("isIn",   1,                 nullptr);
        vt.addChild (port, -1, nullptr);
    }

    return vt;
}

// ---------------------------------------------------------------------------

juce::ValueTree LegacyGalleryImporter::convertKeymap (const juce::XmlElement& el,
                                                        uint32_t nodeID,
                                                        const juce::String& uuid,
                                                        int x, int y,
                                                        const juce::String& name,
                                                        int instanceIndex)
{
    juce::ValueTree vt ("keymap");
    vt.setProperty ("type",    0,                               nullptr);
    vt.setProperty ("width",   111.0f,                          nullptr);
    vt.setProperty ("height",  63.0f,                           nullptr);
    vt.setProperty ("x_y",    juce::String(x) + ", " + juce::String(y), nullptr);
    vt.setProperty ("uuid",    uuid,                            nullptr);
    vt.setProperty ("soundset","syncglobal",                    nullptr);
    vt.setProperty ("nodeID",  (int64_t) nodeID,                nullptr);
    vt.setProperty ("name",    name,                            nullptr);
    vt.setProperty ("numOuts", 1,                               nullptr);

    // Velocity curve defaults
    vt.setProperty ("velocityCurveAsymWarp", 1.0f, nullptr);
    vt.setProperty ("velocityCurveSymWarp",  1.0f, nullptr);
    vt.setProperty ("velocityCurveScale",    1.0f, nullptr);
    vt.setProperty ("velocityCurveOffset",   0.0f, nullptr);
    vt.setProperty ("velocityCurveInvert",   0,    nullptr);
    vt.setProperty ("velocitymin",           0.0f, nullptr);
    vt.setProperty ("velocitymax",           128.0f,nullptr);

    // Keys: old format uses individual k0, k1, ... attributes
    // New format uses space-separated keyOn attribute
    juce::String keyOn;
    for (int i = 0; i < 128; ++i)
    {
        juce::String key = "k" + juce::String (i);
        if (! el.hasAttribute (key)) break;
        if (keyOn.isNotEmpty()) keyOn += " ";
        keyOn += el.getStringAttribute (key);
    }

    // If no individual keys found, default to all keys (0-127)
    if (keyOn.isEmpty())
    {
        for (int i = 0; i < 128; ++i)
        {
            if (i > 0) keyOn += " ";
            keyOn += juce::String (i);
        }
    }
    vt.setProperty ("keyOn", keyOn, nullptr);

    // MODULATABLE_PARAMS
    juce::ValueTree mp ("MODULATABLE_PARAMS");
    mp.addChild (makeModParam ("velocityCurveAsymWarp", 0.0f, 10.0f, 0.3010299801826477f, 1.0f), -1, nullptr);
    mp.addChild (makeModParam ("velocityCurveSymWarp",  0.0f, 5.0f,  0.4306765496730804f, 1.0f), -1, nullptr);
    mp.addChild (makeModParam ("velocityCurveScale",    0.0f, 10.0f, 0.3010299801826477f, 1.0f), -1, nullptr);
    mp.addChild (makeModParam ("velocityCurveOffset",   -1.0f, 1.0f, 1.0f, 0.0f),               -1, nullptr);
    vt.addChild (mp, -1, nullptr);
    vt.addChild (juce::ValueTree ("PARAM_DEFAULT"), -1, nullptr);

    // PORT (output only for keymap)
    {
        juce::ValueTree port ("PORT");
        port.setProperty ("nodeID", (int64_t) nodeID, nullptr);
        port.setProperty ("chIdx",  4096,              nullptr);
        port.setProperty ("isIn",   0,                 nullptr);
        vt.addChild (port, -1, nullptr);
    }

    // Default midi input
    {
        juce::ValueTree mi ("midiInput");
        mi.setProperty ("midiDeviceId", "defaultMidiInput", nullptr);
        vt.addChild (mi, -1, nullptr);
    }

    return vt;
}

// ---------------------------------------------------------------------------

juce::ValueTree LegacyGalleryImporter::convertBlendronic (const juce::XmlElement& el,
                                                            uint32_t nodeID,
                                                            const juce::String& uuid,
                                                            int x, int y,
                                                            const juce::String& name,
                                                            int instanceIndex)
{
    juce::ValueTree vt ("blendronic");
    vt.setProperty ("type",    4,                               nullptr);
    vt.setProperty ("width",   156.0f,                          nullptr);
    vt.setProperty ("height",  79.2f,                           nullptr);
    vt.setProperty ("x_y",    juce::String(x) + ", " + juce::String(y), nullptr);
    vt.setProperty ("uuid",    uuid,                            nullptr);
    vt.setProperty ("soundset","syncglobal",                    nullptr);
    vt.setProperty ("nodeID",  (int64_t) nodeID,                nullptr);
    vt.setProperty ("name",    name,                            nullptr);
    vt.setProperty ("numIns",  1,                               nullptr);
    vt.setProperty ("numOuts", 2,                               nullptr);

    vt.setProperty ("InputGain",  0.0f, nullptr);
    vt.setProperty ("Send",       0.0f, nullptr);
    vt.setProperty ("OutputGain", 0.0f, nullptr);
    vt.setProperty ("updateUIState", 0,  nullptr);

    const juce::XmlElement* params = el.getChildByName ("params");

    // Beat lengths
    juce::String beatVals = "4.0", beatStates = "true";
    if (params != nullptr)
    {
        const juce::XmlElement* bEl  = params->getChildByName ("beats");
        const juce::XmlElement* bStEl = params->getChildByName ("beatsStates");
        if (bEl != nullptr)
        {
            beatVals   = parseIndexedFloats (*bEl,   "f", 12);
            if (bStEl != nullptr)
                beatStates = parseIndexedBools (*bStEl, "b", 12);
        }
    }
    vt.setProperty ("beatLengthsVals",         beatVals,   nullptr);
    vt.setProperty ("beatLengthsSize",         (int) juce::StringArray::fromTokens(beatVals, " ", "").size(), nullptr);
    vt.setProperty ("beatLengthsStates",       beatStates, nullptr);
    vt.setProperty ("beatLengthsStatesSize",   (int) juce::StringArray::fromTokens(beatStates, " ", "").size(), nullptr);

    // Delay lengths
    juce::String delVals = "4.0", delStates = "true";
    if (params != nullptr)
    {
        const juce::XmlElement* dEl  = params->getChildByName ("delayLengths");
        const juce::XmlElement* dStEl = params->getChildByName ("delayLengthsStates");
        if (dEl != nullptr)
        {
            delVals   = parseIndexedFloats (*dEl,   "f", 12);
            if (dStEl != nullptr)
                delStates = parseIndexedBools (*dStEl, "b", 12);
        }
    }
    vt.setProperty ("delayLengthsVals",        delVals,   nullptr);
    vt.setProperty ("delayLengthsSize",        (int) juce::StringArray::fromTokens(delVals, " ", "").size(), nullptr);
    vt.setProperty ("delayLengthsStates",      delStates, nullptr);
    vt.setProperty ("delayLengthsStatesSize",  (int) juce::StringArray::fromTokens(delStates, " ", "").size(), nullptr);

    // Smooth lengths
    juce::String smVals = "50.0", smStates = "true";
    if (params != nullptr)
    {
        const juce::XmlElement* smEl  = params->getChildByName ("smoothLengths");
        const juce::XmlElement* smStEl = params->getChildByName ("smoothLengthsStates");
        if (smEl != nullptr)
        {
            smVals   = parseIndexedFloats (*smEl,   "f", 12);
            if (smStEl != nullptr)
                smStates = parseIndexedBools (*smStEl, "b", 12);
        }
    }
    vt.setProperty ("smoothingTimesVals",      smVals,   nullptr);
    vt.setProperty ("smoothingTimesSize",      (int) juce::StringArray::fromTokens(smVals, " ", "").size(), nullptr);
    vt.setProperty ("smoothingTimesStates",    smStates, nullptr);
    vt.setProperty ("smoothingTimesStatesSize",(int) juce::StringArray::fromTokens(smStates, " ", "").size(), nullptr);

    // Feedback coefficients
    juce::String fbVals = "0.95", fbStates = "true";
    if (params != nullptr)
    {
        const juce::XmlElement* fbEl  = params->getChildByName ("feedbackCoefficients");
        const juce::XmlElement* fbStEl = params->getChildByName ("feedbackCoefficientsStates");
        if (fbEl != nullptr)
        {
            fbVals   = parseIndexedFloats (*fbEl,   "f", 12);
            if (fbStEl != nullptr)
                fbStates = parseIndexedBools (*fbStEl, "b", 12);
        }
    }
    vt.setProperty ("feedbackCoeffsVals",      fbVals,   nullptr);
    vt.setProperty ("feedbackCoeffsSize",      (int) juce::StringArray::fromTokens(fbVals, " ", "").size(), nullptr);
    vt.setProperty ("feedbackCoeffsStates",    fbStates, nullptr);
    vt.setProperty ("feedbackCoeffsStatesSize",(int) juce::StringArray::fromTokens(fbStates, " ", "").size(), nullptr);

    // MODULATABLE_PARAMS
    juce::ValueTree mp ("MODULATABLE_PARAMS");
    mp.addChild (makeModParam ("InputGain",  -80.0f, 6.0f, 2.0f, 0.0f), -1, nullptr);
    mp.addChild (makeModParam ("Send",       -80.0f, 6.0f, 2.0f, 0.0f), -1, nullptr);
    mp.addChild (makeModParam ("OutputGain", -80.0f, 6.0f, 2.0f, 0.0f), -1, nullptr);
    vt.addChild (mp, -1, nullptr);
    vt.addChild (juce::ValueTree ("PARAM_DEFAULT"), -1, nullptr);

    // PORTs
    for (auto [chIdx, isIn] : std::initializer_list<std::pair<int,int>> {{4096,1},{0,0},{2,0}})
    {
        juce::ValueTree port ("PORT");
        port.setProperty ("nodeID", (int64_t) nodeID, nullptr);
        port.setProperty ("chIdx",  chIdx,             nullptr);
        port.setProperty ("isIn",   isIn,              nullptr);
        vt.addChild (port, -1, nullptr);
    }

    return vt;
}

// ---------------------------------------------------------------------------
// Main import entry point
// ---------------------------------------------------------------------------

juce::ValueTree LegacyGalleryImporter::importFromFile (const juce::File& xmlFile,
                                                         juce::String& errorMessage)
{
    // Parse the old XML file
    auto xmlDoc = juce::parseXMLIfTagMatches (xmlFile, "gallery");
    if (xmlDoc == nullptr)
    {
        // Try plain parse (no tag matching) in case the file has a different root
        xmlDoc = juce::parseXML (xmlFile);
    }
    if (xmlDoc == nullptr)
    {
        errorMessage = "Could not parse file as XML: " + xmlFile.getFullPathName();
        return {};
    }
    if (xmlDoc->getTagName().toLowerCase() != "gallery")
    {
        errorMessage = "File does not appear to be a bitKlavier 1 gallery (expected root element 'gallery'): "
                       + xmlFile.getFullPathName();
        return {};
    }

    // Create the root GALLERY ValueTree
    juce::ValueTree gallery ("GALLERY");
    gallery.setProperty ("soundset", "Yamaha_Default", nullptr);
    gallery.setProperty ("uuid",     newUUID(),         nullptr);

    // Global settings from <general>
    const juce::XmlElement* general = xmlDoc->getChildByName ("general");
    if (general != nullptr)
    {
        double a440 = general->getDoubleAttribute ("tuningFund", 440.0);
        double tempoMult = general->getDoubleAttribute ("tempoMultiplier", 1.0);
        gallery.setProperty ("global_A440",             a440,      nullptr);
        gallery.setProperty ("global_tempo_multiplier", tempoMult, nullptr);
    }

    // Build lookup maps: old preparation type tag → map of Id → XmlElement*
    // We support: direct, synchronic, nostalgic, tuning, tempo, keymap, blendronic
    // Resonance is silently skipped.
    using PrepMap = std::map<int, const juce::XmlElement*>;
    std::map<juce::String, PrepMap> prepsByTagAndId;

    for (auto* child = xmlDoc->getFirstChildElement(); child != nullptr;
         child = child->getNextElement())
    {
        juce::String tag = child->getTagName().toLowerCase();
        if (tag == "direct" || tag == "synchronic" || tag == "nostalgic" ||
            tag == "tuning" || tag == "tempo" || tag == "keymap" || tag == "blendronic")
        {
            int id = child->getIntAttribute ("Id", -1);
            prepsByTagAndId[tag][id] = child;
        }
        // Skip: resonance, general, equalizer, compressor (handled separately or ignored)
    }

    // Instance counter per preparation type (for naming)
    std::map<juce::String, int> instanceCounters;

    // Process each <piano>
    int pianoIndex = 0;
    for (auto* pianoEl = xmlDoc->getChildByName ("piano"); pianoEl != nullptr;
         pianoEl = pianoEl->getNextElementWithTagName ("piano"))
    {
        juce::String pianoName = pianoEl->getStringAttribute ("name", "Piano " + juce::String (pianoIndex + 1));

        juce::ValueTree pianoVT ("PIANO");
        pianoVT.setProperty ("isActive", (pianoIndex == 0) ? 1 : 0, nullptr);
        pianoVT.setProperty ("name",     pianoName,                  nullptr);

        juce::ValueTree preparations ("PREPARATIONS");
        juce::ValueTree connections   ("CONNECTIONS");
        juce::ValueTree modConnections ("MODCONNECTIONS");

        // Maps (oldType, oldId, pianoIdx) → nodeID for this piano instance
        // Key: (tag, id)
        struct InstKey { juce::String tag; int id; };
        // Use map indexed by tag+id string
        std::map<juce::String, uint32_t> instToNodeID;
        std::map<juce::String, juce::String> instToUUID;

        // Helper to get or create a new-format preparation for a given (type, id)
        // Returns the nodeID, and adds the prep to `preparations` if not already added.
        // Returns 0 if the type is not supported.
        auto getOrCreatePrep = [&] (int oldType, int oldId, int itemX, int itemY) -> uint32_t
        {
            juce::String tag = tagForOldType (oldType);
            if (tag.isEmpty()) return 0; // unsupported type (e.g. resonance)

            juce::String instKey = tag + "_" + juce::String (oldId);
            auto it = instToNodeID.find (instKey);
            if (it != instToNodeID.end())
                return it->second;

            // Look up the prep in the global map
            const juce::XmlElement* prepEl = nullptr;
            auto tagIt = prepsByTagAndId.find (tag);
            if (tagIt != prepsByTagAndId.end())
            {
                auto idIt = tagIt->second.find (oldId);
                if (idIt != tagIt->second.end())
                    prepEl = idIt->second;
                // Fallback to Id=-1 (default prep) if not found
                if (prepEl == nullptr)
                {
                    auto defIt = tagIt->second.find (-1);
                    if (defIt != tagIt->second.end())
                        prepEl = defIt->second;
                }
            }

            if (prepEl == nullptr)
            {
                // Cannot find any definition — skip
                return 0;
            }

            // Generate identifiers for this instance.
            // nodeID MUST equal UUID.getTimeLow() because PreparationList derives the
            // AudioProcessorGraph NodeID from the UUID when adding the node, and writes
            // it back to IDs::nodeID. Connections are looked up by nodeID, so if it
            // doesn't match the UUID-derived value the connections won't resolve.
            juce::String uuid = newUUID();
            uint32_t nodeID = juce::Uuid (uuid).getTimeLow();
            instToNodeID[instKey] = nodeID;
            instToUUID[instKey]   = uuid;

            int& counter = instanceCounters[tag];
            ++counter;
            juce::String instName = tag + " " + juce::String (counter);

            // Create the new-format preparation
            juce::ValueTree prepVT;
            if      (tag == "direct")     prepVT = convertDirect     (*prepEl, nodeID, uuid, itemX, itemY, instName, counter);
            else if (tag == "synchronic") prepVT = convertSynchronic (*prepEl, nodeID, uuid, itemX, itemY, instName, counter);
            else if (tag == "nostalgic")  prepVT = convertNostalgic  (*prepEl, nodeID, uuid, itemX, itemY, instName, counter);
            else if (tag == "tuning")     prepVT = convertTuning      (*prepEl, nodeID, uuid, itemX, itemY, instName, counter);
            else if (tag == "tempo")      prepVT = convertTempo       (*prepEl, nodeID, uuid, itemX, itemY, instName, counter);
            else if (tag == "keymap")     prepVT = convertKeymap      (*prepEl, nodeID, uuid, itemX, itemY, instName, counter);
            else if (tag == "blendronic") prepVT = convertBlendronic  (*prepEl, nodeID, uuid, itemX, itemY, instName, counter);

            if (prepVT.isValid())
                preparations.addChild (prepVT, -1, nullptr);

            return nodeID;
        };

        // Walk the piano's <item> children.
        // Each <item> wrapper contains one inner <item> (the "self" node) and
        // a <connections> child (what this node drives).
        for (auto* itemWrapper = pianoEl->getFirstChildElement(); itemWrapper != nullptr;
             itemWrapper = itemWrapper->getNextElement())
        {
            if (itemWrapper->getTagName().toLowerCase() != "item") continue;

            // First child = the "self" <item>
            const juce::XmlElement* selfItem = itemWrapper->getFirstChildElement();
            if (selfItem == nullptr || selfItem->getTagName().toLowerCase() != "item") continue;

            int selfType = selfItem->getIntAttribute ("type", -1);
            int selfId   = selfItem->getIntAttribute ("Id",   -1);
            int selfX    = (int) selfItem->getDoubleAttribute ("X", 0.0);
            int selfY    = (int) selfItem->getDoubleAttribute ("Y", 0.0);

            // Skip default preparations (Id=-1) that aren't actually in the layout
            // (they appear only as non-active targets) — let them be created on demand.

            uint32_t selfNodeID = getOrCreatePrep (selfType, selfId, selfX, selfY);
            if (selfNodeID == 0) continue; // unsupported type

            // Walk <connections>
            const juce::XmlElement* connsEl = itemWrapper->getChildByName ("connections");
            if (connsEl == nullptr) continue;

            for (auto* connItem = connsEl->getFirstChildElement(); connItem != nullptr;
                 connItem = connItem->getNextElement())
            {
                if (connItem->getTagName().toLowerCase() != "item") continue;

                int connType = connItem->getIntAttribute ("type", -1);
                int connId   = connItem->getIntAttribute ("Id",   -1);
                int connX    = (int) connItem->getDoubleAttribute ("X", 0.0);
                int connY    = (int) connItem->getDoubleAttribute ("Y", 0.0);

                uint32_t connNodeID = getOrCreatePrep (connType, connId, connX, connY);
                if (connNodeID == 0) continue;

                // Decide connection type
                if (selfType == OldTuning)
                {
                    // Tuning → destination: TUNINGCONNECTION
                    juce::ValueTree tc ("TUNINGCONNECTION");
                    tc.setProperty ("isMod", 0,                    nullptr);
                    tc.setProperty ("src",   (int64_t) selfNodeID, nullptr);
                    tc.setProperty ("dest",  (int64_t) connNodeID, nullptr);
                    modConnections.addChild (tc, -1, nullptr);
                }
                else if (selfType == OldTempo)
                {
                    // Tempo → destination: TEMPOCONNECTION
                    juce::ValueTree tc ("TEMPOCONNECTION");
                    tc.setProperty ("isMod", 0,                    nullptr);
                    tc.setProperty ("src",   (int64_t) selfNodeID, nullptr);
                    tc.setProperty ("dest",  (int64_t) connNodeID, nullptr);
                    modConnections.addChild (tc, -1, nullptr);
                }
                else
                {
                    // All other connections: standard audio CONNECTION
                    juce::ValueTree conn ("CONNECTION");
                    conn.setProperty ("isMod",    0,                    nullptr);
                    conn.setProperty ("src",      (int64_t) selfNodeID, nullptr);
                    conn.setProperty ("srcIdx",   4096,                  nullptr);
                    conn.setProperty ("dest",     (int64_t) connNodeID, nullptr);
                    conn.setProperty ("destIdx",  4096,                  nullptr);
                    conn.setProperty ("isSelected", 0,                  nullptr);
                    connections.addChild (conn, -1, nullptr);
                }
            }
        }

        // --- Add Tempo preparation ---
        // In the old format, Tempo is defined at the gallery level and is implicitly
        // connected to all Synchronic preparations in the piano. It never appears as
        // a piano <item type=7>, so we must add it explicitly.
        // Use the first non-default (Id != -1) tempo definition, if any.
        {
            const juce::XmlElement* tempoEl = nullptr;
            auto tempoIt = prepsByTagAndId.find ("tempo");
            if (tempoIt != prepsByTagAndId.end())
            {
                // prefer first Id > 0
                for (auto& kv : tempoIt->second)
                {
                    if (kv.first >= 0)
                    {
                        tempoEl = kv.second;
                        break;
                    }
                }
                // fallback to Id=-1 default
                if (tempoEl == nullptr)
                {
                    auto defIt = tempoIt->second.find (-1);
                    if (defIt != tempoIt->second.end())
                        tempoEl = defIt->second;
                }
            }

            if (tempoEl != nullptr)
            {
                // Collect all synchronic nodeIDs that were created for this piano
                std::vector<uint32_t> syncNodeIDs;
                for (auto& kv : instToNodeID)
                {
                    if (kv.first.startsWith ("synchronic_"))
                        syncNodeIDs.push_back (kv.second);
                }

                if (! syncNodeIDs.empty())
                {
                    // Create the tempo preparation (use getOrCreatePrep so it's idempotent)
                    // Place it at a sensible default position (near the top)
                    uint32_t tempoNodeID = getOrCreatePrep (OldTempo, tempoEl->getIntAttribute ("Id", -1), 680, 115);

                    if (tempoNodeID != 0)
                    {
                        for (uint32_t syncID : syncNodeIDs)
                        {
                            juce::ValueTree tc ("TEMPOCONNECTION");
                            tc.setProperty ("isMod", 0,                     nullptr);
                            tc.setProperty ("src",   (int64_t) tempoNodeID, nullptr);
                            tc.setProperty ("dest",  (int64_t) syncID,      nullptr);
                            modConnections.addChild (tc, -1, nullptr);
                        }
                    }
                }
            }
        }

        pianoVT.addChild (preparations,   -1, nullptr);
        pianoVT.addChild (connections,     -1, nullptr);
        pianoVT.addChild (modConnections,  -1, nullptr);
        gallery.addChild (pianoVT, -1, nullptr);

        ++pianoIndex;
    }

    // Add default bus processors
    gallery.addChild (makeDefaultBusEQ(),         -1, nullptr);
    gallery.addChild (makeDefaultBusCompressor(), -1, nullptr);
    gallery.addChild (juce::ValueTree ("MODULATABLE_PARAMS"), -1, nullptr);

    return gallery;
}
