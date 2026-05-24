# ResonanceBKSynthesiser — Multi-Threading Design

This document describes the design and implementation of `ResonanceBKSynthesiser`, a multi-threaded subclass of `BKSynthesiser` used exclusively by `ResonanceProcessor`.

## Background

Profiling (Instruments Time Profiler, resonance-heavy scenario) showed that `BKSynthesiser::renderVoices` / `BKSamplerVoice::renderNextBlock` accounts for ~98.6% of `ResonanceProcessor::processBlock` CPU time (~7.97s out of 52.4% total plugin CPU). All other work (MIDI handling, `ResonantString::ringString`, etc.) is negligible by comparison.

The fix is to parallelize `renderVoices` for the Resonance preparation only, using JUCE Audio Workgroups, without touching any other preparation.

**Implementation completed May 2026.**

## Design

### Class Hierarchy

```
BKSynthesiser
└── ResonanceBKSynthesiser   (overrides renderVoices)
```

`BKSynthesiser::renderVoices` must be marked `virtual` — the only required change to the base class.

`ResonanceProcessor` constructs `ResonanceBKSynthesiser` instead of `BKSynthesiser` — one line change in its constructor. A/B switching between implementations requires only changing that constructor line (or a runtime boolean flag).

### Threading Model

- K worker threads are created at construction time (K = number of logical cores, or a fixed value like 4)
- Each worker owns one pre-allocated scratch `AudioBuffer<float>` (never allocated in the audio callback)
- Voice distribution uses lock-free FIFOs (one per worker), populated by the main thread at the start of each block
- Workers accumulate their assigned voices into their scratch buffer
- Main thread waits at a `ThreadBarrier`, then sums all scratch buffers into the output buffer
- `someVoicesActive` is promoted to `std::atomic<bool>`

### JUCE Audio Workgroups

Worker threads are started with `juce::Thread::startRealtimeThread()` and join the host's `juce::AudioWorkgroup` via `WorkgroupToken` before processing each block. This ensures the OS scheduler treats them as real-time audio threads co-operating with the main audio thread.

**Platform note:** Audio Workgroups are macOS/iOS only. This is acceptable since bitKlavier targets macOS.

**Reference:** `JUCE/examples/Audio/AudioWorkgroupDemo.h`

## Key Constraints

- **No heap allocation in the audio callback** — all buffers and FIFOs are pre-allocated at construction
- **Lock safety:** The existing `juce::CriticalSection lock` in `BKSynthesiser::processNextBlock` is held during MIDI handling; `renderVoices` runs inside that lock. Workers operate on the voice array while the lock is held by the main thread — safe because the voice list is not modified during rendering
- **Worker thread lifetime:** Workers must be stopped and joined before the synthesiser is destroyed (destructor responsibility)
- **Scratch buffer sizing:** Scratch buffers must be resized in `prepareToPlay` to match the current block size and channel count — still no allocation in the audio callback

## Integration Points

| File | Change |
|---|---|
| `BKSynthesiser.h` | Mark `renderVoices` as `virtual` |
| `ResonanceBKSynthesiser.h/.cpp` | New class; override `renderVoices`; manage worker threads |
| `ResonanceProcessor.h/.cpp` | Construct `ResonanceBKSynthesiser` instead of `BKSynthesiser` |

## Status

- [x] Mark `BKSynthesiser::renderVoices` as `virtual`
- [x] Implement `ResonanceBKSynthesiser` skeleton
- [x] Implement lock-free FIFO voice distribution
- [x] Implement `ThreadBarrier`
- [x] Wire up JUCE AudioWorkgroup tokens
- [x] A/B performance comparison vs. single-threaded baseline
