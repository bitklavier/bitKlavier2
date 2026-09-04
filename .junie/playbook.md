# Project Playbook (macOS)

## Environment

### Variables
- `CMAKE_COMMAND`: `/Applications/CLion.app/Contents/bin/cmake/mac/aarch64/bin/cmake`
- `CTEST_COMMAND`: `/Applications/CLion.app/Contents/bin/cmake/mac/aarch64/bin/ctest`

---

## Modules

### bitKlavier2

#### Summary
Digital prepared piano audio plugin and standalone application built with JUCE 8. It uses the PampleJuce CMake framework.

#### How to Build
```bash
/Applications/CLion.app/Contents/bin/cmake/mac/aarch64/bin/cmake -B cmake-build-debug -DCMAKE_BUILD_TYPE=Debug
/Applications/CLion.app/Contents/bin/cmake/mac/aarch64/bin/cmake --build cmake-build-debug --target bitKlavier5_Standalone -j4
```

#### How to Run Tests
```bash
/Applications/CLion.app/Contents/bin/cmake/mac/aarch64/bin/cmake --build cmake-build-debug --target Tests -j4
/Applications/CLion.app/Contents/bin/cmake/mac/aarch64/bin/ctest --test-dir cmake-build-debug
```

#### How to Run Single Test
```bash
cd cmake-build-debug && ./Tests "MTS-ESP coordinator: default state"
```

#### Run / Check

- **Standalone Application**:
  `cmake-build-debug/bitKlavier5_artefacts/Debug/Standalone/bitKlavier5.app/Contents/MacOS/bitKlavier5`
- **AU Plugin**:
  `cmake-build-debug/bitKlavier5_artefacts/Debug/AU/bitKlavier5.component`
- **VST3 Plugin**:
  `cmake-build-debug/bitKlavier5_artefacts/Debug/VST3/bitKlavier5.vst3`

#### Notes
- **Target Naming**: The binary suffix (e.g., `bitKlavier5`) is determined by the major version in the `VERSION` file.
- **Bundled Tools**: Uses CMake and CTest bundled with CLion.
- **Test Failures**: `Plugin instance` test currently fails due to a hardcoded name check for "bitKlavier" instead of "bitKlavier5".
- **Dependencies**: Core dependencies like `chowdsp_utils` are managed via CMake `add_subdirectory` and JUCE modules.

---

## Tools
- **CMake**: 3.25+ (via CLion bundle)
- **Catch2**: Unit testing framework (v3)
- **JUCE**: Audio application framework (v8)
- **chowdsp_utils**: Parameter and DSP utility library

## Notes
- The build automatically copies plugins to `~/Library/Audio/Plug-Ins/` if `COPY_PLUGIN_AFTER_BUILD` is enabled (default).
- CLion integration is the preferred development workflow.
