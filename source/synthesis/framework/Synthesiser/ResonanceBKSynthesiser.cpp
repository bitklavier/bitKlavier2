//
// ResonanceBKSynthesiser.cpp
// Created for bitKlavier2
//

#include "ResonanceBKSynthesiser.h"

//==============================================================================
// WorkerThread: wakes on a WaitableEvent (auto-reset, acts as binary semaphore),
// renders its assigned voices into its scratch buffer, then participates in the
// completion barrier. Called exactly once per audio block — no re-entrancy.
//==============================================================================
class ResonanceBKSynthesiser::WorkerThread final : private juce::Thread
{
public:
    using Thread::isThreadRunning;

    explicit WorkerThread (const WorkerThreadOptions& opts)
        : juce::Thread ("ResonanceBKSynthWorker"),
          options (opts)
    {
        startRealtimeThread (
            juce::Thread::RealtimeOptions{}.withApproximateAudioProcessingTime (
                options.samplesPerBlock, options.sampleRate));
    }

    ~WorkerThread() override
    {
        signalThreadShouldExit();
        startEvent.signal();   // unblock the thread so it can see threadShouldExit()
        stopThread (-1);
    }

    //==========================================================================
    // Called from the audio thread before signalStart().
    // Sets the per-block context and queues voice pointers.
    int prepareAndQueueVoices (juce::Span<BKSynthesiserVoice* const> voiceSpan,
                               juce::AudioBuffer<float>* scratchBuf,
                               int startSample,
                               int numSamples)
    {
        pendingScratchBuffer.store (scratchBuf, std::memory_order_relaxed);
        pendingStartSample.store   (startSample, std::memory_order_relaxed);
        pendingNumSamples.store    (numSamples,  std::memory_order_relaxed);

        size_t spanIndex = 0;
        const auto write = voiceFifo.write ((int) voiceSpan.size());
        write.forEach ([&] (int dstIdx)
        {
            voiceQueue[(size_t) dstIdx] = voiceSpan[spanIndex++];
        });

        return write.blockSize1 + write.blockSize2;
    }

    // Called once per block after all voices have been queued.
    // Auto-reset WaitableEvent: at most one pending wakeup, cannot accumulate.
    void signalStart()
    {
        startEvent.signal();
    }

private:
    void run() override
    {
        juce::WorkgroupToken token;
        if (options.workgroup)
            options.workgroup.join (token);

        while (! threadShouldExit())
        {
            // Block until the audio thread signals one block of work
            startEvent.wait();

            if (threadShouldExit())
                break;

            auto* scratchBuf   = pendingScratchBuffer.load (std::memory_order_acquire);
            const int start    = pendingStartSample.load   (std::memory_order_acquire);
            const int nSamples = pendingNumSamples.load    (std::memory_order_acquire);

            if (scratchBuf != nullptr)
            {
                voiceFifo.read (voiceFifo.getNumReady()).forEach ([&] (int srcIdx)
                {
                    voiceQueue[(size_t) srcIdx]->renderNextBlock (*scratchBuf, start, nSamples);
                });
            }

            options.barrier->arriveAndWait();
        }
    }

    static constexpr int maxVoicesPerWorker = 512;

    WorkerThreadOptions options;

    // Auto-reset: signal() stores at most one pending wakeup
    juce::WaitableEvent startEvent { false };

    std::array<BKSynthesiserVoice*, maxVoicesPerWorker> voiceQueue {};
    juce::AbstractFifo voiceFifo { maxVoicesPerWorker };

    std::atomic<juce::AudioBuffer<float>*> pendingScratchBuffer { nullptr };
    std::atomic<int> pendingStartSample { 0 };
    std::atomic<int> pendingNumSamples  { 0 };

    JUCE_DECLARE_NON_COPYABLE (WorkerThread)
    JUCE_DECLARE_NON_MOVEABLE (WorkerThread)
};

//==============================================================================
BKSynthesiserVoice* ResonanceBKSynthesiser::findVoiceToSteal (BKSynthesiserSound* soundToPlay,
                                                               int /*midiChannel*/,
                                                               int midiNoteNumber) const
{
    jassert (!voices.isEmpty());

    const juce::ScopedLock sl (stealLock);

    // Identify the lowest and highest non-released notes to protect them.
    BKSynthesiserVoice* low = nullptr;
    BKSynthesiserVoice* top = nullptr;

    for (auto* voice : voices)
    {
        if (voice->canPlaySound (soundToPlay) && !voice->isPlayingButReleased())
        {
            const auto note = voice->getCurrentlyPlayingNote();
            if (low == nullptr || note < low->getCurrentlyPlayingNote()) low = voice;
            if (top == nullptr || note > top->getCurrentlyPlayingNote()) top = voice;
        }
    }

    if (top == low)
        top = nullptr;

    // Single linear pass: track the best candidate in each priority tier.
    // In the MT path, voices set up in the current MIDI-processing step have not
    // yet rendered (envelopeVal == 0). Stealing them produces a silent graveyard
    // slot that dies instantly. We therefore add an extra "unrendered" tier below
    // the normal tiers — use those only as a last resort, and they get hard-cut
    // in startVoice (no graveyard needed for a silent voice).
    BKSynthesiserVoice* oldestReleased    = nullptr; // released (no key/pedal)
    BKSynthesiserVoice* oldestNoFinger    = nullptr; // key up but still sounding
    BKSynthesiserVoice* oldestUnprotected = nullptr; // any non-protected, rendered voice
    BKSynthesiserVoice* oldestUnrendered  = nullptr; // not yet rendered (envVal == 0)

    for (auto* voice : voices)
    {
        if (!voice->canPlaySound (soundToPlay))
            continue;

        jassert (voice->isVoiceActive());

        // Exact pitch match: ideal steal target, return immediately.
        if (voice->getCurrentlyPlayingNote() == midiNoteNumber)
            return voice;

        if (voice == low || voice == top)
            continue;

        // Skip voices started in the same noteOn burst — they haven't rendered yet
        // and stealing them produces a silent graveyard slot. They are inaudible
        // so a hard-cut is fine; the caller (startVoice) will detect envVal==0.
        const bool startedThisBurst = (voice->noteOnTime >= currentNoteOnBurstStart);
        const bool hasRendered = (voice->getAmpEnvValue() > 0.0f);

        if (startedThisBurst || !hasRendered)
        {
            // Unrendered or started this burst — lowest priority, any is fine
            if (oldestUnrendered == nullptr || voice->wasStartedBefore (*oldestUnrendered))
                oldestUnrendered = voice;
            continue;
        }

        if (voice->isPlayingButReleased())
        {
            if (oldestReleased == nullptr || voice->wasStartedBefore (*oldestReleased))
                oldestReleased = voice;
        }

        if (!voice->isKeyDown())
        {
            if (oldestNoFinger == nullptr || voice->wasStartedBefore (*oldestNoFinger))
                oldestNoFinger = voice;
        }

        if (oldestUnprotected == nullptr || voice->wasStartedBefore (*oldestUnprotected))
            oldestUnprotected = voice;
    }

    if (oldestReleased    != nullptr) return oldestReleased;
    if (oldestNoFinger    != nullptr) return oldestNoFinger;
    if (oldestUnprotected != nullptr) return oldestUnprotected;
    if (oldestUnrendered  != nullptr) return oldestUnrendered;

    // Only protected voices remain — steal top first, then low.
    jassert (low != nullptr);
    return (top != nullptr) ? top : low;
}

//==============================================================================
ResonanceBKSynthesiser::ResonanceBKSynthesiser (EnvParams& envParams,
                                                 chowdsp::GainDBParameter& gainParam)
    : BKSynthesiser (envParams, gainParam)
{
}

ResonanceBKSynthesiser::~ResonanceBKSynthesiser()
{
    for (auto& wt : workerThreads)
        wt.reset();
}

//==============================================================================
void ResonanceBKSynthesiser::prepareWorkgroup (juce::AudioWorkgroup workgroup,
                                               int samplesPerBlock,
                                               double sampleRate,
                                               int /*numChannels*/)
{
    currentSamplesPerBlock = samplesPerBlock;
    currentNumChannels     = 2;  // voices render stereo only

    for (auto& wt : workerThreads)
        wt.reset();

    for (auto& buf : scratchBuffers)
        buf.setSize (currentNumChannels, currentSamplesPerBlock,
                     false, true, false);

    completionBarrier = ResonanceThreadBarrier::make (ResonanceNumWorkerThreads + 1);

    const WorkerThreadOptions opts { workgroup, completionBarrier, samplesPerBlock, sampleRate };
    for (int i = 0; i < ResonanceNumWorkerThreads; ++i)
        workerThreads[i] = std::make_unique<WorkerThread> (opts);
}

//==============================================================================
void ResonanceBKSynthesiser::renderNextBlock (juce::AudioBuffer<float>& outputAudio,
                                               const juce::MidiBuffer& inputMidi,
                                               int startSample,
                                               int numSamples)
{
    // Fall back to sequential if not yet prepared
    if (completionBarrier == nullptr)
    {
        BKSynthesiser::renderNextBlock (outputAudio, inputMidi, startSample, numSamples);
        return;
    }

    // Hold the lock for the entire block, matching the base class behaviour.
    // MIDI events update voice state; voice rendering happens in parallel below.
    const juce::ScopedLock sl (lock);

    //--------------------------------------------------------------------------
    // Step 1: Render all currently-active voices in parallel.
    // This runs BEFORE MIDI so that when stealing occurs in step 2, existing
    // voices already have envVal > 0 and can be properly graveyard-faded.
    // Voices started in step 2 will render on the next block — one block of
    // latency is acceptable for sympathetic resonance.
    //--------------------------------------------------------------------------

    // Render active graveyard slots on the main thread.
    for (auto* v : graveyardVoices)
        if (v->isVoiceActive())
            v->renderNextBlock (outputAudio, startSample, numSamples);

    const int totalVoices = voices.size();

    // Clear scratch buffers (no allocation)
    for (int w = 0; w < ResonanceNumWorkerThreads; ++w)
        scratchBuffers[w].clear (startSample, numSamples);

    // Distribute voices round-robin across workers
    for (int i = 0; i < totalVoices; ++i)
    {
        const int w = i % ResonanceNumWorkerThreads;
        BKSynthesiserVoice* const vp = voices[i];
        workerThreads[w]->prepareAndQueueVoices (
            juce::Span<BKSynthesiserVoice* const> (&vp, 1),
            &scratchBuffers[w],
            startSample,
            numSamples);
    }

    // Signal workers — one signal per block, cannot accumulate
    for (auto& wt : workerThreads)
        wt->signalStart();

    // Main thread waits at barrier until all workers finish
    completionBarrier->arriveAndWait();

    // Mix scratch buffers into output (voices render stereo only)
    const int channelsToSum = juce::jmin (currentNumChannels, outputAudio.getNumChannels());
    for (int w = 0; w < ResonanceNumWorkerThreads; ++w)
        for (int ch = 0; ch < channelsToSum; ++ch)
            outputAudio.addFrom (ch, startSample, scratchBuffers[w], ch, startSample, numSamples);

    //--------------------------------------------------------------------------
    // Step 2: Process all MIDI events. Voices started here render next block.
    // Because step 1 already rendered existing voices, they now have envVal > 0
    // and will be properly graveyard-faded if stolen during this MIDI pass.
    //--------------------------------------------------------------------------
    for (const auto& meta : inputMidi)
    {
        if (meta.samplePosition >= startSample &&
            meta.samplePosition < startSample + numSamples)
        {
            handleMidiEvent (meta.getMessage());
        }
    }

    //--------------------------------------------------------------------------
    // Step 3: Update someVoicesActive
    //--------------------------------------------------------------------------
    someVoicesActive = false;
    int activeVoiceCount = 0;
    for (auto* v : voices)
    {
        if (v->isVoiceActive())
        {
            someVoicesActive = true;
            ++activeVoiceCount;
        }
    }

    int activeGraveyardCount = 0;
    int graveyardEnvNonZero  = 0;
    for (auto* v : graveyardVoices)
    {
        if (v->isVoiceActive())
        {
            someVoicesActive = true;
            ++activeGraveyardCount;
            if (v->getAmpEnvValue() > 0.0f) ++graveyardEnvNonZero;
        }
    }

    // Immediate diagnostic whenever graveyard count changes
    static int lastGraveyardCount = 0;
    if (activeGraveyardCount != lastGraveyardCount)
    {
        lastGraveyardCount = activeGraveyardCount;
        DBG ("ResonanceBKSynthesiser step3: activeVoices=" + juce::String (activeVoiceCount)
             + " graveyardActive=" + juce::String (activeGraveyardCount)
             + " graveyardEnvNonZero=" + juce::String (graveyardEnvNonZero));
    }

    // Periodic diagnostic: print active voice counts every ~500 blocks
    static int diagBlockCounter = 0;
    if (++diagBlockCounter >= 500)
    {
        diagBlockCounter = 0;
        DBG ("ResonanceBKSynthesiser: activeVoices=" + juce::String (activeVoiceCount)
             + " graveyardActive=" + juce::String (activeGraveyardCount)
             + " totalVoices=" + juce::String (voices.size()));
    }
}
