// Copyright (C) 2022-2025 Dan Trueman
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ChucKlavierProcessor.h"
#include "synth_base.h"
#include "TempoProcessor.h"

// VERSION is defined as a preprocessor macro by the JUCE build system ("5.1.0").
// chuck.h has a member `static std::string VERSION` which the macro corrupts.
// Undef before including to avoid the collision.
#undef VERSION
#include "chuck.h"
#include "chuck_errmsg.h"
#include "chuck_globals.h"

// Thread-local pointer set before vm_->run() so the static callback can dispatch to the right instance.
thread_local ChucKlavierProcessor* ChucKlavierProcessor::g_currentProcessor = nullptr;

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

// Parse //@knob annotations from a ChucK script.
// Expected format (on the line immediately before a `global float NAME;` declaration):
//   //@knob [label="My Label"] [min=0] [max=1] [default=0.5]
// Any key=value pair can be present or absent; defaults are applied when missing.
struct ParsedKnob
{
    juce::String name;
    juce::String label;
    float minVal    = 0.0f;
    float maxVal    = 1.0f;
    float defaultVal = 0.5f;
};

// Find the value string for `key` in `src`, tolerating spaces around `=`.
// Returns empty string if key not found.
// Enforces a word boundary before the key so "max" doesn't match "maxValue".
static juce::String getKVValue (const juce::String& src, const juce::String& key)
{
    int idx = 0;
    while ((idx = src.indexOf (idx, key)) >= 0)
    {
        // Word-boundary: char before key must not be alphanumeric or '_'
        if (idx > 0)
        {
            juce::juce_wchar prev = src[idx - 1];
            if (juce::CharacterFunctions::isLetterOrDigit (prev) || prev == '_')
            {
                idx += key.length();
                continue;
            }
        }
        // After the key: skip whitespace, expect '='
        juce::String after = src.substring (idx + key.length()).trimStart();
        if (! after.startsWithChar ('='))
        {
            idx += key.length();
            continue;
        }
        return after.substring (1).trimStart();  // value follows '='
    }
    return {};
}

static float parseKV (const juce::String& src, const juce::String& key, float fallback)
{
    juce::String rest = getKVValue (src, key);
    if (rest.isEmpty()) return fallback;
    if (rest.startsWithChar ('"'))
        rest = rest.substring (1, rest.indexOf (1, "\""));
    return rest.getFloatValue();
}

static juce::String parseKVString (const juce::String& src, const juce::String& key,
                                   const juce::String& fallback)
{
    juce::String rest = getKVValue (src, key);
    if (rest.isEmpty()) return fallback;
    if (rest.startsWithChar ('"'))
    {
        int end = rest.indexOf (1, "\"");
        return end > 0 ? rest.substring (1, end) : fallback;
    }
    // Unquoted: read until next whitespace
    return rest.upToFirstOccurrenceOf (" ", false, false)
               .upToFirstOccurrenceOf ("\t", false, false);
}

static std::vector<ParsedKnob> parseScriptKnobs (const juce::String& script)
{
    std::vector<ParsedKnob> result;
    juce::StringArray lines;
    lines.addLines (script);

    for (int i = 0; i < lines.size() - 1; ++i)
    {
        juce::String line = lines[i].trim();
        if (! line.startsWith ("//@knob")) continue;

        // Next non-blank line must be `global float NAME;`
        juce::String nextLine = lines[i + 1].trim();
        if (! nextLine.startsWith ("global") || ! nextLine.contains ("float")) continue;

        // Extract variable name: "global float NAME;" -> NAME
        juce::String afterFloat = nextLine.fromFirstOccurrenceOf ("float", false, false).trim();
        juce::String varName = afterFloat.upToFirstOccurrenceOf (";", false, false).trim()
                                         .upToFirstOccurrenceOf (" ", false, false).trim()
                                         .upToFirstOccurrenceOf ("[", false, false).trim();
        if (varName.isEmpty()) continue;

        ParsedKnob pk;
        pk.name       = varName;
        pk.minVal     = parseKV       (line, "min",     0.0f);
        pk.maxVal     = parseKV       (line, "max",     1.0f);
        pk.defaultVal = parseKV       (line, "default", 0.5f);
        pk.label      = parseKVString (line, "label",   varName);
        result.push_back (pk);
    }
    return result;
}

static int findFreeSlot (const std::array<ChucKlavierProcessor::SlotBinding,
                                          ChucKlavierParams::kMaxChuckModParams>& slots)
{
    for (int i = 0; i < ChucKlavierParams::kMaxChuckModParams; ++i)
        if (slots[i].name.isEmpty())
            return i;
    return -1;  // all 32 slots used
}

ChucKlavierProcessor::ChucKlavierProcessor (SynthBase& parent, const juce::ValueTree& vt, juce::UndoManager* um)
    : PluginBase (parent, vt, um, chucKlavierBusLayout())
{
    // Restore slot bindings from an existing <chuckKnobs> child (gallery load path).
    loadSlotBindingsFromVT();
}

// Destructor defined here so std::unique_ptr<ChucK> sees the complete ChucK type.
ChucKlavierProcessor::~ChucKlavierProcessor() = default;

void ChucKlavierProcessor::setTuning (TuningProcessor* tun)
{
    if (tuning == tun)
        return;

    if (tuning != nullptr)
        tuning->removeListener (this);

    tuning = tun;

    if (tuning != nullptr)
        tuning->addListener (this);
}

void ChucKlavierProcessor::tuningStateInvalidated()
{
    tuning = nullptr;
}

void ChucKlavierProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    if (vm_ && vmSampleRate_ == sampleRate)
    {
        // Same sample rate — just recompile any saved script without recreating the VM.
        // This avoids a full teardown when prepareToPlay is called multiple times.
    }
    else
    {
        vm_.reset();
        vm_ = std::make_unique<ChucK>();

        vm_->setParam (CHUCK_PARAM_SAMPLE_RATE,     (t_CKINT) sampleRate);
        vm_->setParam (CHUCK_PARAM_INPUT_CHANNELS,  (t_CKINT) 2);
        vm_->setParam (CHUCK_PARAM_OUTPUT_CHANNELS, (t_CKINT) 2);
        vm_->setParam (CHUCK_PARAM_VM_HALT,         (t_CKINT) 0);

        // Route ChucK's stderr to our captured string rather than the console.
        vm_->setCherrCallback ([] (const char* msg) {
            // Stateless callback — nothing to do here; callers read EM_lasterror() directly.
            (void) msg;
        });

        vm_->init();
        vm_->start();
        vm_->globals()->listenForGlobalEvent ("bkMidiOutEvent", &ChucKlavierProcessor::onMidiOutFromVM, TRUE);
        vmSampleRate_ = sampleRate;
    }

    chuckInBuf_.resize  ((size_t) samplesPerBlock * 2, 0.0f);
    chuckOutBuf_.resize ((size_t) samplesPerBlock * 2, 0.0f);

    // Compile the saved script, or the default passthrough if none.
    juce::String savedScript = v.getProperty (IDs::chuckScript, "");
    const std::string script = savedScript.isNotEmpty()
                               ? savedScript.toStdString()
                               : std::string (kDefaultScript);
    vm_->compileCode (script, "");

    // After compilation, cache t_CKFLOAT* pointers for any loaded slot bindings.
    // (For a fresh gallery, slots_ is populated from loadSlotBindingsFromVT() in the ctor.)
    cacheGlobalPtrs();
}

bool ChucKlavierProcessor::doHotSwap (const std::string& script)
{
    // Build the new VM first — don't touch vm_ until we know compilation succeeds.
    // vmSuspended_=true guarantees the AT is silent and not reading vm_.
    auto newVm = std::make_unique<ChucK>();
    newVm->setParam (CHUCK_PARAM_SAMPLE_RATE,     (t_CKINT) getSampleRate());
    newVm->setParam (CHUCK_PARAM_INPUT_CHANNELS,  (t_CKINT) 2);
    newVm->setParam (CHUCK_PARAM_OUTPUT_CHANNELS, (t_CKINT) 2);
    newVm->setParam (CHUCK_PARAM_VM_HALT,         (t_CKINT) 0);
    newVm->setCherrCallback ([] (const char* msg) { (void) msg; });
    newVm->init();
    newVm->start();
    newVm->globals()->listenForGlobalEvent ("bkMidiOutEvent", &ChucKlavierProcessor::onMidiOutFromVM, TRUE);

    if (!newVm->compileCode (script, "", 1, /*immediate=*/ true))
    {
        // Capture the error NOW, while newVm is still alive — ChucK's dtor may
        // clear the global EM_lasterror state when newVm goes out of scope.
        lastCompileMessage = juce::String::fromUTF8 (EM_lasterror());
        if (lastCompileMessage.isEmpty())
            lastCompileMessage = "Compile error";
        return false;  // vm_ is unchanged; old script keeps running
    }

    vm_ = std::move (newVm);
    vmSampleRate_ = getSampleRate();
    lastCompileMessage = "Compiled OK";
    return true;
}

void ChucKlavierProcessor::allNotesOff()
{
    // If a hot-swap is already in flight, that swap will silence the VM — skip.
    if (vmSuspended_.load (std::memory_order_acquire))
        return;

    // Reload the VM with the currently saved script.  This tears down every spork,
    // UGen, and event the script built up, silencing all ChucK output instantly.
    // Reading from v (the persisted ValueTree) gives us the last-compiled script,
    // not any unsent edits in the editor buffer — intentional.
    juce::String savedScript = v.getProperty (IDs::chuckScript, "");
    const std::string script = savedScript.isNotEmpty()
                               ? savedScript.toStdString()
                               : std::string (kDefaultScript);

    // Reset vmParked_ BEFORE asserting vmSuspended_ so the wait loop below waits
    // for the AT to park in response to THIS request (not a stale flag from an
    // earlier hot-swap).  Mirrors the exact ordering used by the Send-to-VM button.
    vmParked_.store (false, std::memory_order_release);
    vmSuspended_.store (true, std::memory_order_release);
    while (!vmParked_.load (std::memory_order_acquire))
        juce::Thread::sleep (1);

    doHotSwap (script);

    vmSuspended_.store (false, std::memory_order_release);
}

// ---------------------------------------------------------------------------
// Slot binding helpers
// ---------------------------------------------------------------------------

// Restore slot table from the <chuckKnobs> VT child (called from ctor).
void ChucKlavierProcessor::loadSlotBindingsFromVT()
{
    auto knobsVt = v.getChildWithName (IDs::chuckKnobs);
    if (! knobsVt.isValid()) return;
    for (int ci = 0; ci < knobsVt.getNumChildren(); ++ci)
    {
        auto child = knobsVt.getChild (ci);
        if (! child.hasType (IDs::chuckKnob)) continue;
        const int slot = (int) child.getProperty (IDs::parameter, -1);
        if (slot < 0 || slot >= ChucKlavierParams::kMaxChuckModParams) continue;
        slots_[slot].name     = child.getProperty (IDs::chuckGlobalName, "").toString();
        slots_[slot].label    = child.getProperty (IDs::name, slots_[slot].name).toString();
        slots_[slot].minVal   = (float) child.getProperty (IDs::start, 0.0f);
        slots_[slot].maxVal   = (float) child.getProperty (IDs::end,   1.0f);
        slots_[slot].defaultVal = (float) child.getProperty ("default", 0.5f);
        slots_[slot].active   = slots_[slot].name.isNotEmpty();
        // Restore slider value if present (before setupModulationMappings runs)
        if (child.hasProperty (IDs::sliderval))
        {
            float sv = (float) child.getProperty (IDs::sliderval, 0.5f);
            if (state.params.chuckMod[slot] != nullptr)
                state.params.chuckMod[slot]->setParameterValue (sv);
        }
    }
}

// Write the current slot table back to the VT (called from reconcileSlots + setSlotLabel/Range).
void ChucKlavierProcessor::saveSlotBindingsToVT()
{
    auto knobsVt = v.getOrCreateChildWithName (IDs::chuckKnobs, nullptr);
    knobsVt.removeAllChildren (nullptr);
    for (int i = 0; i < ChucKlavierParams::kMaxChuckModParams; ++i)
    {
        if (slots_[i].name.isEmpty()) continue;
        auto child = juce::ValueTree (IDs::chuckKnob);
        child.setProperty (IDs::parameter,      i,                 nullptr);
        child.setProperty (IDs::chuckGlobalName, slots_[i].name,   nullptr);
        child.setProperty (IDs::name,            slots_[i].label,  nullptr);
        child.setProperty (IDs::start,           slots_[i].minVal, nullptr);
        child.setProperty (IDs::end,             slots_[i].maxVal, nullptr);
        child.setProperty ("default",            slots_[i].defaultVal, nullptr);
        if (state.params.chuckMod[i] != nullptr)
            child.setProperty (IDs::sliderval, state.params.chuckMod[i]->getCurrentValue(), nullptr);
        knobsVt.appendChild (child, nullptr);
    }
}

// Cache t_CKFLOAT* pointers for all active slots from the current VM.
// Must be called while vmSuspended_=true (AT parked) or from prepareToPlay
// (audio not yet running for this processor).
void ChucKlavierProcessor::cacheGlobalPtrs()
{
    if (vm_ == nullptr) return;
    auto* g = vm_->globals();
    for (int i = 0; i < ChucKlavierParams::kMaxChuckModParams; ++i)
    {
        if (! slots_[i].active || slots_[i].name.isEmpty())
        {
            slots_[i].cachedPtr = nullptr;
            continue;
        }
        const std::string name = slots_[i].name.toStdString();
        // init_global_float ensures no null-entry in the map (see chuck_embedding.md)
        g->init_global_float (name);
        slots_[i].cachedPtr =
            reinterpret_cast<double*> (g->get_ptr_to_global_float (name));
    }
}

// Reconcile the stable slot→name mapping against the freshly-compiled script.
// Parses //@knob annotations; keeps existing slot assignments for known names;
// allocates free slots for new names; marks removed names as inactive (orphaned).
// Updates the <chuckKnobs> VT child and caches fresh t_CKFLOAT* pointers.
// Must be called on the message thread while vmSuspended_=true (AT parked).
void ChucKlavierProcessor::reconcileSlots (const juce::String& script)
{
    auto parsed = parseScriptKnobs (script);

    // Step 1: mark all currently-active slots inactive; we'll re-activate matched ones.
    for (auto& sb : slots_)
        sb.active = false;

    // Step 2: for each parsed knob, find an existing slot with the same name or claim a free one.
    for (const auto& pk : parsed)
    {
        // Find existing slot with this name (keeps slot index stable)
        int targetSlot = -1;
        for (int i = 0; i < ChucKlavierParams::kMaxChuckModParams; ++i)
        {
            if (slots_[i].name == pk.name)
            {
                targetSlot = i;
                break;
            }
        }
        if (targetSlot < 0)
            targetSlot = findFreeSlot (slots_);
        if (targetSlot < 0) continue;  // no free slots

        slots_[targetSlot].name       = pk.name;
        slots_[targetSlot].label      = pk.label;
        slots_[targetSlot].minVal     = pk.minVal;
        slots_[targetSlot].maxVal     = pk.maxVal;
        slots_[targetSlot].defaultVal = pk.defaultVal;
        slots_[targetSlot].active     = true;

        // Set the parameter to the default value (normalized)
        float normDefault = juce::jlimit (0.0f, 1.0f,
            (pk.defaultVal - pk.minVal) / juce::jmax (1e-6f, pk.maxVal - pk.minVal));
        if (state.params.chuckMod[targetSlot] != nullptr)
            state.params.chuckMod[targetSlot]->setParameterValue (normDefault);

        // Update the MODULATABLE_PARAMS VT entry for this slot so that the modulation
        // system's setScalingValue() and updateScalingAudioThread() use the display range
        // [minVal, maxVal] rather than the chowdsp parameter's native [0, 1] range.
        // Without this, sliderVal (e.g. 12) passed to setScalingValue with start/end = 0/1
        // gets clamped to 1, corrupting the scaling calculation.
        {
            auto modParamsVt = v.getChildWithName (IDs::MODULATABLE_PARAMS);
            const juce::String paramId = juce::String ("ChuckMod") + juce::String (targetSlot);
            auto modParamVt = modParamsVt.getChildWithProperty (IDs::parameter, paramId);
            if (modParamVt.isValid())
            {
                modParamVt.setProperty (IDs::start,    pk.minVal,     nullptr);
                modParamVt.setProperty (IDs::end,      pk.maxVal,     nullptr);
                modParamVt.setProperty (IDs::sliderval, pk.defaultVal, nullptr);
            }
        }
    }

    // Step 3: cache fresh VM pointers for active slots.
    cacheGlobalPtrs();

    // Step 4: persist the updated table to the VT.
    saveSlotBindingsToVT();
}

void ChucKlavierProcessor::setSlotLabel (int slot, const juce::String& label)
{
    if (slot < 0 || slot >= ChucKlavierParams::kMaxChuckModParams) return;
    slots_[slot].label = label;
    saveSlotBindingsToVT();
}

void ChucKlavierProcessor::setSlotRange (int slot, float minVal, float maxVal)
{
    if (slot < 0 || slot >= ChucKlavierParams::kMaxChuckModParams) return;
    slots_[slot].minVal = minVal;
    slots_[slot].maxVal = maxVal;
    saveSlotBindingsToVT();

    auto modParamsVt = v.getChildWithName (IDs::MODULATABLE_PARAMS);
    const juce::String paramId = juce::String ("ChuckMod") + juce::String (slot);
    auto modParamVt = modParamsVt.getChildWithProperty (IDs::parameter, paramId);
    if (modParamVt.isValid())
    {
        modParamVt.setProperty (IDs::start, minVal, nullptr);
        modParamVt.setProperty (IDs::end,   maxVal, nullptr);
    }
}

void ChucKlavierProcessor::releaseResources()
{
    vm_.reset();
    vmSampleRate_ = 0.0;
}

void ChucKlavierProcessor::onMidiOutFromVM()
{
    if (auto* p = g_currentProcessor)
        p->handleMidiOutEvent();
}

void ChucKlavierProcessor::handleMidiOutEvent()
{
    if (currentMidiOutputBuffer_ == nullptr || vm_ == nullptr)
        return;

    auto* g = vm_->globals();
    const auto status = (juce::uint8) g->get_global_int_array_value ("bkMidiOut", 0);
    if (status == 0)
        return;  // script hasn't written an event

    const auto d1  = (juce::uint8) g->get_global_int_array_value ("bkMidiOut", 1);
    const auto d2  = (juce::uint8) g->get_global_int_array_value ("bkMidiOut", 2);
    const int  pos = (int)         g->get_global_int_array_value ("bkMidiOut", 3);
    const int clampedPos = juce::jlimit (0, juce::jmax (0, currentBlockSize_ - 1), pos);
    const juce::uint8 bytes[3] = { status, d1, d2 };
    currentMidiOutputBuffer_->addEvent (juce::MidiMessage (bytes, 3), clampedPos);
}

void ChucKlavierProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    state.getParameterListeners().callAudioThreadBroadcasters();
    processContinuousModulations (buffer);

    // Snapshot inbound MIDI; clear the buffer (script decides what passes through).
    juce::MidiBuffer inboundMidi;
    inboundMidi.swapWith (midiMessages);
    // midiMessages is now empty; script will addEvent() into it via handleMidiOutEvent().

    const int numSamples = buffer.getNumSamples();
    const bool muted = state.params.muted_.load (std::memory_order_relaxed);

    // Internal input gain
    auto inputGainMult = bitklavier::utils::dbToMagnitude (state.params.inputGain->getCurrentValue());
    buffer.applyGain (0, 0, numSamples, inputGainMult);
    buffer.applyGain (1, 0, numSamples, inputGainMult);

    std::get<0> (state.params.inputLevels) = buffer.getRMSLevel (0, 0, numSamples);
    std::get<1> (state.params.inputLevels) = buffer.getRMSLevel (1, 0, numSamples);

    // Mix in external audio
    if (externalInputBuffer != nullptr)
    {
        const int extSamples = juce::jmin (numSamples, externalInputBuffer->getNumSamples());
        auto extGainMult = bitklavier::utils::dbToMagnitude (state.params.externalGain->getCurrentValue());

        std::get<0> (state.params.externalLevels) = externalInputBuffer->getRMSLevel (0, 0, extSamples) * extGainMult;
        std::get<1> (state.params.externalLevels) = externalInputBuffer->getRMSLevel (1, 0, extSamples) * extGainMult;

        buffer.addFrom (0, 0, *externalInputBuffer, 0, 0, extSamples, extGainMult);
        buffer.addFrom (1, 0, *externalInputBuffer, 1, 0, extSamples, extGainMult);
    }
    else
    {
        std::get<0> (state.params.externalLevels) = 0.0f;
        std::get<1> (state.params.externalLevels) = 0.0f;
    }

    // ChucK VM tick — skip when suspended for hot-swap, or if not yet initialised.
    if (vm_ != nullptr && !vmSuspended_.load (std::memory_order_acquire))
    {
        // Feed each inbound MIDI message to the VM as a global-int-array + event signal.
        {
            auto* g = vm_->globals();
            for (auto mi : inboundMidi)
            {
                const auto msg = mi.getMessage();
                if (msg.getRawDataSize() < 3) continue;
                const auto* raw = msg.getRawData();
                t_CKINT arr[4] = { (t_CKINT) raw[0], (t_CKINT) raw[1],
                                   (t_CKINT) raw[2], (t_CKINT) mi.samplePosition };
                g->set_global_int_array ("bkMidiIn", arr, 4);
                g->signalGlobalEvent ("bkMidiInEvent");
            }
        }

        // Push tuning table (128 Hz values) and tempo BPM into ChucK globals each block.
        if (tuning != nullptr)
        {
            t_CKFLOAT tmp[128];
            double dtmp[128];
            tuning->fillTuningTable (dtmp);
            for (int i = 0; i < 128; ++i)
                tmp[i] = (t_CKFLOAT) dtmp[i];
            vm_->globals()->set_global_float_array ("bkTuningTable", tmp, 128);
        }

        if (tempo != nullptr)
        {
            auto* g = vm_->globals();
            // init_global_float ensures the container exists before we take its address.
            // Without this, get_ptr_to_global_float uses operator[] which inserts nullptr
            // on a fresh VM, preventing the script's own 'global float bkTempoBPM'
            // declaration from allocating the container → crash on first script read.
            g->init_global_float ("bkTempoBPM");
            if (auto* ptr = g->get_ptr_to_global_float ("bkTempoBPM"))
                *ptr = (t_CKFLOAT) tempo->getState().params.tempoParam->getCurrentValue();
        }

        // Interleave JUCE stereo → ChucK interleaved format
        const float* L = buffer.getReadPointer (0);
        const float* R = buffer.getReadPointer (1);
        for (int i = 0; i < numSamples; ++i)
        {
            chuckInBuf_[size_t(i * 2)]     = L[i];
            chuckInBuf_[size_t(i * 2 + 1)] = R[i];
        }

        // Write denormalized values of active knob slots into ChucK globals.
        // processContinuousModulations() (called above at line ~177) has already
        // applied any mod offsets to state.params.chuckMod[i], so getCurrentValue()
        // here reflects base + modulation.
        for (int i = 0; i < ChucKlavierParams::kMaxChuckModParams; ++i)
        {
            if (! slots_[i].active || slots_[i].cachedPtr == nullptr) continue;
            if (state.params.chuckMod[i] == nullptr) continue;
            const float norm = state.params.chuckMod[i]->getCurrentValue();
            *slots_[i].cachedPtr =
                (double) (slots_[i].minVal + norm * (slots_[i].maxVal - slots_[i].minVal));
            modulatedValues_[i].store (norm, std::memory_order_relaxed);
        }

        // Arm context pointers so the static MIDI-out callback can dispatch to us.
        g_currentProcessor      = this;
        currentMidiOutputBuffer_ = &midiMessages;
        currentBlockSize_        = numSamples;

        vm_->run (chuckInBuf_.data(), chuckOutBuf_.data(), (t_CKINT) numSamples);

        currentMidiOutputBuffer_ = nullptr;
        currentBlockSize_        = 0;
        g_currentProcessor       = nullptr;

        // Deinterleave ChucK output → JUCE stereo
        float* outL = buffer.getWritePointer (0);
        float* outR = buffer.getWritePointer (1);
        for (int i = 0; i < numSamples; ++i)
        {
            outL[i] = (float) chuckOutBuf_[size_t(i * 2)];
            outR[i] = (float) chuckOutBuf_[size_t(i * 2 + 1)];
        }
    }
    else if (vmSuspended_.load (std::memory_order_acquire))
    {
        // Silence output while hot-swap is in progress; acknowledge the park.
        // inboundMidi is discarded — one-block MIDI dropout matches the audio dropout.
        buffer.clear (0, 0, numSamples);
        buffer.clear (1, 0, numSamples);
        vmParked_.store (true, std::memory_order_release);
    }

    // Send output
    const int sendBufferIndex = getChannelIndexInProcessBlockBuffer (false, 2, 0);
    auto sendGainMult = muted ? 0.0f : bitklavier::utils::dbToMagnitude (state.params.outputSend->getCurrentValue());
    buffer.copyFrom (sendBufferIndex,     0, buffer.getReadPointer (0), numSamples, sendGainMult);
    buffer.copyFrom (sendBufferIndex + 1, 0, buffer.getReadPointer (1), numSamples, sendGainMult);

    std::get<0> (state.params.sendLevels) = buffer.getRMSLevel (sendBufferIndex,     0, numSamples);
    std::get<1> (state.params.sendLevels) = buffer.getRMSLevel (sendBufferIndex + 1, 0, numSamples);

    // Main output gain
    auto outputGainMult = muted ? 0.0f : bitklavier::utils::dbToMagnitude (state.params.outputGain->getCurrentValue());
    buffer.applyGain (0, 0, numSamples, outputGainMult);
    buffer.applyGain (1, 0, numSamples, outputGainMult);

    std::get<0> (state.params.outputLevels) = buffer.getRMSLevel (0, 0, numSamples);
    std::get<1> (state.params.outputLevels) = buffer.getRMSLevel (1, 0, numSamples);
}
