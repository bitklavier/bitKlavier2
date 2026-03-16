# bitKlavier: the prepared digital piano
Developed by Dan Trueman, Michael Mulshine, Matt Wang and Davis Polito, 
with Theo Trevisan, Katie Chou, Jeff Gordon, Camy Streuly, and Myra Norton.


[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.3667650.svg)](https://doi.org/10.5281/zenodo.3667650)


bitKlavier takes inspiration from John Cage's *prepared piano*, but instead of screws and erasers we place a reconfigurable collection of digital machines between the virtual strings of the digital piano. Learn more at the [bitKlavier website](http://bitklavier.com).

NOTE: bitKlavier2 is the GitHub Repository name for bitKlavier versions 4 and higher, and was completely rewritten in 2025-26, superceding the [prior version](https://github.com/Princeton-CDH/bitKlavier) (which ran through 3.5) 
<br>
<br>

## License
This project is made available under the [GPLv3 license](https://www.gnu.org/licenses/quick-guide-gplv3.en.html)
<br>
<br>

## Development Setup

### Prerequisites

- **CMake** 3.25 or higher
- **C++20**-capable compiler
- **Xcode** (macOS) — required for AU/AUv3 targets and code signing
- **Git** with submodule support

### Cloning

Clone with submodules to pull in JUCE and other git-tracked dependencies:

```bash
git clone --recurse-submodules https://github.com/bitklavier/bitKlavier2.git
```

If you've already cloned without submodules:

```bash
git submodule update --init --recursive
```

### Building
In CLion, reload the CMake project after any change to the `VERSION` file (Tools → CMake → Reload CMake Project), then select the desired target from the target selector.

Otherwise, configure and build from the project root:

```bash
# Configure (Debug)
cmake -B cmake-build-debug -DCMAKE_BUILD_TYPE=Debug

# Build a specific target
cmake --build cmake-build-debug --target bitKlavier_Standalone
cmake --build cmake-build-debug --target bitKlavier_AU
cmake --build cmake-build-debug --target bitKlavier_VST3
```

With `COPY_PLUGIN_AFTER_BUILD` enabled (the default), built plugins are automatically copied to:
- AU: `~/Library/Audio/Plug-Ins/Components/`
- VST3: `~/Library/Audio/Plug-Ins/VST3/`

### External Libraries

Dependencies live in two places:

**`modules/`** — JUCE-format modules, included via `juce_add_module`:
- [melatonin_audio_sparklines](https://github.com/sudara/melatonin_audio_sparklines) — prints audio buffers to the console in debug builds via `melatonin::printSparkline(myAudioBlock)`

**`third_party/`** — libraries included via CMake `add_subdirectory`:
- [chowdsp_utils](https://github.com/Chowdhury-DSP/chowdsp_utils) — parameter management, DSP utilities, preset system, and GUI helpers; this is a core dependency throughout the codebase
- [melatonin_inspector](https://github.com/sudara/melatonin_inspector) — interactive UI component inspector; note that some features are limited due to OpenGL rendering
- tracktion_engine — selected utility files from [Tracktion Engine](https://github.com/Tracktion/tracktion_engine) (not the full library)
- juce-toys — LLDB helpers (`juce_lldb_xcode.py`) for more readable JUCE types in the debugger
- sfzq — SoundFont player

**`JUCE/`** — [JUCE 8](https://github.com/juce-framework/JUCE) framework, included as a git submodule.

https://git-lfs.com 

