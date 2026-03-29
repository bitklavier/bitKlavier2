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

## Piano Switching Performance (ConstructionSite)

Quick wins landed on branch `danwork8` (March 2026):
- Added `is_rebuilding_` flag to skip N redundant `updateScrollBars()` calls during `rebuildAllGui()` — now called once at the end of `setActivePiano()` instead of per-preparation
- Removed dead `setCentrePosition(s->curr_point)` in `moduleAdded` (was immediately overwritten by the scroll-adjusted version)

**Deferred: per-piano component cache**
See `memory/piano_cache.md` for full design and trade-offs. Key risks that need solving before implementing:
- Cache invalidation: `prep_list->removeListener(this)` is called when a piano is hidden, so ValueTree changes (add/delete prep, undo) while it's cached won't update the cache → stale restore
- `all_sliders_` map accumulation: `removeSubSection` doesn't clean those maps, so cached pianos leave stale entries that could affect modulation display
- Memory: all pianos' GL textures and component trees stay alive simultaneously
