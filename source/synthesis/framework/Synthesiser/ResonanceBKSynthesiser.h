//
// ResonanceBKSynthesiser.h
// Created for bitKlavier2
//
// A subclass of BKSynthesiser that parallelises voice rendering using JUCE Audio Workgroups.
//
// Design: overrides renderNextBlock() (called exactly once per audio callback) to:
//   1. Render all voices in parallel across K worker threads into pre-allocated scratch buffers.
//   2. Sum scratch buffers into the output.
//   3. Process MIDI events from the buffer to update voice state for the next block.
//
// This avoids the sub-block re-entrancy problem that arises when overriding renderVoices()
// (which processNextBlock() can call multiple times per block for MIDI accuracy).
//
// macOS/iOS only (AudioWorkgroup is an Apple API).
//

#pragma once

#include "BKSynthesiser.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <atomic>
#include <array>
#include <mutex>
#include <condition_variable>

static constexpr int ResonanceNumWorkerThreads = 4;

//==============================================================================
// ThreadBarrier: all N threads must call arriveAndWait() before any return.
// Identical to the JUCE AudioWorkgroupDemo.h implementation.
//
class ResonanceThreadBarrier : public juce::ReferenceCountedObject
{
public:
    using Ptr = juce::ReferenceCountedObjectPtr<ResonanceThreadBarrier>;

    static Ptr make (int numThreadsToSynchronise)
    {
        return { new ResonanceThreadBarrier { numThreadsToSynchronise } };
    }

    void arriveAndWait()
    {
        std::unique_lock<std::mutex> lk { mutex };

        [[maybe_unused]] const auto c = ++blockCount;
        jassert (c <= threadCount);

        if (blockCount == threadCount)
        {
            blockCount = 0;
            cv.notify_all();
            return;
        }

        cv.wait (lk, [this] { return blockCount == 0; });
    }

private:
    std::mutex mutex;
    std::condition_variable cv;
    int blockCount{};
    const int threadCount{};

    explicit ResonanceThreadBarrier (int n) : threadCount (n) {}

    JUCE_DECLARE_NON_COPYABLE (ResonanceThreadBarrier)
    JUCE_DECLARE_NON_MOVEABLE (ResonanceThreadBarrier)
};

//==============================================================================
class ResonanceBKSynthesiser : public BKSynthesiser
{
public:
    ResonanceBKSynthesiser (EnvParams& envParams, chowdsp::GainDBParameter& gainParam);
    ~ResonanceBKSynthesiser() override;

    //==========================================================================
    // Must be called from prepareToPlay() before the first audio block.
    // Sizes scratch buffers and (re)starts worker threads joined to the workgroup.
    void prepareWorkgroup (juce::AudioWorkgroup workgroup, int samplesPerBlock, double sampleRate, int numChannels);

    //==========================================================================
    // Override renderNextBlock (called exactly once per audio callback) to render
    // voices in parallel, then process MIDI events for the next block.
    void renderNextBlock (juce::AudioBuffer<float>& outputAudio,
                          const juce::MidiBuffer& inputMidi,
                          int startSample,
                          int numSamples) override;

    //==========================================================================
    // O(n) voice stealing override — replaces the base class O(n^2 log n) sort.
    // Finds the best candidate in a single linear pass without sorting.
    BKSynthesiserVoice* findVoiceToSteal (BKSynthesiserSound* soundToPlay,
                                          int midiChannel,
                                          int midiNoteNumber) const override;

private:
    //==========================================================================
    class WorkerThread;

    struct WorkerThreadOptions
    {
        juce::AudioWorkgroup workgroup;
        ResonanceThreadBarrier::Ptr barrier;
        int samplesPerBlock;
        double sampleRate;
    };

    //==========================================================================
    // One scratch buffer per worker thread. Sized in prepareWorkgroup().
    std::array<juce::AudioBuffer<float>, ResonanceNumWorkerThreads> scratchBuffers;

    std::array<std::unique_ptr<WorkerThread>, ResonanceNumWorkerThreads> workerThreads;

    // Barrier synchronising (ResonanceNumWorkerThreads + 1) threads
    ResonanceThreadBarrier::Ptr completionBarrier;

    int currentSamplesPerBlock = 512;
    int currentNumChannels     = 2;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ResonanceBKSynthesiser)
};
