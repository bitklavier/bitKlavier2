// Copyright (C) 2022-2025 Dan Trueman
// SPDX-License-Identifier: GPL-3.0-or-later

/*
 * ReverbProcessor — Hall reverb bus processor for bitKlavier
 * Based on Dragonfly Hall Reverb by Michael Willis and Rob van den Berg
 * https://github.com/michaelwillis/dragonfly-reverb  (GPL-3.0)
 * DSP uses freeverb3 (Jezar at Dreampoint / Teru Kamogashira, GPL-2.0+)
 */

#pragma once
#include "Identifiers.h"
#include "PluginBase.h"
#include "utils.h"
#include <PreparationStateImpl.h>
#include <chowdsp_sources/chowdsp_sources.h>
#ifndef LIBFV3_FLOAT
#define LIBFV3_FLOAT
#endif
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdynamic-exception-spec"
#pragma clang diagnostic ignored "-Wdeprecated"
#include "freeverb/earlyref.hpp"
#include "freeverb/zrev2.hpp"
#pragma clang diagnostic pop
// fv3_defs.h defines isfinite/isnormal/fpclassify as macros that wrap std::,
// which breaks any TU that uses std::isfinite etc. after including this header.
#undef isfinite
#undef isnormal
#undef fpclassify

// ──────────────────────────────────────────────────────────────────────
// Preset data (copied from Dragonfly Hall DistrhoPluginInfo.h)
// order: dry, early, late, size, width, predelay, diffuse,
//        lowCut, lowXover, lowMult, highCut, highXover, highMult,
//        spin, wander, decay, earlySend, modulation
// ──────────────────────────────────────────────────────────────────────
static constexpr int kReverbParamCount = 18;

struct ReverbPresetEntry {
    const char* bankName;
    const char* presetName;
    float params[kReverbParamCount];
};

static const ReverbPresetEntry kReverbPresets[25] = {
    // Rooms
    {"Rooms", "Bright Room",            { 80,10,20,10, 90,  4,90, 4,  500,0.80,16000, 7900,0.75,1.0,25,0.6,20,30 }},
    {"Rooms", "Clear Room",             { 80,10,20,10, 90,  4,90, 4,  500,0.90,13000, 5800,0.50,1.0,25,0.6,20,30 }},
    {"Rooms", "Dark Room",              { 80,10,20,10, 90,  4,50, 4,  500,1.20, 7300, 4900,0.35,1.0,25,0.7,20,30 }},
    {"Rooms", "Small Chamber",          { 80,10,20,16, 80,  8,70, 4,  500,1.10, 8200, 5500,0.35,1.2,10,0.8,20,20 }},
    {"Rooms", "Large Chamber",          { 80,10,20,20, 80,  8,90, 4,  500,1.30, 7000, 4900,0.25,1.8,12,1.0,20,20 }},
    // Studios
    {"Studios", "Acoustic Studio",      { 80,10,20,12, 90,  8,75, 4,  450,1.50, 7600, 4900,0.80,2.5, 7,0.8,20,20 }},
    {"Studios", "Electric Studio",      { 80,10,20,12, 90,  6,45, 4,  250,1.25, 7600, 5800,0.70,2.5, 7,0.9,20,30 }},
    {"Studios", "Percussion Studio",    { 80,10,20,12, 90,  6,30,20,  200,1.75, 5800, 5200,0.45,2.5, 7,0.7,20,10 }},
    {"Studios", "Piano Studio",         { 80,10,20,12, 80,  8,40,20,  600,1.50, 8200, 5800,0.50,2.8,10,0.7,20,15 }},
    {"Studios", "Vocal Studio",         { 80,10,20,12, 90,  0,60, 4,  400,1.20, 5800, 5200,0.40,2.5, 7,0.8,20,10 }},
    // Small Halls
    {"Small Halls", "Small Bright Hall",     { 80,10,20,24, 80, 12,90, 4,  400,1.10,11200, 6250,0.75,2.5,13,1.3,20,15 }},
    {"Small Halls", "Small Clear Hall",      { 80,10,20,24,100,  4,90, 4,  500,1.30, 7600, 5500,0.50,3.3,15,1.3,20,15 }},
    {"Small Halls", "Small Dark Hall",       { 80,10,20,24,100, 12,60, 4,  500,1.50, 5800, 4000,0.35,2.5,10,1.5,20,15 }},
    {"Small Halls", "Small Percussion Hall", { 80,10,20,24, 80, 12,40,20,  250,2.00, 5200, 4000,0.35,2.0,13,1.1,20,10 }},
    {"Small Halls", "Small Vocal Hall",      { 80,10,20,24, 80,  4,60, 4,  500,1.25, 6250, 5200,0.35,3.1,15,1.2,20,10 }},
    // Medium Halls
    {"Medium Halls", "Medium Bright Hall",     { 80,10,20,30,100, 18,90, 4,  400,1.25,10000, 6400,0.60,2.9,15,1.6,20,15 }},
    {"Medium Halls", "Medium Clear Hall",      { 80,10,20,30,100,  8,90, 4,  500,1.50, 7600, 5500,0.50,2.9,15,1.7,20,15 }},
    {"Medium Halls", "Medium Dark Hall",       { 80,10,20,30,100, 18,60, 4,  500,1.75, 5800, 4000,0.40,2.9,15,1.8,20,15 }},
    {"Medium Halls", "Medium Percussion Hall", { 80,10,20,30, 80, 12,40,20,  300,2.00, 5200, 4000,0.35,2.0,12,1.2,20,10 }},
    {"Medium Halls", "Medium Vocal Hall",      { 80,10,20,32, 80,  8,75, 4,  600,1.50, 5800, 5200,0.40,2.8,16,1.3,20,10 }},
    // Large Halls
    {"Large Halls", "Large Bright Hall",  { 80,10,20,40,100, 20,90, 4,  400,1.50, 8200, 5800,0.50,2.1,20,2.5,20,15 }},
    {"Large Halls", "Large Clear Hall",   { 80,10,20,40,100, 12,80, 4,  550,2.00, 8200, 5200,0.40,2.1,20,2.8,20,15 }},
    {"Large Halls", "Large Dark Hall",    { 80,10,20,40,100, 20,60, 4,  600,2.50, 6250, 2800,0.20,2.1,20,3.0,20,15 }},
    {"Large Halls", "Large Vocal Hall",   { 80,10,20,40, 80, 12,80, 4,  700,2.25, 6250, 4600,0.30,2.1,17,2.4,20,10 }},
    {"Large Halls", "Great Hall",         { 80,10,20,50, 90, 20,95, 4,  750,2.50, 5500, 4000,0.30,2.6,22,3.8,20,15 }},
};

// Default: Small Clear Hall (index 11, 0-based)
static constexpr int kReverbDefaultPreset = 12; // 1-based: preset 12 = Small Clear Hall

// ──────────────────────────────────────────────────────────────────────
// ReverbParams
// ──────────────────────────────────────────────────────────────────────
struct ReverbParams : chowdsp::ParamHolder
{
    float gainRangeStart = -80.0f;
    float gainRangeEnd   =   6.0f;
    float gainSkew       =   2.0f;

    ReverbParams (const juce::ValueTree& /*v*/) : chowdsp::ParamHolder ("reverb")
    {
        add (activeReverb,
             inputGain, externalGain, outputGain, outputSend,
             dryLevel, earlyLevel, lateLevel,
             size, width, predelay, diffuse,
             lowCut, lowXover, lowMult,
             highCut, highXover, highMult,
             spin, wander, decay, earlySend, modulation);

        doForAllParameters ([this] (auto& param, size_t) {
            if (auto* sliderParam = dynamic_cast<chowdsp::FloatParameter*> (&param))
                if (sliderParam->supportsMonophonicModulation())
                    modulatableParams.emplace_back(sliderParam);
        });
    }

    chowdsp::BoolParameter::Ptr activeReverb {
        juce::ParameterID { "reverbActive", 100 }, "Active", false };

    chowdsp::GainDBParameter::Ptr inputGain {
        juce::ParameterID { "reverbInputGain", 100 }, "Input Gain",
        juce::NormalisableRange { gainRangeStart, gainRangeEnd, 0.0f, gainSkew, false }, 0.0f, true };
    chowdsp::GainDBParameter::Ptr externalGain {
        juce::ParameterID { "reverbExternalGain", 100 }, "External Gain",
        juce::NormalisableRange { gainRangeStart, gainRangeEnd, 0.0f, gainSkew, false }, gainRangeStart, true };
    chowdsp::GainDBParameter::Ptr outputGain {
        juce::ParameterID { "reverbOutputGain", 100 }, "Output Gain",
        juce::NormalisableRange { gainRangeStart, gainRangeEnd, 0.0f, gainSkew, false }, 0.0f, true };

    chowdsp::GainDBParameter::Ptr outputSend {
        juce::ParameterID { "reverbSend", 100 }, "Send",
        juce::NormalisableRange { gainRangeStart, gainRangeEnd, 0.0f, gainSkew, false }, 0.0f, true };

    std::tuple<std::atomic<float>, std::atomic<float>> inputLevels;
    std::tuple<std::atomic<float>, std::atomic<float>> outputLevels;
    std::tuple<std::atomic<float>, std::atomic<float>> sendLevels;
    std::tuple<std::atomic<float>, std::atomic<float>> externalLevels;

    // 18 reverb parameters
    chowdsp::FloatParameter::Ptr dryLevel {
        juce::ParameterID { "reverbDry", 100 }, "Dry Level",
        juce::NormalisableRange { 0.f, 100.f }, 0.f,
        &chowdsp::ParamUtils::floatValToString, &chowdsp::ParamUtils::stringToFloatVal, true };
    chowdsp::FloatParameter::Ptr earlyLevel {
        juce::ParameterID { "reverbEarly", 100 }, "Early Level",
        juce::NormalisableRange { 0.f, 100.f }, 0.f,
        &chowdsp::ParamUtils::floatValToString, &chowdsp::ParamUtils::stringToFloatVal, true };
    chowdsp::FloatParameter::Ptr lateLevel {
        juce::ParameterID { "reverbLate", 100 }, "Late Level",
        juce::NormalisableRange { 0.f, 100.f }, 0.f,
        &chowdsp::ParamUtils::floatValToString, &chowdsp::ParamUtils::stringToFloatVal, true };
    chowdsp::FloatParameter::Ptr size {
        juce::ParameterID { "reverbSize", 100 }, "Size",
        juce::NormalisableRange { 10.f, 60.f }, 10.f,
        &chowdsp::ParamUtils::floatValToString, &chowdsp::ParamUtils::stringToFloatVal, true };
    chowdsp::FloatParameter::Ptr width {
        juce::ParameterID { "reverbWidth", 100 }, "Width",
        juce::NormalisableRange { 50.f, 150.f }, 50.f,
        &chowdsp::ParamUtils::floatValToString, &chowdsp::ParamUtils::stringToFloatVal, true };
    chowdsp::FloatParameter::Ptr predelay {
        juce::ParameterID { "reverbPredelay", 100 }, "Predelay",
        juce::NormalisableRange { 0.f, 100.f }, 0.f,
        &chowdsp::ParamUtils::floatValToString, &chowdsp::ParamUtils::stringToFloatVal, true };
    chowdsp::FloatParameter::Ptr diffuse {
        juce::ParameterID { "reverbDiffuse", 100 }, "Diffuse",
        juce::NormalisableRange { 0.f, 100.f }, 0.f,
        &chowdsp::ParamUtils::floatValToString, &chowdsp::ParamUtils::stringToFloatVal, true };
    chowdsp::FloatParameter::Ptr lowCut {
        juce::ParameterID { "reverbLowCut", 100 }, "Low Cut",
        juce::NormalisableRange { 0.f, 200.f }, 0.f,
        &chowdsp::ParamUtils::floatValToString, &chowdsp::ParamUtils::stringToFloatVal, false };
    chowdsp::FloatParameter::Ptr lowXover {
        juce::ParameterID { "reverbLowXover", 100 }, "Low Cross",
        juce::NormalisableRange { 200.f, 1200.f }, 200.f,
        &chowdsp::ParamUtils::floatValToString, &chowdsp::ParamUtils::stringToFloatVal, false };
    chowdsp::FloatParameter::Ptr lowMult {
        juce::ParameterID { "reverbLowMult", 100 }, "Low Mult",
        juce::NormalisableRange { 0.5f, 2.5f }, 0.5f,
        &chowdsp::ParamUtils::floatValToString, &chowdsp::ParamUtils::stringToFloatVal, false };
    chowdsp::FloatParameter::Ptr highCut {
        juce::ParameterID { "reverbHighCut", 100 }, "High Cut",
        chowdsp::ParamUtils::createNormalisableRange (1000.f, 16000.f, 4000.f, 1.f), 1000.f,
        &chowdsp::ParamUtils::floatValToString, &chowdsp::ParamUtils::stringToFloatVal, false };
    chowdsp::FloatParameter::Ptr highXover {
        juce::ParameterID { "reverbHighXover", 100 }, "High Cross",
        chowdsp::ParamUtils::createNormalisableRange (1000.f, 16000.f, 4000.f, 1.f), 1000.f,
        &chowdsp::ParamUtils::floatValToString, &chowdsp::ParamUtils::stringToFloatVal, false };
    chowdsp::FloatParameter::Ptr highMult {
        juce::ParameterID { "reverbHighMult", 100 }, "High Mult",
        juce::NormalisableRange { 0.2f, 1.2f }, 0.2f,
        &chowdsp::ParamUtils::floatValToString, &chowdsp::ParamUtils::stringToFloatVal, false };
    chowdsp::FloatParameter::Ptr spin {
        juce::ParameterID { "reverbSpin", 100 }, "Spin",
        juce::NormalisableRange { 0.f, 10.f }, 0.f,
        &chowdsp::ParamUtils::floatValToString, &chowdsp::ParamUtils::stringToFloatVal, false };
    chowdsp::FloatParameter::Ptr wander {
        juce::ParameterID { "reverbWander", 100 }, "Wander",
        juce::NormalisableRange { 0.f, 40.f }, 0.f,
        &chowdsp::ParamUtils::floatValToString, &chowdsp::ParamUtils::stringToFloatVal, false };
    chowdsp::FloatParameter::Ptr decay {
        juce::ParameterID { "reverbDecay", 100 }, "Decay",
        chowdsp::ParamUtils::createNormalisableRange (0.1f, 10.f, 1.f, 0.01f), 0.1f,
        &chowdsp::ParamUtils::floatValToString, &chowdsp::ParamUtils::stringToFloatVal, true };
    chowdsp::FloatParameter::Ptr earlySend {
        juce::ParameterID { "reverbEarlySend", 100 }, "Early Send",
        juce::NormalisableRange { 0.f, 100.f }, 0.f,
        &chowdsp::ParamUtils::floatValToString, &chowdsp::ParamUtils::stringToFloatVal, false };
    chowdsp::FloatParameter::Ptr modulation {
        juce::ParameterID { "reverbModulation", 100 }, "Modulation",
        juce::NormalisableRange { 0.f, 100.f }, 0.f,
        &chowdsp::ParamUtils::floatValToString, &chowdsp::ParamUtils::stringToFloatVal, false };
};

struct ReverbNonParameterState : chowdsp::NonParamState
{
    ReverbNonParameterState() {}
};

// ──────────────────────────────────────────────────────────────────────
// ReverbProcessor
// ──────────────────────────────────────────────────────────────────────
class ReverbProcessor
    : public bitklavier::PluginBase<bitklavier::PreparationStateImpl<ReverbParams, ReverbNonParameterState>>,
      public bitklavier::ExternalAudioInputReceiver
{
public:
    ReverbProcessor (SynthBase& parent, const juce::ValueTree& vt, juce::UndoManager*);
    ~ReverbProcessor() = default;

    void setExternalInputBuffer (const juce::AudioBuffer<float>* buf) override { externalInputBuffer = buf; }

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}

    void processAudioBlock (juce::AudioBuffer<float>&) override {}
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override;
    void processBlockBypassed (juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}

    // Preset index: 0 = Custom, 1-25 = presets. Stored as VT property for persistence.
    int  getPresetIndex() const { return (int) v.getProperty (IDs::reverbPreset, 0); }
    void setPresetIndex (int idx) { v.setProperty (IDs::reverbPreset, idx, nullptr); }
    void applyPreset (int idx); // 1-based; applies all 18 param values

    juce::AudioProcessor::BusesProperties reverbBusLayout()
    {
        return BusesProperties()
            .withOutput ("Output",     juce::AudioChannelSet::stereo(),               true)
            .withInput  ("Input",      juce::AudioChannelSet::stereo(),               true)
            .withInput  ("Send Pad",   juce::AudioChannelSet::stereo(),               true)
            .withInput  ("Modulation", juce::AudioChannelSet::discreteChannels (24),  true) // 12 modulatable params × 2 channels (ramp + LFO)
            .withOutput ("Modulation", juce::AudioChannelSet::mono(),                 false)
            .withOutput ("Send",       juce::AudioChannelSet::stereo(),               true);
    }
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    bool hasEditor() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }

private:
    // Dragonfly Hall DSP state (adapted from DSP.hpp/DSP.cpp, DPF deps removed)
    float oldParams_[kReverbParamCount];
    float newParams_[kReverbParamCount];

    float dryLevel_   = 0.f;
    float earlyLevel_ = 0.f;
    float early_send_ = 0.2f;
    float lateLevel_  = 0.f;

    fv3::earlyref_f early_;
    fv3::zrev2_f    late_;

    static constexpr uint32_t kBufSize = 256;
    float earlyOutBuf_[2][kBufSize];
    float lateInBuf_ [2][kBufSize];
    float lateOutBuf_[2][kBufSize];

    double sampleRate_ = 44100.0;

    const juce::AudioBuffer<float>* externalInputBuffer = nullptr;

    juce::int64 presetAppliedAtMs_ = -1;
    chowdsp::ScopedCallbackList reverbCallbacks_;
};
