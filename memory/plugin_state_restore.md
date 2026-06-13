# Plugin State Restore & Standalone/Plugin Isolation (fixed Jun 13 2026, dandev)

Symptoms in Logic AU before fix:
1. Session restore: gallery menu name correct, but actual loaded content was Basic Piano. Yellow signal cables not visible even when content was correct.
2. Two bitKlavier tracks each with different galleries restored to the same gallery.
3. Last gallery loaded in Logic dictated which gallery the standalone opened next.
4. Prelude 1 (Inside Out) restore triggered Logic's "Audio Unit reported a problem" instability warning — went away once the restore race below was fixed; it was symptom, not separate bug.

## Five compounding bugs

### 1. Logic AU `prepareToPlay` can fire BEFORE `setStateInformation`
The "conforming host calls setStateInformation first" assumption is not reliable in Logic AU. Pre-queue checks (`!setStateWasCalled_`) only see state at queue time. **The queued lambda must re-check at fire time.**

In `PluginProcessor::prepareToPlay`:
```cpp
if (!setStateWasCalled_ && !defaultLoadAttempted_) {
    defaultLoadAttempted_ = true;
    juce::MessageManager::callAsync([this]() {
        // Re-check inside the lambda — pre-queue check is only a fast path.
        if (setStateWasCalled_ || isSamplesLoading() || hasPendingPreset())
            return;
        // ... default Basic Piano load ...
    });
}
```

### 2. `loadFromFile` immediate path must clear `pendingPresetTree` from any in-flight deferred load
`loadFromFile` has two branches: deferred (samples not yet cached → sets `pendingPresetTree`, returns; `finishedSampleLoading` applies it later) and immediate (samples already cached → applies tree now).

Race sequence that bit us:
1. `prepareToPlay` callAsync → `loadFromFile(BasicPiano)` deferred path → `pendingPresetTree = BasicPiano`, samples loading.
2. `setStateInformation` arrives → `loadFromFile(SavedGallery)`. Yamaha already cached from step 1's loadSamples call → takes immediate path → tree replaced with SavedGallery.
3. **Old code did not clear `pendingPresetTree`** → still = BasicPiano.
4. BasicPiano's async job finishes → `finishedSampleLoading` applies stale `pendingPresetTree = BasicPiano` → overwrites SavedGallery in tree.

**Fix:** immediate path must invalidate any prior pending tree:
```cpp
if (presetPending.load()) {
    presetPending.store(false);
    pendingPresetTree = juce::ValueTree{};
}
```

Pattern: any time you have a deferred-async + immediate path to the same shared state, the immediate path must invalidate the deferred path's pending data.

### 3. Cables invisible because OpenGL context not attached when editor-ctor `setActivePiano` ran
`Cable::connectionAdded` routes visibility setup through `OpenGLContext::executeOnGLThread`, which **silently no-ops** if the GL context isn't attached. PluginEditor ctor runs before the host adds the editor to its window → GL not attached → setVisible callAsync never queued → cables stay invisible.

**Fix:** defer `setActivePiano` via `callAsync` from the editor ctor, and re-read `getActivePianoValueTree()` inside the lambda (not captured) to avoid stale-tree:
```cpp
juce::WeakReference<SynthGuiInterface> weakThis(this);
juce::MessageManager::callAsync([weakThis]() {
    if (auto* self = weakThis.get()) {
        if (self->getSynth()->isSamplesLoading() || self->getSynth()->hasPendingPreset())
            return;
        self->setActivePiano(self->getSynth()->getActivePianoValueTree());
    }
});
```

### 4. `getStateInformation` was reading the shared `user_prefs->tree.last_gallery_path` for the per-track save block
`UserPreferencesWrapper::userPreferences` is `juce::SharedResourcePointer<UserPreferences>` — **one instance per process**, shared by every bitKlavier in the host. When Logic asked Track A and Track B to serialize, both read the same property (whichever was last loaded won) and wrote the same path into their separate state blocks. On restore each track read its own block — both contained the same path — both loaded the same gallery.

**Fix:** `getStateInformation` writes per-instance `getActiveFile().getFullPathName()`. `setStateInformation` already reads from the per-track block, so restore needed no change.

### 5. `SynthBase::loadFromFile`/`saveToFile` wrote to `user_prefs` regardless of plugin vs standalone
Persists to `~/Library/Application Support/bitklavier/bitklavier.settings`, which standalone reads at startup → loading a gallery in Logic dictated standalone's next-launch gallery and polluted its recents menu.

**Fix:** added `virtual bool SynthBase::isStandaloneApp() const { return false; }`, overridden to `true` in `SynthEditor`. The three write sites in `synth_base.cpp` (deferred-load path, immediate-load path, save-to-file path) are gated behind it. Standalone keeps its remember-last-gallery behavior; plugin can't touch the file.

## `samplesLoaded` flag semantics

`SampleLoadManager::loadSamples` returns `true` on the "already loading" branch too — so the old code's `samplesLoaded = synth_->sampleLoadManager->loadSamples(...)` in `SynthGuiInterface` ctor flipped it true before any samples actually finished. PluginEditor consults this flag to decide whether to hide the "Samples Loading…" overlay.

**Fix:** `samplesLoaded = true` is now written only inside `finishedSampleLoading` (next to `samplesLoading.store(false)`). The ctor only sets it speculatively for the cache-hit branch where the soundset was already loaded.

## Key files
- `source/PluginProcessor.cpp` — `prepareToPlay` (re-check at fire time); `getStateInformation` (per-instance `getActiveFile()`, not shared user_prefs)
- `source/PluginEditor.cpp` — deferred `setActivePiano` via callAsync (cable visibility)
- `source/common/synth_gui_interface.cpp` — removed stale `setActivePiano` callAsync ctor; restore-aware `loadSamples`/`samplesLoaded`
- `source/synthesis/synth_base.h` — `hasPendingPreset()`, `isSamplesLoading()`, `virtual bool isStandaloneApp() const`
- `source/synthesis/synth_base.cpp` — immediate-path clears stale `pendingPresetTree`; explicit `setActivePiano` after immediate load; `samplesLoaded` authoritative write moved to `finishedSampleLoading`; `user_prefs` writes gated by `isStandaloneApp()`
- `source/standalone/synth_editor.h` — `bool isStandaloneApp() const override { return true; }`

## Lessons / patterns

- **Process-wide singletons in plugin code are landmines.** Anything reached via `juce::SharedResourcePointer` (or other process-wide statics) is shared across all plugin instances in the host. Per-instance state must come from per-instance sources (e.g. `active_file_`, the ValueTree owned by `this`).
- **Plugin vs standalone behavior split** should use a `virtual bool isStandaloneApp() const` (override in `SynthEditor`), not a runtime check on `juce::JUCEApplication::getInstance()` etc.
- **Logic AU is the canary host.** If something assumes "host calls X before Y", verify in Logic — many setups that work fine in REAPER/Live break here. Test scenario: AU in Logic, save Logic session, quit Logic, reopen Logic.
- **Multi-instance plugin testing** is its own scenario: two tracks in one session, each with different state. Easy to forget when single-track works.
- **callAsync pre-checks are insufficient when the async target is a long-lived flag transition.** Re-check at fire time.
- **executeOnGLThread silently no-ops if GL context isn't attached.** When work depends on GL being live (visibility, texture init), it has to wait until after the host adds the editor to its window. Defer via callAsync from ctors.
- **Misleading symptoms:** Logic's "Audio Unit reported a problem" warning was caused by this restore race, not by anything specific to the gallery that triggered it. Don't assume an instability warning means a separate bug.
