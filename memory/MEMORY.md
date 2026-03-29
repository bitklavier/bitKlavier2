# bitKlavier2 Memory

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

## Piano Switching Performance (ConstructionSite)

Quick wins landed on branch `danwork8` (March 2026):
- Added `is_rebuilding_` flag to skip N redundant `updateScrollBars()` calls during `rebuildAllGui()` — now called once at the end of `setActivePiano()` instead of per-preparation
- Removed dead `setCentrePosition(s->curr_point)` in `moduleAdded` (was immediately overwritten by the scroll-adjusted version)

**Deferred: per-piano component cache**
See `memory/piano_cache.md` for full design and trade-offs. Key risks that need solving before implementing:
- Cache invalidation: `prep_list->removeListener(this)` is called when a piano is hidden, so ValueTree changes (add/delete prep, undo) while it's cached won't update the cache → stale restore
- `all_sliders_` map accumulation: `removeSubSection` doesn't clean those maps, so cached pianos leave stale entries that could affect modulation display
- Memory: all pianos' GL textures and component trees stay alive simultaneously
