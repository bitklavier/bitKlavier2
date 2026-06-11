---
name: busreverb-save-bug
description: BUSREVERB power state (activeReverb) not saved/restored with gallery — fixed Jun 11 2026
metadata:
  type: project
---

**Status:** Fixed Jun 11 2026. Save-path fix in `syncBusProcessorsToValueTree` re-links orphaned `proc->v` before serializing. Load-path adds explicit `activeReverb = false` reset when the loaded gallery has no `<BUSREVERB>` (legacy-gallery case) so power matches fresh-startup behavior.


The static (bus) Reverb processor's `activeReverb` (BoolParameter, paramID "reverbActive") does not persist across gallery save/load.

**Why:** `addDefaultChain` in `source/synthesis/sound_engine/sound_engine.cpp` creates `<BUSREVERB>` (and BUSEQ/BUSCOMPRESSOR) at startup and points `reverbProcessor->v` at it. When a gallery is loaded, `SynthBase::loadFromValueTree` calls `tree.copyPropertiesAndChildrenFrom(state)` which removes all existing children and re-adds children from the loaded state. If the loaded gallery lacks `<BUSREVERB>` (legacy/older save, or one previously broken by this bug), `loadBusProcessorsFromValueTree` early-returns on the invalid child and `reverbProcessor->v` is left pointing at the now-orphaned VT (not in the live tree). On next save, `syncBusProcessorsToValueTree` writes serialized params to the orphan VT, and `tree.createXml()` produces a file with no `<BUSREVERB>`. Bug self-perpetuates.

**Confirmed via:** `/tmp/rev.bk2` (user-supplied copy of `~/Documents/bitKlavier/galleries/Tests/ReverbSave_test2.bk2`) — has `<BUSEQ>` and `<BUSCOMPRESSOR>` at root but no `<BUSREVERB>`. Also has a per-piano `<reverb>` preparation inside `<PIANO>/<PREPARATIONS>` (unrelated — those work).

**Attempted fix #1 (in stash, REVERTED):** Modified `loadBusProcessorsFromValueTree` to create a missing BUSREVERB on load. Caused/coincided with a startup hang at the "Samples Loading..." popup. Reverted.

**Attempted fix #2 (in stash, current top of stash):** Modified only `syncBusProcessorsToValueTree`:
- Added `juce::ValueTree& rootTree` param; `synth_base.cpp:899` passes `tree`.
- In the lambda, if `proc->v.isValid() == false` or `proc->v.getParent() != rootTree`, create a fresh `BUSREVERB`/`BUSEQ`/`BUSCOMPRESSOR` child with type/uuid/soundset, append to rootTree, point proc->v at it before serializing.
- LOAD path is byte-identical to origin/dandev.

Header sig change required: `sound_engine.h:684` `void syncBusProcessorsToValueTree (juce::ValueTree& rootTree);`

**Why this fix is save-only-safe:** `loadBusProcessorsFromValueTree` is unchanged, so startup gallery auto-load (which triggers fix #1's hang) is unaffected. The new code only runs from `SynthBase::saveToFile`.

**Startup hang investigation:** User reports both attempted-fix builds AND the clean baseline (origin/dandev = v4.9.31 = a79a6882) hang at sample loading today, but yesterday's binary of the same v4.9.31 source works. Likely cause: stale incremental build state in `cmake-build-debug` from compiling several intermediate code versions today (DBG instrumentation, load-path fix, save-path fix). Resolution path: quit/restart CLion → reload CMake → if still hangs, `rm -rf cmake-build-debug` and reconfigure. Yesterday's installed AU/VST3 plugins at `~/Library/Audio/Plug-Ins/{Components,VST3}` (timestamped Jun 10 15:04) work fine.

**To resume after rebuild succeeds:**
1. `git stash pop` to restore fix #2.
2. Verify startup still works.
3. Open `ReverbSave_test2.bk2`, toggle Reverb power, save.
4. Check saved file contains `<BUSREVERB ... reverbActive="1" ...>`.
5. Reload the file and confirm power state persists.

**Related code:** [[bus_processor_save_load]] (in main MEMORY.md as "Bus Processor Save/Load (CompressorProcessor + EQProcessor)") — covers the same save/sync mechanism for EQ and Compressor.
