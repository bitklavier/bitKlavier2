# Sample loading lifecycle and failure handling

Non-obvious things about `SampleLoadManager` that aren't visible from a single call site.

## The "Samples Loading…" overlay lifecycle

The overlay is shown by `SynthBase::startSampleLoading()` and is **only** hidden via this chain:

```
~SampleLoadJob()
  → loadManager->triggerAsyncUpdate()
  → SampleLoadManager::handleAsyncUpdate()        (message thread)
  → parent->finishedSampleLoading()
  → FullInterface::hideLoadingSection()
```

There is no other path that dismisses the overlay. Consequence: **if `startSampleLoading()` is called but no `SampleLoadJob` is ever queued, the overlay hangs forever** — the destructor never fires, async update never fires, the chain stalls. This was the launch hang with a missing `Yamaha_Default`. The fix (v4.9.34 area) is to pre-validate via `validateSoundset()` and only call `startSampleLoading()` after at least one job is actually added to the pool.

`progress->totalJobs == 0` at the end of `loadSamples_sub()` rounds is the canonical "we queued no work" signal — bail and surface the failure instead of waiting.

## SampleLoadJob has two failure modes

`SampleLoadJob::runJob()` must distinguish them:

| Job type | Detection | Permanent failure return | Transient failure return |
|----------|-----------|--------------------------|--------------------------|
| Soundfont | `sampleReaderVector.empty() && sfzFile.getFullPathName().isNotEmpty()` | `jobHasFinished` + set `progress->hadFailure` | n/a — there is no transient |
| Pitch sample | otherwise | n/a | `jobNeedsRunningAgain` (resumes via `continueValue`) |

Before the fix, **soundfont permanent failures returned `jobNeedsRunningAgain`** → busy-loop. The pitch path's `jobNeedsRunningAgain` is legitimate (transient I/O), leave it alone.

## Two-tier alert path

`SampleLoadManager::loadSamples(name, tree, bool reportErrorsHere = true)`:
- `reportErrorsHere = true` (default): pre-validation failure posts its own `AlertWindow` immediately. Right for single‑user actions (launch, picking a soundset from the menu).
- `reportErrorsHere = false`: caller wants to aggregate failures. Used by `SynthBase::loadFromFile()`, which collects failed names from the prepass loop and shows ONE consolidated alert listing all of them.

Job-side failures (file existed at validate-time but didn't parse) flow through `SampleSetProgress::hadFailure` (std::atomic<bool>, set on the worker thread, read in `handleAsyncUpdate` on the message thread). They always alert via `handleAsyncUpdate` — there is no aggregation path for those because they're rare and the message thread is the only place we know they happened.

## Soundfont "successful parse with zero regions"

`SFZSound::load_regions()` + `load_samples()` succeed quietly on junk `.sf2` files — they just produce zero regions. `loadSoundFont` must explicitly check `sound->num_regions() == 0` AFTER the iterate-regions loop and return false in that case, otherwise the empty sound gets moved into `sfzBanks` and the soundset is silently dead.

## `synth_->samplesLoaded` is "load was initiated", not "done"

`SynthGuiInterface` ctor: `bool firstLoad = !synth_->samplesLoaded; … synth_->samplesLoaded = loadSamples(...)`. The flag is set from the return value of `loadSamples`, which returns true when the load was successfully queued (not when sample data is actually in memory). Used to gate `showLoadingSection()` and to skip re-loading on plugin window reopen.

## BKPianoSampleType subdir naming

`bitklavier::utils::BKPianoSampleType_string` in `source/common/utils.h:270` is `{"/main", "/hammer", "/resonance", "/pedal"}` — **lowercase, leading slash**. Easy to assume "Main" capitalized; it isn't. `validateSoundset()` hardcodes the same four names without the slash since it builds paths via `getChildFile()`.

## Key files
- `source/synthesis/SampleLoadManager.h/cpp` — `validateSoundset`, `SoundsetLoadStatus`, `postSoundsetLoadAlert`, `loadSamples`, `handleAsyncUpdate`, `SampleLoadJob::runJob`, `loadSoundFont`
- `source/synthesis/synth_base.cpp` — `loadFromFile` (aggregated alert), `startSampleLoading`/`finishedSampleLoading`
- `source/common/synth_gui_interface.cpp` — `if (firstLoad && synth_->samplesLoaded) showLoadingSection()`
- `source/interface/sections/loading_section.h` — the overlay itself
