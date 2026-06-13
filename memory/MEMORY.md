# bitKlavier2 Memory

> **Note for Claude:** Always save new memories here (repo `memory/` folder) and update this file. The user-level auto-memory at `~/.claude/projects/.../memory/` is secondary.

## State Modulation — Known Fixes (March 2026)

### Nostalgic TransposeParams not applied (fixed on danwork9)
`NostalgicProcessor::processBlock` was calling `state.params.processStateChanges()` which hit the no-op base `StateChangeableParameter` implementation. Fix: call each sub-holder explicitly — `state.params.transpose.processStateChanges()` and `state.params.holdTimeMinMaxParams.processStateChanges()` — matching the Direct pattern. Any new state-modulatable param holder in Nostalgic (or other preps) must be called explicitly here.

### State mod fires one block late when same key triggers note + state change (fixed on danwork9)
Root cause: state-only connections (no continuous/audio-rate mod) add no audio edge in the `AudioProcessorGraph`, so JUCE's topological sort has no constraint — DirectProcessor ran before ModulationProcessor.

Fix (three files):
- `synth_base.cpp` `connectStateModulation`: add a MIDI graph edge `ModulationProcessor → dest` using `juce::AudioProcessorGraph::midiChannelIndex`; store in `connection->connection_`. `disconnectModulation(StateConnection*)`: remove that edge only when no other state connection shares the same pair.
- `ModulationProcessor.h`: `producesMidi()` returns `true` (JUCE's `canConnect()` rejects MIDI edges from processors where this is false).
- `ModulationProcessor.cpp` end of `processBlock`: `midiMessages.clear()` — prevents Keymap MIDI from leaking downstream as duplicate note events through the ordering edge.

### Ramp mod fires one block late when same key triggers note + ramp change (fixed on danwork9)
Two separate issues, both needed fixing:

**Issue 1 — Graph ordering:** `connectModulation` adds an audio bus edge (ModProc output → dest input) but the audio edge alone is unreliable for ordering. Without a guaranteed constraint, ModulationProcessor could run after DirectProcessor.
- `ModulationConnection.h`: added `midi_ordering_conn_` field to store the MIDI ordering edge (analogous to `StateConnection::connection_`).
- `synth_base.cpp` `connectModulation(ModulationConnection*)`: after adding the audio edge, also add MIDI ordering edge `ModulationProcessor → dest`, store in `midi_ordering_conn_`. `disconnectModulation(ModulationConnection*)`: remove edge only when no other ramp-mod or state-mod connection uses the same pair (both disconnect functions now cross-check the other list).

**Issue 2 — Ramp time minimum:** `RampParams::time` had a 10ms minimum, so the ramp's first sample value was ~0.00227 (essentially zero). `processContinuousModulations` reads only the first sample (`*in`), so no modulation was applied on the first press. The carry mechanism made the second press work.
- `RampModulator.h`: changed range min/default from 10ms to 0ms.
- `RampModulator.cpp` `setTime()`: added `timeToDest <= 0` branch that does `value_ = target_; state_ = 0` — instant jump. The existing `timeToDest < 1.0` floor remains to guard non-zero near-zero values.

## VST Parameter Modulation (Phase 1 + 3 complete, dandev branch)

### Architecture
A `VSTModulationBridge` AudioProcessor is inserted alongside every VST plugin node in the AudioProcessorGraph. Graph topology:
```
ModulationProcessor --[mod bus]--> VSTModulationBridge
VSTModulationBridge --[MIDI order edge]--> VST Plugin
```
The bridge reads modulation bus channels (block-accurate) and calls `param->setValue()` directly on VST parameters.

### Key design decisions
- Bridge VT stored as `IDs::vstbridge` child of the VST's state ValueTree (NOT as a sibling in PREPARATIONS) — avoids polluting PREPARATIONS, survives save/load naturally
- Bridge UUID stored as `IDs::bridgeUuid` on the VST state for NodeID stability across reloads
- `connectModulation(const juce::ValueTree&)` pre-check and `connectModulation(ModulationConnection*)` both do a one-level recursive fallback search when `preparations.getChildWithProperty(uuid)` returns invalid, to find the bridge VT (grandchild of PREPARATIONS)
- Fixed max 32 modulatable params (64 bus channels) for stable graph topology
- Slot index (integer string "0".."31") used as param ID in MODULATABLE_PARAMS to avoid underscore parsing issues in `connectModulation`

### Files changed
- `source/common/Identifiers.h` — added `IDs::vstbridge`, `IDs::bridgeUuid`
- `source/synthesis/framework/Processors/VSTModulationBridge.h/.cpp` — new files
- `source/common/ObjectLists/PreparationList.h` — added `bridgeNodeID` to `PluginInstanceWrapper`
- `source/common/ObjectLists/PreparationList.cpp` — bridge creation/cleanup in createNewObject/deleteObject
- `source/synthesis/synth_base.cpp` — recursive bridge VT search in BOTH `connectModulation` overloads (pre-check + full connect)
- `source/common/ObjectLists/PreparationList.h` — save crash fix: check `IDs::PLUGIN` child BEFORE calling `data.toString()` (binary VST state is not valid UTF-8)

### Phase 2 (complete)
- `VSTParametersView.h` (`source/interface/ParameterView/`): header-only SynthSection with `VSTParamAttachment` (bidirectional `juce::AudioProcessorParameter::Listener` + `juce::Slider::Listener`); sliders laid out in 4-column grid, `kMaxVSTModParams` rows max
- `PluginPreparation.cpp/h`: `getPrepPopup()` now constructs `VSTParametersView` (finds bridge via `state.getProperty(IDs::bridgeUuid)` → NodeID → `VSTModulationBridge*` cast); `mouseDoubleClick` opens native window AND calls `showPrepPopup` for the param view
- `VSTModulationBridge`: added `setBaseValue(slot, value)` / `getBaseValue(slot)` for attachment updates

### Workflow
1. User double-clicks VST prep → native PluginWindow opens + `VSTParametersView` opens in `prep_popup`
2. User double-clicks ModulationPrep → `ModulationModuleSection` opens in `mod_popup`
3. User drags from mod popup to any slider in `VSTParametersView` → `connectModulation` creates the connection via bridge's `MODULATABLE_PARAMS` + bus channels

### Bug fix: modAmt_=0 for new connections (fixed dandev)
`ModulationManager::setModulationValues` was updating UI sliders via `dontSendNotification` but never calling `connection->setScalingValue()`. For new connections, `modAmt_` starts at 0 so the bridge received 0 offset and VST params never changed.

Fix: at end of `setModulationValues`, call `connection->setScalingValue(amount, sliderVal)` where `sliderVal` is `slider_model_lookup_[destination]->getValue()` (falling back to `connection->currentDestinationSliderVal`). The `sliderValueChanged` path already called `setScalingValue` before `setModulationValues` — the redundant second call is harmless.

Also fixed: `VSTModulationBridge::acceptsMidi()` and `producesMidi()` now return `true` so MIDI ordering edges (ModProc→Bridge, Bridge→VST) can be added by the graph.

## Mute/Solo System

See `memory/mute_solo_system.md`. All 8 sound-making preps have M+S buttons. Key points:
- 4 atomics per params struct: `muted_` (effective), `userMuted_` (intent), `soloed_`, `soloMuted_` (coordinator-imposed)
- `IMuteSolable` interface in `source/synthesis/framework/Processors/IMuteSolable.h`
- `SynthBase::coordinateSoloChanged` / `coordinateMuteChanged` coordinate across the active prep list
- M button onClick reads `userMuted_` (not toggle state — button blinks during solo-mute)
- Solo state is transient, not saved to galleries

## macOS Sonoma: setVisible() During Mouse Event Dispatch Cancels Drag

See `memory/setvisible_during_event_dispatch.md`. Two-layer fix: (1) JUCE patch forwards mouseDown/Up/Dragged from NSOpenGLView to superview (root cause — NSOpenGLView swallows left-mouse events on Sonoma); (2) ConstructionSite defers beginLasso to first mouseDrag (any setVisible during mouseDown cancels drag). CI was silently skipping the JUCE patch due to git safe.directory — see [[ci-git-apply-safe-directory]].

## GitHub Actions: git apply in submodule silently fails without safe.directory

See `memory/ci_git_apply_safe_directory.md`. Always add `git config --global --add safe.directory <submodule>` before any git command in a submodule directory in CMake. Never use ERROR_QUIET when non-zero return has multiple meanings.

## BUSREVERB power state not saving/restoring (fixed Jun 11 2026)

See `memory/busreverb_save_bug.md`. Root cause: after `tree.copyPropertiesAndChildrenFrom(state)` during gallery load, if the loaded file lacks `<BUSREVERB>`, `reverbProcessor->v` is left orphaned and subsequent saves write to the dangling VT, perpetuating the missing-BUSREVERB state. Save-path fix in `syncBusProcessorsToValueTree` detects orphaned `proc->v` and creates a fresh child in the live tree before serializing. Load-path adds an explicit `activeReverb = false` reset when the loaded gallery has no `<BUSREVERB>` so legacy galleries match fresh-startup behavior (power off).

## State-mod clone constructors must not deref `params` (fixed Jun 12 2026)

See `memory/clone_constructor_null_params.md`. The private no-arg clone constructors for `OpenGL_ClusterMinMaxSlider` and `OpenGL_HoldTimeMinMaxSlider` dereferenced `params` (always null on the clone path), crashing on indicator click. HoldTime also had a typo in its literal max (`12000.f` vs the real `120000.f`) that was masked on the regular path but tripped `setSkewFactorFromMidPoint` once the deref was removed. Pass the shared `*_rangeMin/Max` constants to the parent `BKRangeSlider` constructor, never literals.

## Sample loading lifecycle and failure handling (fixed Jun 13 2026)

See `memory/sample_load_lifecycle.md`. The "Samples Loading…" overlay is only dismissed via `~SampleLoadJob → triggerAsyncUpdate → handleAsyncUpdate → finishedSampleLoading` — so calling `startSampleLoading()` without queuing a job hangs forever. Pre-validate via `validateSoundset()` and only show the overlay after a job is queued. Also covers: `SampleLoadJob::runJob` two failure modes (soundfont = permanent → `jobHasFinished` + `progress->hadFailure`; pitch path = transient → `jobNeedsRunningAgain` via `continueValue`); soundfont silent-success-with-zero-regions edge; `loadSamples`'s `reportErrorsHere` flag for caller-aggregated alerts; `samplesLoaded` semantic (load was initiated, not finished); `BKPianoSampleType_string` subdir names are lowercase with leading slashes.

## Piano Switching Performance (ConstructionSite)

Quick wins landed on branch `danwork8` (March 2026):
- Added `is_rebuilding_` flag to skip N redundant `updateScrollBars()` calls during `rebuildAllGui()` — now called once at the end of `setActivePiano()` instead of per-preparation
- Removed dead `setCentrePosition(s->curr_point)` in `moduleAdded` (was immediately overwritten by the scroll-adjusted version)

**Deferred: per-piano component cache**
See `memory/piano_cache.md` for full design and trade-offs. Key risks that need solving before implementing:
- Cache invalidation: `prep_list->removeListener(this)` is called when a piano is hidden, so ValueTree changes (add/delete prep, undo) while it's cached won't update the cache → stale restore
- `all_sliders_` map accumulation: `removeSubSection` doesn't clean those maps, so cached pianos leave stale entries that could affect modulation display
- Memory: all pianos' GL textures and component trees stay alive simultaneously
