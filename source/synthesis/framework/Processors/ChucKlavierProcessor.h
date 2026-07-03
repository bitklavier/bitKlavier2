// Copyright (C) 2022-2025 Dan Trueman
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "IMuteSolable.h"
#include "PluginBase.h"
#include "TuningProcessor.h"
#include "Identifiers.h"
#include <PreparationStateImpl.h>
#include <chowdsp_plugin_base/chowdsp_plugin_base.h>
#include <chowdsp_plugin_utils/chowdsp_plugin_utils.h>
#include <chowdsp_plugin_state/chowdsp_plugin_state.h>
#include <chowdsp_serialization/chowdsp_serialization.h>
#include "utils.h"

// Forward-declare ChucK to avoid including chuck.h in this header.
// chuck.h defines VERSION as a class member but the project also defines VERSION
// as a preprocessor macro ("5.1.0"), causing a collision in all TUs that include
// chuck.h. Keeping chuck.h in .cpp files only prevents this from spreading.
class ChucK;

struct ChucKlavierParams : chowdsp::ParamHolder
{
    // Maximum number of simultaneously exposed ChucK globals (matches VSTModulationBridge).
    static constexpr int kMaxChuckModParams = 32;

    float rangeStart = -80.0f;
    float rangeEnd   =   6.0f;
    float skewFactor =   2.0f;

    ChucKlavierParams (const juce::ValueTree& /*v*/) : chowdsp::ParamHolder ("chucklavier")
    {
        add (outputGain, inputGain, externalGain, outputSend);

        // Initialize 32 normalized (0..1) float params for ChucK global modulation slots.
        // The display label/range for each slot is stored in ChucKlavierProcessor::slots_[];
        // these params carry only the normalized value used by processContinuousModulations.
        for (int i = 0; i < kMaxChuckModParams; ++i)
        {
            chuckMod[i].setOwning (new chowdsp::FloatParameter (
                juce::ParameterID { "ChuckMod" + juce::String (i), 100 },
                "ChucK Knob " + juce::String (i),
                juce::NormalisableRange<float> { 0.0f, 1.0f },
                0.5f,
                [] (float v) { return juce::String (v, 3); },
                [] (const juce::String& s) { return s.getFloatValue(); },
                true  // audio-rate modulatable
            ));
        }
        add (chuckMod);

        doForAllParameters ([this] (auto& param, size_t) {
            if (auto* sliderParam = dynamic_cast<chowdsp::FloatParameter*> (&param))
                if (sliderParam->supportsMonophonicModulation())
                    modulatableParams.push_back (sliderParam);
        });
    }

    chowdsp::GainDBParameter::Ptr inputGain {
        juce::ParameterID { "ChucKInputGain", 100 },
        "Internal Gain",
        juce::NormalisableRange { rangeStart, rangeEnd, 0.0f, skewFactor, false },
        0.0f,
        true
    };

    chowdsp::GainDBParameter::Ptr externalGain {
        juce::ParameterID { "ChucKExternalGain", 100 },
        "External Gain",
        juce::NormalisableRange { rangeStart, rangeEnd, 0.0f, skewFactor, false },
        rangeStart,
        true
    };

    chowdsp::GainDBParameter::Ptr outputSend {
        juce::ParameterID { "ChucKSend", 100 },
        "Send",
        juce::NormalisableRange { rangeStart, rangeEnd, 0.0f, skewFactor, false },
        0.0f,
        true
    };

    chowdsp::GainDBParameter::Ptr outputGain {
        juce::ParameterID { "ChucKOutputGain", 100 },
        "Output Gain",
        juce::NormalisableRange { rangeStart, rangeEnd, 0.0f, skewFactor, false },
        0.0f,
        true
    };

    // 32 normalized (0..1) audio-rate modulatable params for ChucK global slots.
    std::array<chowdsp::FloatParameter::Ptr, kMaxChuckModParams> chuckMod;

    std::tuple<std::atomic<float>, std::atomic<float>> outputLevels;
    std::tuple<std::atomic<float>, std::atomic<float>> sendLevels;
    std::tuple<std::atomic<float>, std::atomic<float>> inputLevels;
    std::tuple<std::atomic<float>, std::atomic<float>> externalLevels;

    std::atomic<bool> muted_    { false };
    std::atomic<bool> userMuted_{ false };
    std::atomic<bool> soloed_   { false };
    std::atomic<bool> soloMuted_{ false };
};

struct ChucKlavierNonParameterState : chowdsp::NonParamState
{
    ChucKlavierNonParameterState() = default;
};

class ChucKlavierProcessor
    : public bitklavier::PluginBase<bitklavier::PreparationStateImpl<ChucKlavierParams, ChucKlavierNonParameterState>>,
      public bitklavier::ExternalAudioInputReceiver,
      public IMuteSolable,
      public TuningListener
{
public:
    ChucKlavierProcessor (SynthBase& parent, const juce::ValueTree& v, juce::UndoManager*);
    ~ChucKlavierProcessor();  // defined in .cpp after #include "chuck.h"

    std::atomic<bool>& getMuted()     override { return state.params.muted_; }
    std::atomic<bool>& getUserMuted() override { return state.params.userMuted_; }
    std::atomic<bool>& getSoloed()    override { return state.params.soloed_; }
    std::atomic<bool>& getSoloMuted() override { return state.params.soloMuted_; }

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processAudioBlock (juce::AudioBuffer<float>& buffer) override {}
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    void processBlockBypassed (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override {}

    void setExternalInputBuffer (const juce::AudioBuffer<float>* buf) override { externalInputBuffer = buf; }

    bool hasEditor() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }

    juce::AudioProcessor::BusesProperties chucKlavierBusLayout()
    {
        // 4 existing audio-rate params + kMaxChuckModParams script-exposed globals,
        // each needing 2 channels (ramp + LFO) for the modulation bus.
        const int numModChans = (4 + ChucKlavierParams::kMaxChuckModParams) * 2;
        return BusesProperties()
            .withOutput ("Output",     juce::AudioChannelSet::stereo(), true)
            .withInput  ("Input",      juce::AudioChannelSet::stereo(), true)
            .withInput  ("Send Pad",   juce::AudioChannelSet::stereo(), true)
            .withInput  ("Modulation", juce::AudioChannelSet::discreteChannels (numModChans), true)
            .withOutput ("Modulation", juce::AudioChannelSet::mono(), false)
            .withOutput ("Send",       juce::AudioChannelSet::stereo(), true);
    }

    bool isBusesLayoutSupported (const juce::AudioProcessor::BusesLayout&) const override { return true; }

    ChucK* getVM() { return vm_.get(); }

    // Hot-swap: build a fresh VM, compile script, replace vm_ on success.
    // Returns true on success; vm_ is unchanged on failure.
    // Must be called with vmSuspended_=true (AT is parked and not touching vm_).
    bool doHotSwap (const std::string& script);

    // Silence ChucKlavier by reloading the VM with the currently-running script.
    // Called from SoundEngine::allNotesOff() on the message thread.
    void allNotesOff();

    // --- Script-exposed global knob (slot) system ---
    //
    // Users annotate script globals with //@knob metadata comments.
    // Each annotated global gets a stable slot (0..kMaxChuckModParams-1) so that
    // modulation connections survive script edits as long as the variable name persists.
    //
    // Slot model mirrors VSTModulationBridge but without a separate bridge node:
    //   - 32 chowdsp FloatParameter slots (ChucKlavierParams::chuckMod[]) carry the
    //     normalized (0..1) modulated value through processContinuousModulations.
    //   - SlotBinding maps each slot to a ChucK global name + display label + range.
    //   - processBlock denormalizes and writes into cached t_CKFLOAT* pointers.

    struct SlotBinding
    {
        juce::String name;           // ChucK global variable name; empty = free/orphaned
        juce::String label;          // display label for the UI knob
        float minVal    = 0.0f;
        float maxVal    = 1.0f;
        float defaultVal = 0.5f;
        double* cachedPtr = nullptr; // t_CKFLOAT* — cached from VM, valid until next hot-swap
        bool active = false;         // true = annotated in current script
    };

    // Parses //@knob annotations from the compiled script, reconciles the stable
    // slot→name mapping, and caches t_CKFLOAT* pointers from the current VM.
    // Must be called on the message thread while vmSuspended_=true (AT parked).
    void reconcileSlots (const juce::String& script);

    const SlotBinding& getSlotBinding (int slot) const { return slots_[slot]; }

    // Overrides for user-edited label/range (persisted in the <chuckKnobs> VT child).
    void setSlotLabel (int slot, const juce::String& label);
    void setSlotRange (int slot, float minVal, float maxVal);

    // Modulated values for the 30 Hz UI polling timer.
    std::array<std::atomic<float>, ChucKlavierParams::kMaxChuckModParams> modulatedValues_ {};

    // Atomic handshake for hot-swap: MT sets vmSuspended_ true, AT acknowledges via vmParked_.
    std::atomic<bool> vmSuspended_ { false };
    std::atomic<bool> vmParked_    { false };

    // Last error/status message from compile; written on MT, read on MT.
    juce::String lastCompileMessage;

    // MIDI-out callback: set before vm_->run(), cleared after. Thread-local so
    // multiple concurrent ChucKlavier instances on the same audio thread are safe.
    static thread_local ChucKlavierProcessor* g_currentProcessor;

    // Static trampoline registered with ChucK via listenForGlobalEvent.
    // Fires synchronously inside ChucK::run() on the audio thread.
    static void onMidiOutFromVM();
    void handleMidiOutEvent();

    void setTuning (TuningProcessor* tun) override;
    void tuningStateInvalidated() override;

public:
    static constexpr const char* kDefaultScript =
        "// bitKlavier ChucKlavier default: audio + MIDI passthrough\n"
        "global int bkMidiIn[4];\n"
        "global int bkMidiOut[4];\n"
        "global Event bkMidiInEvent;\n"
        "global Event bkMidiOutEvent;\n"
        "global float bkTuningTable[128];\n"
        "global float bkTempoBPM;\n"
        "\n"
        "adc => dac;\n"
        "\n"
        "fun void midiPassthrough() {\n"
        "    while( true ) {\n"
        "        bkMidiInEvent => now;\n"
        "        bkMidiIn[0] => bkMidiOut[0];\n"
        "        bkMidiIn[1] => bkMidiOut[1];\n"
        "        bkMidiIn[2] => bkMidiOut[2];\n"
        "        bkMidiIn[3] => bkMidiOut[3];\n"
        "        bkMidiOutEvent.broadcast();\n"
        "    }\n"
        "}\n"
        "spork ~ midiPassthrough();\n"
        "\n"
        "while( true ) { 1::samp => now; }\n";

    void loadSlotBindingsFromVT();
    void saveSlotBindingsToVT();
    void cacheGlobalPtrs();  // cache t_CKFLOAT* while AT is parked

    std::unique_ptr<ChucK> vm_;
    double vmSampleRate_ = 0.0;
    std::vector<float> chuckInBuf_;
    std::vector<float> chuckOutBuf_;

    // Stable slot→name binding table.  Written on MT (reconcileSlots, setSlotLabel,
    // setSlotRange) while vmSuspended_=true; read on AT (processBlock).  The AT is
    // guaranteed silent whenever the MT writes, so no additional locking is needed.
    std::array<SlotBinding, ChucKlavierParams::kMaxChuckModParams> slots_ {};

    // Set on the audio thread before vm_->run(), cleared after; read by onMidiOutFromVM callback.
    juce::MidiBuffer* currentMidiOutputBuffer_ = nullptr;
    int currentBlockSize_ = 0;


    const juce::AudioBuffer<float>* externalInputBuffer = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChucKlavierProcessor)
};
