# Windows Port: Status and Next Steps

Branch: `windows-port` (off `main` at v5.3.1). **Not merged.** Last verified CI run: **41719185963**
(commit `de86e0af`): macOS and Windows jobs both green.

## Where things stand

- **Builds:** the Windows job (`windows_build` in `.github/workflows/build_and_test.yml`) configures, compiles and links
  Standalone and VST3 with MSVC (VS 2022+, Ninja, Release). Tests and Benchmarks targets also link.
- **Packaging:** the Windows job uploads `bitKlavier-windows-standalone` (the .exe) and `bitKlavier-windows-vst3`
  (~7.5 MB each, unsigned, no samples). Get them with `gh run download <run-id>`.
- **macOS:** builds, links, packages, notarizes and staples on the branch.
- **Never run:** nothing from this branch has been *run* on Windows, and macOS has not been runtime-tested since the
  shared-code changes below. CI only proves "it compiles". **Testing both platforms is the next step.**

## What was changed

### Windows-only (guarded; macOS unaffected)
- `CMakeLists.txt`: `FORMATS` = Standalone + VST3, AU/AUv3 only `if(APPLE)`; Metal frameworks only `if(APPLE)`;
  `-O0` link option skipped on MSVC; **LTO disabled on MSVC** (with `/GL` the static lib exceeded 4 GB, LNK1248).
- `cmake/JUCEDefaults.cmake`: embedded debug info only for Debug/RelWithDebInfo (keeps Release libs small).
- `third_party/chuck-cmake/CMakeLists.txt`: Apple-only multitouch stub moved under `if(APPLE)`; Windows gets
  `HAVE_WIN32_THREADS` (embedded liblo) and links `winmm ws2_32 iphlpapi dinput8 dxguid`.
- `AssocVector.h`: MSVC branch supplies the typedefs that `std::__binary_function` provided.
- `sfzq/SFZPlayer/SFZSynth.cpp`: MSVC-only replacement for POSIX `random()`.
- `.github/workflows/build_and_test.yml`: new `windows_build` job (`continue-on-error`, not a dependency of `release`,
  builds with `-k 0` so one run lists every error), plus artifact upload.

### Shared code (affects macOS too; these are what the macOS test must cover)
- `#include <bitset>` (and `<array> <atomic> <tuple>` in `utils.h`) in ~24 files that used `std::bitset` without it.
- `utils.h`: `VelocityRange<N>` primary template now has an initializer (`= {}`); MSVC rejected the bare `constexpr`.
- **Forward declarations made to match definitions** (`struct`, not `class`) for `ModulationConnection`,
  `StateConnection`, `TuningState`, `PopupItems`. MSVC mangles the class-key; the mismatch caused an unresolved symbol.
- `envelope_editor.cpp`: `powerScale` made `static` (duplicate symbol with `curve_look_and_feel.cpp`).
- `popup_browser.cpp:1455`: `juce::String(var)` replaced with `var.toString()`.
- `CompressorProcessor.cpp`: `<simd/common.h>` behind `__has_include` (it was unused beyond the include).
- `synth_editor.cpp`: `setMacMainMenu` calls wrapped in `#if JUCE_MAC`.
- `sfzq/RIFF.cpp`: VLA replaced with a `std::string` buffer.
- **Functions that fell off the end without returning (undefined behavior everywhere; MSVC C4716):**
  `ModulationItem::hitTest` and `SynthBase::isConnected` now return the value they computed (matches what clang happened
  to produce); `setDefaultBipolar`/`setDestParamIndex`/`setCurrentTotalBaseValue` return their argument; stub bodies
  (`LoadSave::getFavoritesFile/getAdditionalFolders/getDataDirectory`, `ParametersView::getComponentForParameter`,
  `getPrepPopup`, `getImageComponent`, `SynthBase::addPlugin`, five `getTailLengthSeconds`) return `{}`/`nullptr`/`0.0`;
  vendored `chowdsp_BaseSerializer::deserialize` now returns `x`.
- **New `source/common/UserPaths.h`:** `expandUserPath()` resolves `~/Documents/...` through
  `juce::File::userDocumentsDirectory` (follows OneDrive-redirected Documents on Windows; identical on macOS). Replaces two
  duplicate local `expandUserPath`s, three hardcoded `<home>/Documents/bitKlavier/galleries` paths, and the `~` handling
  in `header_section.cpp`.
- CI: `stapler staple` retried up to 5x (Apple CloudKit "network connection was lost", error 68, hit this branch
  repeatedly). Applies to `main` builds too.

## Testing still to do

### macOS (regression check of the shared-code changes above)
**Results so far (2026-11-04, build from CI run 41719185963, tested by Dan):**
- Passed: Compressor; reload of last gallery on launch; Gallery menu (lists Basic Piano plus subfolders);
  opening/saving galleries; protected-gallery behavior for Basic Piano; ChucKlavier example galleries;
  soundfont playback; **pluginval on Release AU and VST3 (both pass)**.
- Modulation tests, in `~/Documents/bitKlavier/galleries/Tests/` (not in the repo): `LFO_test`, `RampModSave_test`
  and `ResonanceStateMod_tests` work fully.
- `StateModSave_test` has an issue that **predates the Windows port** (confirmed by Dan against an earlier build), so it is
  not a regression from this branch. Track it separately.
- `SynchronicHost_test` (host tempo) works.
- SF2 soundfont names/playback read correctly (this exercises the `RIFF.cpp` VLA fix, the only sfzq change that
  compiles on macOS). No `.sfz` files were available to test; the `.sfz`-specific change (`random()` in `SFZSynth.cpp`)
  is MSVC-only, so macOS is unaffected.
- **macOS testing is complete.** Remaining gate before merge: Windows runtime testing.
- Note: `CLAUDE.md` names test galleries (`RampModSave_test.bk2`, etc.) that are not in the repo; they live in the
  Tests folder above. `test-presets/` in the repo holds other presets (e.g. `RampModAlternate_*`).

Original checklist:
- Launch standalone; load `Prelude 1 (Inside Out)` etc. and confirm sound plays.
- Galleries listed in `CLAUDE.md`: `RampModSave_test.bk2`, `StateModSave_test.bk2`, `SynchronicHost_test.bk2`, `LFO_test.bk2`.
- Specifically exercise: Compressor, SFZ/soundfont playback (sfzq), modulation connections (ModulationItem hit-testing,
  `isConnected`), the Gallery menu, Open/Recent/last-gallery restore, protected-gallery behavior (Basic Piano), ChucKlavier.
- pluginval on the AU and VST3 (Release), per `CLAUDE.md`.

### Windows
- Standalone launches; audio device selection (WASAPI; ASIO needs the Steinberg SDK + `JUCE_ASIO`, not set up).
- Load a gallery, play; check samples/soundfonts/galleries resolve under `Documents\bitKlavier` (incl. OneDrive case).
- OpenGL UI: rendering, mouse interaction (the macOS mouse-forwarding JUCE patch is Apple-only), idle GPU/CPU.
- **Resonance (multithreaded):** ~300 voices on 4 workers with no audio-workgroup support on Windows. Check for dropouts.
- ChucKlavier (ChucK/liblo/DirectInput were only compiled and linked, never run).
- VST3 loads in a DAW (REAPER etc.); plugin scanner subprocess; MTS-ESP (dynamic `libMTS` load; Windows path untested).
- Legacy gallery import, gallery save/load round trip, keyboard shortcuts (`ConstructionSite.cpp:1538` has a `JUCE_MAC` block).

## Known risks / open work (not started)
- **Resonance threading:** consider thread priority / MMCSS on Windows if it glitches.
- **Installer:** `packaging/installer.iss` (Inno Setup) exists but is unverified; needs samples/resources delivery (the macOS
  postinstall flow does not carry over) and Azure trusted-signing secrets. The old Windows CI steps are still commented
  out in the matrix; the new `windows_build` job does not build an installer.
- **Intel IPP:** optional; the Windows path is not exercised in CI.
- **Warnings:** the build uses `-w`, so MSVC warnings (many C4244 etc.) have not been reviewed.
- **Code signing / SmartScreen:** Windows binaries are unsigned.
- `sfzq` `random()` replacement is a simple `rand()` combiner; fine for velocity-layer randomization, not high quality.

## CI notes
- Windows job result does not affect the overall run (`continue-on-error`). macOS flakiness is almost always the
  Apple staple step (now retried); runner-shutdown cancellations also occurred once.
- `concurrency: cancel-in-progress` cancels older runs on a new push to the same branch.
- Merge criteria suggested: macOS runtime test passes, at least one Windows tester confirms launch + sound, then merge
  `windows-port` into `main` (or open a PR). Delete the `continue-on-error` and decide whether `windows_build` should
  gate releases only once Windows is considered supported.
