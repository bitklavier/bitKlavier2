# ChucKlavier — Multi-Stage Implementation Plan

Status: **approved, not yet started** (as of 2026-07-02).

ChucKlavier embeds the [ChucK](https://chuck.stanford.edu/) audio language as a bitKlavier preparation. It has audio in (External + Internal like Blendronic), audio out (Send + Main like Blendronic), and MIDI in/out (like MIDI Target). The primary UI element is a text editor for editing ChucK scripts, plus a button to hot-swap the script into the running VM.

## Locked decisions

| Question | Decision |
|---|---|
| ChucK vendoring | Git submodule at `third_party/chuck`, pinned to a release tag |
| Stage 1 script editor | `juce::TextEditor` (multi-line). Upgrade to `juce::CodeEditorComponent` in Stage 5 |
| VM threading | Run `ChucK::run()` on the JUCE audio thread inside `processBlock`. RT-safety must be verified first (Stage 2). |
| Icon | Raster PNG (`chuck-logo2023w.png`) in `assets/chucklavier/` for Stage 1; convert to `Paths::chucklavier` SVG path in Stage 5 |

## Architectural anchors (from codebase survey)

- **Bus layout** — copy Blendronic verbatim (`BlendronicProcessor.h:266-281`): Output / Input / Send Pad / Modulation-in / Modulation-out / Send.
- **"External" audio is not a bus** — it arrives via the `ExternalAudioInputReceiver` interface (`PluginBase.h:36-40`), populated in `SoundEngine::addNode` (`sound_engine.h:220-224`).
- **MIDI I/O is not a bus** — MIDI-producing preps just mutate the `MidiBuffer&` passed to `processBlock` (`MidiTargetProcessor.cpp:94-188`). The `AudioProcessorGraph` routes MIDI along edges.
- **Script text** — store as a ValueTree property (`IDs::chuckScript`), following the `CommentProcessor` pattern (`CommentPreparation.cpp:39-58`), *not* a chowdsp param. chowdsp params are for numeric/enum/bool with modulation.
- **Factory registration** — name-keyed in `synth_base.cpp:104-120`, one line per prep.
- **ChucK host API** (from `src/host-examples/example-2-audio.cpp`): `new ChucK()` → `setParam(SAMPLE_RATE, …)` → `init()` → `start()` → `run(input, output, nFrames)` per block; `compileCode()` / `compileFile()` for scripts; global events / `setGlobal*` for host↔script comm.
- **ChucK is MIT/GPL2 dual-licensed**, Makefile-only, has a `src/core/` "vanilla" mode that skips platform I/O (perfect for embedding).

---

## Stage 1 — Shell preparation, ChucK-free (audio + script text persistence)

Ship a fully-wired ChucKlavier that does nothing except copy input to output. This proves out all the plumbing before we touch a third-party library.

### Files to touch

- `source/synthesis/framework/Processors/ChucKlavierProcessor.h/cpp` — copy `BlendronicProcessor` skeleton; 6-bus layout; `inputGain` / `outputGain` / `outputSend` / `externalGain` params; meter atomics; `ExternalAudioInputReceiver` + `IMuteSolable` interfaces; `processBlock` initially does input-gain → external mix → output-gain → send-copy (identical DSP to Blendronic minus its delay).
- `source/common/common.h` — add `PreparationTypeChucKlavier` to `BKPreparationType` enum + entry in `AllPrepInfo`.
- `source/common/Identifiers.h` — `DECLARE_ID(chucklavier)`, `DECLARE_ID(chuckScript)`, append `chucklavier` to `preparationIDs`.
- `source/synthesis/synth_base.cpp:104-120` — register in `prepFactory`.
- `source/interface/components/ConstructionSite.cpp` — add to `prepSizes` (line 26), popup command ID + case in switch (~line 319+), `nodeFactory.Register`.
- `source/interface/components/BKitems/BKItem.h/cpp` — `ChucKlavierItem` class; icon: drop `chuck-logo2023w.png` into `assets/chucklavier/`, load via `ImageCache` in the item's paint.
- `source/interface/Preparations/ChucKlavierPreparation.h/cpp` — `create()` entry, popup wiring.
- `source/interface/ParameterView/ChucKlavierParametersView.h/cpp` — copy `BlendronicParametersView` structure; four `PeakMeterSection` sliders (External/Internal on left, Send/Main on right); large multi-line `juce::TextEditor` for script text (CommentPreparation pattern — read/write `state.setProperty(IDs::chuckScript, …, &undo)` on change); "Send to VM" button (no-op stub).
- Wire the popup size in `FullInterface::resized()`.

### Exit criteria

- Drop ChucKlavier from popup → item appears with ChucK logo → double-click opens popup → four gain knobs functional with metering → script text edits persist across gallery save/load → audio passthrough is bit-identical to no-op.

---

## Stage 2 — Embed ChucK VM (audio only)

### Tasks

- `git submodule add https://github.com/ccrma/chuck third_party/chuck`, pin to a release tag.
- Write `third_party/chuck/CMakeLists.txt` (ours, not upstream's) that compiles the "vanilla" core file list from `chuck/src/core/`. Skip `chuck/src/host/`. Target C++20. Verify mac + Windows builds and CI packaging.
- **Verify `ChucK::run()` RT-safety first** — read core sources for allocations/locks in the steady-state audio path. If unsafe, decision point: patch (fork) vs. move to worker thread (revisit threading question).
- Per-processor `std::unique_ptr<ChucK> vm_` created in `prepareToPlay` with `setParam(SAMPLE_RATE, …)` from the host block info; destroyed in `releaseResources`. `vm_->run(inPtr, outPtr, numFrames)` in `processBlock` between input gain and output gain.
- Default script hardcoded as a `constexpr const char*` in the processor (something like `adc => dac; while(true){1::samp => now;}`). Compiled on first `prepareToPlay` after VM start.
- "Send to VM" button: on click, `removeAllShreds()` then `compileCode(text, "")` on the message thread. Guard with a lock-free flag consumed on the audio thread so `run()` and `compileCode()` never overlap.
- Redirect ChucK stderr to a captured string; show under the editor as a status line.

### Exit criteria

- Default script produces passthrough (indistinguishable from Stage 1 in a bit-comparison test).
- Typing `adc => Gain g => dac; 0.5 => g.gain;` and clicking Send hot-swaps to half-gain without audio dropout.
- Loading a gallery with a saved script auto-compiles it.

---

## Stage 3 — MIDI in/out via ChucK globals

**Recommended initial approach: global-events (no ChucK fork).**

### Tasks

- Host side: at the top of `processBlock`, for each incoming MIDI message, `vm_->setGlobalIntArray("bkMidiIn", {status, data1, data2, timestamp})` then `vm_->signalGlobalEvent("bkMidiInEvent")`.
- Script side (in the default script): `global Event bkMidiInEvent; global int bkMidiIn[]; while(true){ bkMidiInEvent => now; /* read bkMidiIn */ }`.
- Outgoing: script writes to `global int bkMidiOut[]` and calls `bkMidiOutEvent.broadcast()`. Host installs a `Chuck_Globals_Manager` listener that pushes to a lock-free queue drained at the start of each `processBlock` into the `MidiBuffer&`.
- Rewrite the hardcoded default script to include a "pass audio + MIDI through" boilerplate.
- Revisit Stage 3 with the custom-API approach (`BKMidi` type, example-5 pattern) once we have real user scripts to test ergonomics.

### Exit criteria

- ChucKlavier downstream of a Keymap: notes played on the keyboard flow through the ChucK script and into a downstream Direct.
- Transposing script that adds +12 to every note works.

---

## Stage 4 — Tuning + Tempo interop

### Tasks

- Each block, host writes `tuningTable[128]` and `tempoBPM` to ChucK globals via `setGlobalFloatArray` / `setGlobalFloat`.
- Add a Tuning ↔ ChucKlavier and Tempo ↔ ChucKlavier connection type in `PreparationList` — same wiring as Direct's Tuning connection, but ChucKlavier reads the values from its upstream Tuning/Tempo processors and pushes them to the VM.
- **Deferred design work**: reflecting script-declared globals back as bitKlavier modulation targets. Add a manifest in the script (e.g. `// @bk_export cutoff: float [20,20000]`) parsed by the host to build a modulation UI. Prototype separately.

### Exit criteria

- Attaching a Tuning preparation to a ChucKlavier changes the pitch produced by a ChucK-side `SinOsc` driven by MIDI.

---

## Stage 5 — Polish

- Replace `juce::TextEditor` with `juce::CodeEditorComponent` + a `ChucKCodeTokeniser`.
- Add gain params to the modulation channel map (Blendronic pattern) so they're audio-rate modulatable.
- Bundled starter scripts / example gallery entries.
- Convert PNG icon to `Paths::chucklavier` SVG path for skin consistency.
- Script error highlighting (line numbers from ChucK's error output → editor).

---

## Open items (to resolve when we get there)

- Stage 2 first task is the **RT-safety audit of `ChucK::run()`**. If it allocates or locks in steady state, we either fork or move to a worker thread. That verdict shapes Stage 2's back half.
- Stage 3 MIDI approach may migrate from (a) globals to (b) custom API bindings based on script ergonomics.
- Modulation-of-script-globals is intentionally deferred to a post-Stage-5 design conversation.

## Suggested checkpoint pattern

PR-per-stage, each with the exit criteria as its acceptance test.

- Stage 1 is comfortably its own PR (touches ~10 files, all mechanical).
- Stage 2 is the biggest risk (third-party build integration + first live VM).
- Stages 3–5 are additive.
