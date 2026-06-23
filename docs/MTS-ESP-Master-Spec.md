# MTS-ESP Master / Publishing Support — Implementation Spec

**Status:** Approved design, not yet implemented
**Scope:** Master/publishing mode only. Receive/client mode is explicitly out of scope for phase 1.
**Last updated:** 2026-06-22

---

## 0. Locked design decisions

These were confirmed by the project owner and override any conflicting recommendation below:

1. **Spring tuning is in scope and must publish continuously during note sustain.** Adaptive tuning types ride the same continuous mechanism. The old bitKlavier had continuous spring behavior; we must match/replace it — but, unlike the old code, **all MTS-ESP work stays off the audio thread**.
2. **Ship the `libMTS` installer with bitKlavier** so master mode works out of the box.
3. **`BITKLAVIER_ENABLE_MTS_ESP` defaults ON** (desktop macOS/Windows/Linux). The SDK degrades gracefully when the runtime library isn't installed.
4. **The selected master keeps publishing across active-piano switches**, even when the Tuning's graph node is bypassed. No stop/restart thrash on piano changes.

---

## 1. User-facing behavior

- Each Tuning preparation exposes an **"MTS-ESP Master"** control plus a status line.
- Selection is **exclusive across the whole gallery/plugin instance**: only one Tuning can be the MTS-ESP master source at a time. Selecting a Tuning implicitly deselects any other.
- Clicking when not selected → selects this Tuning as the master source. Clicking when already selected → clears it.
- bitKlavier publishes its tuning table so other MTS-ESP-aware plugins in the session follow bitKlavier's tuning, **including live changes** while Spring/Adaptive tunings evolve during sustain.
- If another app/plugin already owns the MTS-ESP master role (including a second bitKlavier instance), bitKlavier fails gracefully and shows a "blocked" status.
- A "Reinitialize MTS-ESP" affordance appears **only** when `MTS_CanRegisterMaster()` is false **and** `MTS_HasIPC()` is true (stale shared memory after a DAW crash), per ODDSound guidance.

### Status strings (UI)
- `Off` — no master selected.
- `Selected` — this Tuning is selected but not currently publishing (e.g. registration in flight).
- `Publishing` — registered and pushing tunings. Optionally show client count.
- `Blocked — another MTS-ESP master is running`.
- `MTS-ESP library not installed` — SDK present at compile time, runtime dylib missing.
- `Disabled in this build` — `BITKLAVIER_ENABLE_MTS_ESP` was OFF.

---

## 2. State model

### Saved (authoritative intent)
A single property on the root `IDs::GALLERY` ValueTree — sibling concept to the existing `BUSEQ` / `BUSCOMPRESSOR` / `BUSREVERB` root children:

```
GALLERY.mtsMasterTuningUuid = "<tuning-uuid>"   // absent/empty = no master selected
```

- New identifier `IDs::mtsMasterTuningUuid` in `source/common/Identifiers.h`.
- **No per-Tuning boolean.** A single UUID is the entire saved exclusive-selection state.
- Serialized for free by the existing gallery save path.

### Transient (runtime, never saved) — held by the coordinator
```cpp
enum class MtsStatus {
    Disabled,                 // compile-time off
    LibraryMissing,           // runtime dylib absent
    Off,                      // no selection
    Selected_NotPublishing,   // selected, registration not held yet
    Publishing,               // registered + pushing
    Blocked_OtherMasterExists // another master owns the slot
};

std::atomic<MtsStatus> status;
bool   weHoldMasterRegistration = false;  // did THIS coordinator call MTS_RegisterMaster
juce::Uuid selectedMasterUuid;            // mirror of saved property
std::atomic<bool> republishPending{false};// event-driven republish request
```

Saved intent ("the user wants this Tuning to drive MTS") is separate from runtime success ("we actually hold the master slot"), which can fail at any time.

---

## 3. Coordinator & wrapper classes

Two layers; the ODDSound SDK header is included in exactly one translation unit.

### `MTSESPMasterWrapper` (SDK-facing, thin)
Location: `source/synthesis/framework/MTSESP/MTSESPMasterWrapper.{h,cpp}`. The **only** file that includes `libMTSMaster.h`.

```cpp
bool canRegister();                          // MTS_CanRegisterMaster
bool registerMaster();                       // MTS_RegisterMaster (+ verify)
void deregisterMaster();                     // MTS_DeregisterMaster
bool hasIPC();                               // MTS_HasIPC
void reinitialize();                         // MTS_Reinitialize
void setNoteTunings(const double freqs[128]);// MTS_SetNoteTunings
void setScaleName(const char* name);         // MTS_SetScaleName (optional)
int  numClients();                           // MTS_GetNumClients
```

When `BITKLAVIER_ENABLE_MTS_ESP` is OFF, every method is a no-op and `canRegister()` returns false.

### `MTSESPMasterCoordinator` (policy/lifecycle, no SDK headers)
Location: `source/synthesis/framework/MTSESP/MTSESPMasterCoordinator.{h,cpp}`. Owns one wrapper and a `juce::Timer` (message thread). Owned by `SynthBase` (one per plugin instance).

Public API:
```cpp
void   setSelectedMasterTuningUuid(juce::Uuid);
void   clearSelectedMasterTuning();
juce::Uuid getSelectedMasterTuningUuid() const;
bool   isTuningSelectedAsMaster(juce::Uuid) const;
MtsStatus getStatus() const;

void   notifyTuningChanged(juce::Uuid);   // event-driven republish (static/scala)
void   notifyTuningDeleted(juce::Uuid);   // clears selection if it matched

// engine wiring
void   setTuningLookup(std::function<TuningProcessor*(juce::Uuid)>); // resolve uuid -> live processor
void   loadSelectionFromTree(const juce::ValueTree& galleryRoot);    // on gallery load
```

> **Process-global caveat:** `MTS_RegisterMaster()` claims a single *per-process* slot with no handle. Two bitKlavier instances in one DAW share that slot; the second instance's `canRegister()` returns false and it shows `Blocked_OtherMasterExists`. Phase 1 only needs to fail cleanly. A future process-wide arbiter could allow graceful hand-off.

---

## 4. Tuning processor responsibilities

- Provide a **side-effect-free** snapshot of its current 128-note tuning:
  ```cpp
  void TuningState::fillTuningTable(double out[128], float a4) const;
  ```
  This must NOT mutate `lastFrequencyTarget`, `lastFrequencyHz`, `spiralNotes`, `lastMidiNote`, etc. (the existing `getTargetFrequency` writes those and runs on the audio thread — calling it from the coordinator's message thread would race). Implement a `const` computation path that mirrors static / scala / spring / adaptive frequency math without touching shared playback state.
  - Static / Scala: read the atomic offset arrays + scala tables.
  - Spring: read `springTuner->getFrequency(note, a4)` (already lock-protected; safe off-thread).
  - Adaptive: compute from current adaptive state snapshot.
- Expose whether its tuning type is **dynamic** (Spring / Adaptive / Adaptive-Anchored → continuous publishing) or **static** (Static / Scala → event-driven publishing).
- On a state change, set the coordinator's `republishPending` flag (via the existing `TuningListener::tuningStateInvalidated` / parameter listeners). Setting an atomic flag is the only cross-thread action; **no SDK call from the processor**.

---

## 5. MTS-ESP wrapper responsibilities

- Encapsulate all SDK calls and all `#ifdef BITKLAVIER_ENABLE_MTS_ESP` guards.
- Translate SDK outcomes into `MtsStatus` for the coordinator.
- Provide the dlopen-style behavior (vendored `libMTSMaster.cpp` already does this): missing runtime dylib → no-op, surfaced as `LibraryMissing`.
- Optionally set a human-readable scale name from the Tuning's system/fundamental.

---

## 6. Threading model

**Hard rule: no MTS-ESP call, allocation, lock, or IPC ever runs on the audio thread.**

| Action | Thread | Mechanism |
|---|---|---|
| User selects/clears master | Message | UI → coordinator → write `GALLERY.mtsMasterTuningUuid` |
| Register / deregister / reinitialize | Message | coordinator on selection change, load, shutdown |
| Detect "selected tuning changed" | Audio **or** HRT (spring) | set `republishPending` atomic only — no SDK |
| Build 128-note table + `MTS_SetNoteTunings` | Message | coordinator `juce::Timer` tick |
| Status read for UI | any | `std::atomic<MtsStatus>` |

### Publish cadence (coordinator `juce::Timer`)
The coordinator runs a single `juce::Timer` on the message thread. Per tick:
- No selection / library unavailable → do nothing.
- Selected master is **static/scala** → publish only when `republishPending` is set (event-driven; cheap).
- Selected master is **dynamic** (Spring/Adaptive) → publish **every tick** (continuous), giving live updates during sustain.

Recommended timer rate ≈ **50–100 Hz** to track the spring simulation smoothly. The spring sim itself runs on a `juce::HighResolutionTimer` (dedicated thread) via `SpringTuning::hiResTimerCallback → simulate()`; the coordinator reads spring frequencies through the lock-protected `getFrequency()`, which is safe from the message thread but must never be called from the audio thread.

Each publish builds a `double[128]` on the stack (no heap) via `fillTuningTable`, then one `MTS_SetNoteTunings` call.

### Why not the old approach
The old code published spring tunings per-note from the audio thread (`getOffset` / `MTS_SetNoteTuning`) and ultimately gated MTS *off* when springs were active. We replace that with a message-thread snapshot pump, achieving continuous updates without any audio-thread SDK calls.

---

## 7. Save / load behavior

- **Save:** `mtsMasterTuningUuid` is a root-tree property → serialized automatically. Runtime status is not saved.
- **Load:** after `loadFromValueTree`, coordinator calls `loadSelectionFromTree`:
  - UUID resolves to a live Tuning → register (deferred, message thread) + initial publish.
  - UUID doesn't resolve (deleted/stale) → clear the property, status `Off`.
- **ValueTree reentrancy:** snapshot the UUID before the first `setProperty` when clearing, per the project's known setProperty-reentrancy gotcha.

---

## 8. Deletion behavior

- The `PreparationList` Tuning-removal path calls `coordinator->notifyTuningDeleted(uuid)`.
- If it matches the selected UUID: clear the saved property, deregister master, set status `Off`, drop any cached `TuningProcessor*` (no dangling pointer).
- Non-matching deletions are ignored.

---

## 9. Active-piano switching behavior

- Per decision (4): the selected master **keeps publishing regardless of bypass state**. Tuning nodes persist in the graph across piano switches (just bypassed); the coordinator resolves the selected Tuning by UUID independent of which piano is active, so publishing continues uninterrupted.
- For dynamic tunings, the spring/adaptive sim continues to advance and the coordinator keeps pumping snapshots.

---

## 10. External-conflict behavior

- Before registering, check `canRegister()`. If false:
  - `hasIPC()` true → offer "Reinitialize MTS-ESP", then re-register.
  - else → status `Blocked_OtherMasterExists`; keep the saved selection (intent) but don't hold the slot. Retry on next selection action or load.
- Another bitKlavier instance counts as an external master (process-global slot).

---

## 11. Build / packaging integration

- CMake option `BITKLAVIER_ENABLE_MTS_ESP` (default **ON**).
- Vendor ODDSound `libMTSMaster.{h,cpp}` under `third_party/MTS-ESP/`; compile the wrapper TU; isolate include paths.
- Ship the `libMTS` runtime installer (`libMTSMac*.pkg` / Windows / Linux) with the bitKlavier installer so master mode works out of the box. The installer rolls into the existing CI/installer flow (see `CLAUDE.md` CI/CD section). When the runtime lib is absent, the wrapper no-ops and the UI shows `LibraryMissing`.
- Project must still build and run with the option OFF (all wrapper methods no-op, UI shows `Disabled`).

---

## 12. File-by-file staged implementation plan

Each stage is independently buildable. Stages map to the planning prompts in `docs/MTS-ESP Dev Prompts.md`.

**Stage A — Build integration**
- `third_party/MTS-ESP/` — vendor `libMTSMaster.{h,cpp}` + runtime installer assets.
- `CMakeLists.txt` — add `BITKLAVIER_ENABLE_MTS_ESP` (default ON), wire wrapper TU + include path. Verify OFF build.

**Stage B — Wrapper + coordinator skeleton (no-op)**
- `source/synthesis/framework/MTSESP/MTSESPMasterWrapper.{h,cpp}`.
- `source/synthesis/framework/MTSESP/MTSESPMasterCoordinator.{h,cpp}`.
- `source/synthesis/synth_base.{h,cpp}` — own `std::unique_ptr<MTSESPMasterCoordinator>`; construct/destruct; accessor; provide `setTuningLookup`.

**Stage C — Saved selection state**
- `source/common/Identifiers.h` — add `IDs::mtsMasterTuningUuid`.
- `synth_base.cpp` — read/write root property; `loadSelectionFromTree` on load; clear-on-missing.
- `source/common/ObjectLists/PreparationList.cpp` — call `notifyTuningDeleted` on Tuning removal.

**Stage D — Tuning UI**
- `source/interface/ParameterView/TuningParametersView.{h,cpp}` — MTS button + status label; wire to coordinator; refresh in existing `timerCallback()`; reinitialize affordance gated on `!canRegister() && hasIPC()`.

**Stage E — Snapshot + publish path**
- `TuningProcessor.{h,cpp}` — add side-effect-free `fillTuningTable(double out[128], float a4) const` covering static/scala/spring/adaptive; dynamic-vs-static type query; set `republishPending` from listeners.
- Coordinator — implement the `juce::Timer` pump: continuous for dynamic masters, event-driven for static.

**Stage F — Real SDK calls**
- `MTSESPMasterWrapper.cpp` — implement register/deregister/setNoteTunings/canRegister/hasIPC/reinitialize; map to `MtsStatus`.
- `source/PluginProcessor.cpp` — coordinator deregisters in the processor destructor (parallels old `Gallery::deregisterMTS`).

**Stage G — Hardening & audio-thread-safety review** (Prompts 10/11)
- Lifecycle/edge-case pass; confirm zero audio-thread SDK/allocation/lock; no dangling `TuningProcessor*`; no stale UUID.

**Stage H — User manual entry** (Prompt 12).

---

## 13. Test plan

### Unit (Catch2, `tests/`)
- Selecting Tuning A then B leaves exactly one saved UUID; selecting the same twice clears it.
- `fillTuningTable` for a known ET/Scala scale at A440 yields expected Hz (A4 = 440, etc.).
- `fillTuningTable` is verifiably side-effect-free (does not alter `spiralNotes`/`lastFrequencyHz`).
- Deleting the selected Tuning clears the saved UUID and sets status `Off`.
- Load with a stale UUID → cleared, status `Off`, no crash.

### Manual / integration
- Audio-thread audit (and debug assert in wrapper) confirms no SDK calls off the message thread.
- With `libMTS` installed: select master, open an MTS-ESP client; client follows bitKlavier. Edit a Static tuning → client updates once.
- **Spring continuous test:** select a Spring tuning as master, hold a chord; client pitches track the spring drift smoothly during sustain. Same for Adaptive.
- Two bitKlavier instances → second shows `Blocked`. External DAW master present → `Blocked`. Kill DAW with IPC active, relaunch → reinitialize affordance recovers.
- `BITKLAVIER_ENABLE_MTS_ESP=OFF` build compiles, runs, UI shows `Disabled`.
- Save/reload a gallery with a selected master → republishes on load.
- Active-piano switch with a master selected → publishing continues without interruption (decision 4).
- `pluginval` (Release) AU + VST3 pass; standalone smoke test.

---

## 14. Open items / future work

- Receive/client mode (phase 2).
- Process-wide master arbiter for graceful multi-instance hand-off.
- Optional `MTS_SetScaleName` population and note-filtering for unmapped notes.
- Optional multi-channel (`MTS_SetMultiChannel*`) support.
