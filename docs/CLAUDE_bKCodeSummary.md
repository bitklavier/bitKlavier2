# bitKlavier2 - Claude Code Guide

## Project Overview

bitKlavier 2 is a digital prepared piano audio plugin and standalone application built with JUCE 8. It uses the [PampleJuce](PampleJuceREADME.md) CMake framework to avoid Projucer. The plugin supports AU, VST3, AUv3, and Standalone formats.

- **Version:** Stored in the `VERSION` file (current: 4.0.1)
- **Bundle ID:** `com.manyarrowsmusic.bitklavier`
- **Manufacturer Code:** `Bitk` / Plugin Code: `B202`

## Build System

**Requirements:** CMake 3.25+, C++20, Xcode (macOS), Ninja (CI)

```bash
# Configure (debug)
cmake -B cmake-build-debug -DCMAKE_BUILD_TYPE=Debug

# Build standalone
cmake --build cmake-build-debug --target bitKlavier_Standalone

# Build AU
cmake --build cmake-build-debug --target bitKlavier_AU

# Build VST3
cmake --build cmake-build-debug --target bitKlavier_VST3

# Run tests
cmake --build cmake-build-debug --target Tests
ctest --test-dir cmake-build-debug
```

After build, plugins are auto-copied (`COPY_PLUGIN_AFTER_BUILD TRUE`):
- AU: `~/Library/Audio/Plug-Ins/Components/`
- VST3: `~/Library/Audio/Plug-Ins/VST3/`

In CLion: select targets like `bitKlavier4_AU` or `bitKlavier4_Standalone` from the target selector.

**CMake reload required** after changing the `VERSION` file: Tools -> CMake -> Reload CMake Project.

## Project Structure

```
source/
├── PluginProcessor.h/cpp       # JUCE AudioProcessor entry point
├── PluginEditor.h/cpp          # JUCE AudioProcessorEditor entry point
├── common/                     # Shared utilities (load_save, tuning utils, etc.)
├── interface/                  # All UI/GUI code
│   ├── components/             # Reusable JUCE components (SynthSlider, etc.)
│   ├── sections/               # Major UI panels
│   ├── Preparations/           # Per-preparation UI panels
│   ├── ParameterView/          # Parameter display components
│   └── look_and_feel/          # Skin, fonts, OpenGL shaders
├── synthesis/                  # Audio engine
│   └── framework/Processors/   # Per-preparation audio processors
└── modulation/                 # Modulation system (UI + backend)

third_party/
├── chowdsp_utils/              # DSP/GUI utilities, parameter system
├── melatonin_inspector/        # UI component inspector (limited with OpenGL)
└── tracktion_engine/           # Utility files (not full engine)

modules/
└── melatonin_audio_sparklines/ # Console audio debug printing

JUCE/                           # JUCE 8.0.12 (git submodule)
assets/                         # SVGs, fonts, default.bitklavierskin
tests/                          # Catch2 unit tests
```

## Architecture

### Audio Engine

The audio graph is set up in `source/synthesis/sound_engine.h/cpp`. Each preparation type has a corresponding `*Processor` class in `source/synthesis/framework/Processors/`. Preparations are always in the graph; the active piano determines which are bypassed.

Key classes:
- `SoundEngine` — top-level audio graph manager
- `SynthBase` — base class with ValueTree state management
- `BKSynthesiser` — custom JUCE synthesizer
- `PluginProcessor` — inherits from both `juce::AudioProcessor` and `SynthBase`

### UI Architecture

- `PluginEditor` inherits from `juce::AudioProcessorEditor` and `SynthGuiInterface`
- OpenGL rendering is used extensively; `FullInterface` owns the OpenGL context
- `ConstructionSite` is the drag-and-drop patching area
- `SynthSlider` and `StateModulatedComponent` are the two UI types that support modulation

### Parameter System

Uses **chowdsp_utils** for parameter management (not APVTS directly). Parameters are defined in `*Params` structs inside each `*Processor.h`.

Debugging parameter listeners: set a breakpoint in `chowdsp_ParameterListeners.cpp` -> `parameterValueChanged`.

### Modulation System

Two types of modulation:
1. **Audio-rate (continuous)** — via `SynthSlider`; uses `AudioProcessorGraph` modulation bus channels. Pass `true` as the last arg to `FloatParameter` constructor. See `DirectProcessor.h` for a full example.
2. **State modulation** — via `StateModulatedComponent`; changes discrete/complex params atomically. Must inherit `StateModulatedComponent`, implement `clone()` and `syncToValueTree()`, and call `SynthSection::addStateModulatedComponent`.

For modulation to work, the parameter ID and the component ID (`Component::setComponentID`) **must match**.

The modulation bus channel count must be set precisely: `22 * 2` channels for 22 continuously modulatable params (x2 for ramp + LFO). See `tuningBusLayout()`.

## Versioning

Version is set in the `VERSION` file (e.g., `4.0.1`). The major number sets the binary suffix (`bitKlavier4`). After changing:
1. Edit `VERSION`
2. Update `juce::String getApplicationName()` in `main.cpp`
3. Reload CMake

## Testing

**Framework:** Catch2 v3.8.1

Test files are in `tests/`. Key gallery files to test manually:
- `RampModSave_test.bk2`
- `StateModSave_test.bk2`
- `Prelude 1 (Inside Out)` — Release build; verify sound plays and active piano is set in plugin formats
- `SynchronicHost_test.bk2` — verify host tempo
- `LFO_test.bk2`

Plugin validation: use `pluginval` with Release builds. AU and VST3 must pass all tests.

```bash
lldb -- /Applications/pluginval.app --validate ~/Library/Audio/Plug-Ins/Components/bitKlavier0.component
# then: run, then: bt
```

Note: plugin scanning may only work in Release builds.

## Debugging

- **LLDB setup:** `~/.lldbinit` needs `settings set target.load-cwd-lldbinit true` for ValueTree display. See `.lldbinit` in project root.
- **ValueTree debugger:** Uncomment `valueTreeDebugger = new ValueTreeDebugger(...)` in `FullInterface.cpp`.
- **Audio sparklines:** `melatonin::printSparkline(myAudioBlock)` in debug builds.
- **Plugin debugging in CLion:** Set `bitKlavier4_VST3` executable to REAPER, use Debug CMake profile.
- **Attach to process:** Run -> Attach to Process in CLion.

## Skin / Visual Constants

- Default skin values: `assets/default.bitklavierskin`
- Color name-to-enum mapping: `skin.cpp` and `skin.h` (must stay in sync)
- UI helper functions: `SynthSection::getKnobSectionHeight()` etc. in `synth_section.cpp`
- Keyboard key colors: `BKOnOffKeyboardComponent`

## Adding a New Preparation

See `docs/bitKlavierDevNotes.md` for the full checklist. High-level steps:
1. Create `*Processor`, `*ParametersView`, `*Preparation` classes
2. Add to `BKPreparationTypes` in `common.h`
3. Create item class in `BKItem.h/.cpp`, add icon path in `getPathForPreparation`
4. Add to popup menu in `synth_gui_interface.cpp`
5. Register in `ConstructionSite.cpp` (factory, CommandIDs, perform, getAllCommands, getCommandInfo)
6. Register in `PreparationList.cpp` constructor
7. Add SVG icon layers in `assets/<prepname>/`, add `path()` in `paths.h`
8. Set popup size in `FullInterface::resized()`

## Adding a Parameter to a Preparation

See `docs/bitKlavierDevNotes.md` for full details. Key points:
- Define chowdsp param in the `*Params` struct in `*Processor.h`
- Pass `true` as last arg if audio-rate modulatable
- Call `parent.getStateBank().addParam` in the processor constructor if state-modulatable
- Create `SynthSlider` or attachment in `*ParametersView.h`, place it in `resized()`
- Add to the modulation channel map via `modChan.setProperty` calls

## CI/CD

GitHub Actions workflow: `.github/workflows/build_and_test.yml`
- Runs on every push; builds, signs (macOS), notarizes, and uploads installer artifact
- Installer found under GitHub Actions -> most recent run -> bottom of page
- Set `INCLUDE_RESOURCES: true/false` in the yml to toggle sample inclusion (installer is ~1.13GB with samples)
- A `bitKlavier_InstallerResources.zip` on the siteGround server is rolled into the installer when resources are included; update it before pushing if resources changed

### Code Signing (macOS)

Certificates expire and must be regenerated from Xcode -> Settings -> Account -> Manage Certificates. Export `.p12`, convert to base64 (`base64 -i cert.p12 | pbcopy`), update in GitHub Secrets:
- `DEVELOPER_ID_APP_CERT` / `DEVELOPER_ID_APP_PASSWORD`
- `DEVELOPER_ID_INSTALLER_CERT` / `DEVELOPER_INSTALLER_PASSWORD`

Notarization debugging:
```bash
xcrun notarytool log <submission-id> --keychain-profile dtrueman@princeton.edu > log.json
# Or with explicit credentials:
xcrun notarytool log <submission-id> --apple-id "dtrueman@princeton.edu" --team-id "YOURTEAMID" --password "abcd-efgh-ijkl-mnop" > log.json
```

## Key External Dependencies

| Library | Purpose |
|---|---|
| JUCE 8.0.12 | Audio plugin framework |
| chowdsp_utils | Parameter system, DSP, preset management |
| melatonin_inspector | UI debugging (limited with OpenGL) |
| melatonin_audio_sparklines | Console audio visualization (debug) |
| Catch2 3.8.1 | Unit testing |
| spdlog | Logging |
| magic_enum / nameof | Enum/name reflection |

## Performance Notes

Key idle CPU issues:
- `FullInterface.cpp`: `setContinuousRepainting(true)` and `setSwapInterval(0)` cause high idle GPU/CPU usage
- Gallery loading bottleneck: `ConstructionSite::setActivePiano` does a full teardown/rebuild (acceptable in Release)

## Real-Time Programming Resources

- https://forum.juce.com/t/lock-free-real-time-stuffs-for-dummies/58870/13
- Never allocate on the audio thread; use lock-free queues for cross-thread communication
