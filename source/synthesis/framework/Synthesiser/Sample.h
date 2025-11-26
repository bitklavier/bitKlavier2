/*
  ==============================================================================
   This file is part of the JUCE examples.
   Copyright (c) 2020 - Raw Material Software Limited
   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.
   THE SOFTWARE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES,
   WHETHER EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR
   PURPOSE, ARE DISCLAIMED.
  ==============================================================================
*/

#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_core/juce_core.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "common.h"
#include "utils.h"
#include "BKADSR.h"
#include "SFZEG.h"
#include "TuningProcessor.h"
#include "SampleBuffer.h"
#include "SFZSample.h"
/**
 * todo: cleanup Sample.h!
 * @tparam T
 */

//==============================================================================
// Represents the constant parts of an audio sample: its name, sample rate,
// length, and the audio sample data itself.
// Samples might be pretty big, so we'll keep shared_ptrs to them most of the
// time, to reduce duplication and copying.


class SampleBuffer;
// Declaration of the concept “Hashable”, which is satisfied by any type “T”
// such that for values “a” of type “T”, the expression std::hash<T>{}(a)
// compiles and its result is convertible to std::size_t
// ---------------------------------------------
// Trait to detect whether ReaderType supports read()
// ---------------------------------------------
template <typename T, typename = void>
struct IsReaderType : std::false_type {};

template <typename T>
struct IsReaderType<T, std::void_t<
    decltype(std::declval<T>().read(
        (juce::AudioBuffer<float>*)nullptr,
        0, 0, 0, true, true))
    >>
: std::true_type {};
#include "SFZRegion.h"

// concept
template<class T>
concept LoadImmediate = not (std::is_base_of_v<juce::MemoryMappedAudioFormatReader, T> || std::is_base_of_v<juce::BufferingAudioReader, T>);
template<class T>
concept NonLoadImmediate = (std::is_base_of_v<juce::MemoryMappedAudioFormatReader, T> || std::is_base_of_v<juce::BufferingAudioReader, T>);
template<class T>
concept SFZLoadType = std::is_same_v<T, SFZRegion>;
template<typename ReaderType>
class Sample
{
public:
    Sample(ReaderType& source, double maxSampleLengthSecs) :  m_sourceSampleRate(source.sampleRate),
                                                              m_length(juce::jmin(int(source.lengthInSamples), int(maxSampleLengthSecs* m_sourceSampleRate))),
                                                              m_data(juce::jmin(2, int(source.numChannels)), m_length + 4), m_source(source)
    {
        if (m_length == 0)
            throw std::runtime_error("Unable to load sample");

        source.read(&m_data, 0, m_length + 4, 0, true, true);

        //upsample(8);
    }

//    Sample(std::vector<std::vector<float>> soundData, double sr) : m_sourceSampleRate{ sr },
//        m_length((int)soundData.at(0).size()) {
//
//        int numChans = (int) soundData.size();
//        int numSamples = m_length;
//
//        m_temp_data.setSize(numChans, numSamples, false, true, false);
//
//        for (int chan = 0; chan < numChans; chan++) {
//            m_temp_data.copyFrom(chan, 0, soundData.at(chan).data(), m_length);
//        }
//
//        upsample(8);
//    }

    double getSampleRate() const { return m_sourceSampleRate; }
    int getLength() const { return m_length; }

    float getRMS()
    {
        float dBFSLevel = 0.0f;
         for (int i = 0; i < m_data.getNumChannels(); ++i)
         {
             dBFSLevel = m_data.getRMSLevel(i, 0, juce::jmin(int(m_sourceSampleRate*0.4f),m_data.getNumSamples()));
         }
         dBFSLevel *= 1.f/m_data.getNumChannels();
         dBFSLevel = juce::Decibels::gainToDecibels(dBFSLevel);
         return dBFSLevel;
    }

    void setStartSample(int startSample){m_startSample = startSample;}
    void setNumSamps(int numSamps) {m_numSamps = numSamps;}
    const std::tuple<const float*, const float*> getBuffer() const  {
        return std::make_tuple(m_data.getReadPointer(0,m_startSample),
                               m_data.getNumChannels() > 1 ? m_data.getReadPointer(1,m_startSample)
                               : m_data.getReadPointer(0,m_startSample));
    }

private:
    double m_sourceSampleRate;
    int m_length;
    int m_startSample;
    int m_numSamps;
    juce::AudioBuffer<float> m_temp_data;
    juce::AudioBuffer<float> m_data;
    ReaderType& m_source;
    juce::LagrangeInterpolator m_interpolator;

    // Whenever sample data is given to the Sample class, a Lagrange interpolator upsamples it in order
    // to be able to play it back at at different speeds with low aliasing artifacts. Therefore, upsample()
    // must be called in each constructor to Sample().
    void upsample(int upSampleRatio) {

        int numInputSamples = m_temp_data.getNumSamples();
        int numOutputSamples = upSampleRatio * numInputSamples;

        m_data.setSize(2, numOutputSamples, false, true, false);

        for (int outChan = 0; outChan < 2; outChan++) {
            int inChan = m_temp_data.getNumChannels() > 1 ? outChan : 0;
            m_interpolator.process(1./(double)(upSampleRatio), m_temp_data.getReadPointer(inChan), m_data.getWritePointer(outChan), numOutputSamples, numInputSamples, 0);
        }

        m_length *= upSampleRatio;
        m_sourceSampleRate *= upSampleRatio;

        m_temp_data.clear();
    }
};

template <NonLoadImmediate ReaderType>
class Sample<ReaderType>
{
    Sample(ReaderType& source, double maxSampleLengthSecs)
            : m_sourceSampleRate(source.sampleRate),
              m_length(juce::jmin(int(source.lengthInSamples),
                                  int(maxSampleLengthSecs* m_sourceSampleRate))),
              m_temp_data(juce::jmin(2, int(source->numChannels)), m_length + 4), m_source(source)
    {
        if (m_length == 0)
            throw std::runtime_error("Unable to load sample");
        source.mapEntireFile();
        //source->read(&m_data, 0, m_length + 4, 0, true, true);

        //upsample(8);
    }

//    double getSampleRate() const { return m_sourceSampleRate; }
//    int getLength() const { return m_length; }


    void setStartSample(int startSample){m_startSample = startSample;}
    void setNumSamps(int numSamps) {m_numSamps = numSamps;}
    //const juce::AudioBuffer<float>& getBuffer() { return m_data; }
    const juce::AudioBuffer<float>& getBuffer(int startSamp, int numSamples)
    {
        m_source.read(&m_data, startSamp, numSamples + 4, 0, true, true);
        return m_data;
    }

    const std::tuple<const float*, const float*> getBuffer() const  {
        m_source.read(&m_data, m_startSample, m_numSamps + 4, 0, true, true);
        return std::make_tuple(m_data.getReadPointer(0,m_startSample),
                               m_data.getNumChannels() > 1 ? m_data.getReadPointer(1,m_startSample) : m_data.getReadPointer(0,m_startSample));
    }

private:
    int m_startSample;
    int m_numSamps;
    double m_sourceSampleRate;
    int m_length;
    juce::AudioBuffer<float> m_temp_data;
    juce::AudioBuffer<float> m_data;
    ReaderType& m_source;
    juce::LagrangeInterpolator m_interpolator;

//    // Whenever sample data is given to the Sample class, a Lagrange interpolator upsamples it in order
//    // to be able to play it back at at different speeds with low aliasing artifacts. Therefore, upsample()
//    // must be called in each constructor to Sample().
//    void upsample(int upSampleRatio) {
//
//        int numInputSamples = m_temp_data.getNumSamples();
//        int numOutputSamples = upSampleRatio * numInputSamples;
//
//        m_data.setSize(2, numOutputSamples, false, true, false);
//
//        for (int outChan = 0; outChan < 2; outChan++) {
//            int inChan = m_temp_data.getNumChannels() > 1 ? outChan : 0;
//            m_interpolator.process(1./(double)(upSampleRatio), m_temp_data.getReadPointer(inChan), m_data.getWritePointer(outChan), numOutputSamples, numInputSamples, 0);
//        }
//
//        m_length *= upSampleRatio;
//        m_sourceSampleRate *= upSampleRatio;
//
//        m_temp_data.clear();
//    }

};
template <>
class Sample<SFZRegion>
{
public:
    explicit Sample(SFZRegion& sfz)
        : m_sfz(sfz)
    {
        jassert(sfz.sample != nullptr && "SFZRegion::sample is null! Did you call load()?");
        jassert(sfz.sample->buffer != nullptr && "SFZRegion::sample->buffer is null! SFZ loader didn't decode the audio!");

        SampleBuffer* buffer = sfz.sample->buffer;

        m_sourceSampleRate = sfz.sample->sample_rate;
        m_length           = (int) sfz.sample->num_samples;

        const int numChans = buffer->num_channels;
        m_data.setSize(numChans, m_length, false, true, false);

        auto in_read  = buffer->read_sample;  // function pointer
        auto stride   = buffer->stride;       // step between samples
        auto chans    = buffer->num_channels;

        for (int ch = 0; ch < numChans; ++ch)
        {
            auto* out = m_data.getWritePointer(ch);

            uint8_t* inPtr = buffer->channel_start(ch);

            for (int i = 0; i < m_length; ++i)
            {
                out[i] = (float) in_read(inPtr);
                inPtr += stride;
            }
        }

        // Initialize offsets
        m_startSample = 0;
        m_numSamps    = m_length;
        // if (sfz.buffer == nullptr)
        //     throw std::runtime_error("SFZSample buffer is null (did you call load()?)");
        //
        // m_sourceSampleRate = sfz.sample_rate;
        // m_length = (int)sfz.num_samples;
        //
        // const int numChans = sfz.buffer->num_channels();
        // m_data.setSize(numChans, m_length);
        //
        // // Copy raw PCM from SFZ loader's SampleBuffer
        // for (int ch = 0; ch < numChans; ++ch)
        //     m_data.copyFrom(ch, 0, sfz.buffer->getChannelPointer(ch), m_length);
    }

    double getSampleRate() const { return m_sourceSampleRate; }
    int getLength() const { return m_length; }

    void setStartSample(int start) { m_startSample = juce::jlimit(0, m_length, start); }
    void setNumSamps(int num)      { m_numSamps   = juce::jlimit(0, m_length, num); }

    std::tuple<const float*, const float*> getBuffer() const
    {
        const float* left  = m_data.getReadPointer(0, m_startSample);
        const float* right = (m_data.getNumChannels() > 1)
                               ? m_data.getReadPointer(1, m_startSample)
                               : left;

        return { left, right };
    }

    float getRMS() const
    {
        const int win = juce::jmin((int)(m_sourceSampleRate * 0.4), m_data.getNumSamples());

        float sum = 0.f;
        for (int c = 0; c < m_data.getNumChannels(); ++c)
            sum += m_data.getRMSLevel(c, 0, win);

        sum /= (float)m_data.getNumChannels();
        return juce::Decibels::gainToDecibels(sum);
    }
    SFZRegion& getSourceRegion () {
        return m_sfz;
    }

private:
    SFZRegion& m_sfz;

    double m_sourceSampleRate = 44100.0;
    int m_length = 0;

    int m_startSample = 0;
    int m_numSamps   = 0;

    juce::AudioBuffer<float> m_data;
};

enum class SoundSampleType {
    Unknown,
    WAV,
    MMap,
    Buffered,
    SFZ
};

class BKSamplerSoundBase;
class  BKSynthesiserSound    : public juce::ReferenceCountedObject
{
protected:
    //==============================================================================
    BKSynthesiserSound(){};

public:
    /** Destructor. */
    ~BKSynthesiserSound() override{};

    //==============================================================================
    /** Returns true if this sound should be played when a given midi note is pressed.

        The Synthesiser will use this information when deciding which sounds to trigger
        for a given note.
    */
    virtual bool appliesToNote (int midiNoteNumber) = 0;

    /** Returns true if the sound should be triggered by midi events on a given channel.

        The Synthesiser will use this information when deciding which sounds to trigger
        for a given note.
    */
    virtual bool appliesToChannel (int midiChannel) = 0;
    virtual bool appliesToVelocity (int midiChannel) = 0;

    /** The class is reference-counted, so this is a handy pointer class for it. */
    using Ptr = juce::ReferenceCountedObjectPtr<BKSynthesiserSound>;

    float dBFSLevel; // dBFS value of this velocity layer
    float dBFSBelow; // dBFS value of velocity layer below this layer
    virtual SoundSampleType getSoundSampleType() const {return SoundSampleType::Unknown;};
    virtual BKSamplerSoundBase* getSamplerSoundBase() noexcept { return nullptr; }

private:
    JUCE_LEAK_DETECTOR (BKSynthesiserSound)
};

class BKSamplerSoundBase : public BKSynthesiserSound
{
public:
    virtual ~BKSamplerSoundBase() = default;

    // Returning `this` allows a safe static_cast later
    virtual BKSamplerSoundBase* getSamplerSoundBase() noexcept override { return this; }
};

template<typename T>
class BKSamplerSound :  public BKSamplerSoundBase
{
public:
    /*
     * for regular bK-style sample libraries
     */
    BKSamplerSound( const juce::String& soundName,
                    std::shared_ptr<Sample<T>> samp,
                    const juce::BigInteger& midiNotes,
                    int rootMidiNote,
                    int transpose,
                    const juce::BigInteger& midiVelocities,
                    int numLayers,
                    //int layerId,
                    float dBFSBelo
                    ) requires (!std::is_same_v<T, SFZRegion>) :

                    numLayers(numLayers), // i don't think we need this argument anymore
                    //layerId(layerId),
                    rootMidiNote(rootMidiNote),
                    transpose(transpose),
                    midiNotes(midiNotes),
                    midiVelocities(midiVelocities),
                    sample(std::move(samp))
    {
        dBFSBelow = dBFSBelo;
        setCentreFrequencyInHz(mtof(rootMidiNote));
        dBFSLevel = sample->getRMS();
    }

    /*
     * for SoundFont libraries
     */
    BKSamplerSound(const juce::String& soundName, std::shared_ptr<Sample<T>> samp)
           requires (std::is_same_v<T, SFZRegion>) : sample(std::move(samp))
    {
        // very important: you expose region via Sample<SFZRegion>
        SFZRegion& region = sample->getSourceRegion();

        // ---------------------------
        // KEY & VELOCITY RANGES
        // ---------------------------
        midiNotes.clear();
        midiVelocities.clear();
        midiNotes.setRange(region.lokey, region.hikey - region.lokey + 1, true);
        midiVelocities.setRange(region.lovel, region.hivel - region.lovel + 1, true);

        // ---------------------------
        // PITCH & ROOT NOTE
        // ---------------------------
        rootMidiNote = region.pitch_keycenter;
        transpose    = region.transpose;

        // ---------------------------
        // LOOPING INFORMATION
        // ---------------------------
        //loopMode = convertLoopMode(region.loop_mode);

        // if (region.loop_mode != SFZRegion::no_loop)
        //     loopPoints = juce::Range<double>(
        //         (double)region.loop_start,
        //         (double)region.loop_end
        //     );

        // ---------------------------
        // FILTER / AMP PARAMETERS
        // ---------------------------
        params.attack  = region.ampeg.attack;
        params.decay   = region.ampeg.decay;
        params.sustain = region.ampeg.sustain;
        params.release = region.ampeg.release;

        // params.pan     = region.pan;
        // params.volume  = region.volume;

        // ---------------------------
        // Calculate RMS
        // ---------------------------
        /*
         * not applicable for now for SoundFonts
         */
        //dBFSLevel = sample->getRMS();
        //dBFSBelow = region.volume;   // or sfz "volume" mapping

        // ---------------------------
        // CENTER FREQUENCY
        // ---------------------------
        setCentreFrequencyInHz(mtof(rootMidiNote));
    }

    //==============================================================================
    bool appliesToNote (int midiNoteNumber)  {
        return midiNotes[midiNoteNumber];
    }

    bool appliesToChannel (int midiNoteNumber)  {
        return true;
    }

    bool appliesToVelocity (int midiNoteVelocity) {
        return midiVelocities[midiNoteVelocity];
    }

    void setSample (std::unique_ptr<Sample<T>> value)
    {
        sample = std::move (value);
        setLoopPointsInSeconds (loopPoints);
    }

    Sample<T>* getSample() const
    {
        return sample.get();
    }

    void setLoopPointsInSeconds (juce::Range<double> value)
    {
        loopPoints = sample == nullptr ? value : juce::Range<double> (0, sample->getLength() / sample->getSampleRate()).constrainRange (value);
    }

    juce::Range<double> getLoopPointsInSeconds() const
    {
        return loopPoints;
    }

    void setCentreFrequencyInHz (double centre)
    {
        centreFrequencyInHz = centre;
    }

    double getCentreFrequencyInHz() const
    {
        return centreFrequencyInHz;
    }

    void setLoopMode (LoopMode type)
    {
        loopMode = type;
    }

    LoopMode getLoopMode() const
    {
        return loopMode;
    }

    // find the bounding velocities for this sound
    int minVelocity (void) { return midiVelocities.findNextSetBit(0); }
    int maxVelocity (void) { return midiVelocities.findNextClearBit(midiVelocities.findNextSetBit(0)); }

    // use the velocity to set a gain multiplier
    //  - if the velocity is at the top velocity range for this layer, then this will return 1.
    //  - if the velocity is at the bottom velocity range for this layer, it will return a value that
    //      should result in a dBFS equivalent to the max loudness of the layer below (dBFSBelow)
    float getGainMultiplierFromVelocity(float velocity) // velocity [0, 127]
    {
        //DBG("layer dB size = " + juce::String(dBFSLevel - dBFSBelow));
        float dbOffset = (dBFSLevel - dBFSBelow) * (velocity - maxVelocity()) / (maxVelocity() - minVelocity());
        return juce::Decibels::decibelsToGain(dbOffset);
    }

    float getGainMultiplierFromVelocity(float velocity, float dbRange) // velocity [0, 127]
    {
        float dbOffset = dbRange * (velocity - 128) / 128.f;
        return juce::Decibels::decibelsToGain(dbOffset);
    }

    void setEnvelopeParameters (BKADSR::Parameters parametersToUse)    { params = parametersToUse; }
    /** The class is reference-counted, so this is a handy pointer class for it. */
    //typedef juce::ReferenceCountedObjectPtr<BKSamplerSound<T>> Ptr;

    int numLayers; // don't think we need this...
    //int layerId;
    int rootMidiNote;
    int transpose;
    SoundSampleType getSoundSampleType() const override {
        return sampleType;
    }

private:
    std::shared_ptr<Sample<T>> sample;
    double centreFrequencyInHz { 440.0 };
    juce::Range<double> loopPoints;
    LoopMode loopMode { LoopMode::none };
    juce::BigInteger midiNotes;
    juce::BigInteger midiVelocities;

    BKADSR::Parameters params;
    static constexpr SoundSampleType sampleType =
        std::is_same_v<T, SFZRegion> ? SoundSampleType::SFZ :
        std::is_base_of_v<juce::MemoryMappedAudioFormatReader, T> ? SoundSampleType::MMap :
        std::is_base_of_v<juce::BufferingAudioReader, T> ? SoundSampleType::Buffered :
        SoundSampleType::WAV;
};


class BKSynthesiser;

/**
    Represents a voice that a juce::Synthesiser can use to play a juce::SynthesiserSound.

    A voice plays a single sound at a time, and a synthesiser holds an array of
    voices so that it can play polyphonically.

    @see juce::Synthesiser, juce::SynthesiserSound

    @tags{Audio}
*/
class BKSynthesiserVoice
{
public:
    //==============================================================================
    /** Creates a voice. */
    BKSynthesiserVoice();

    /** Destructor. */
    virtual ~BKSynthesiserVoice();

    //==============================================================================
    /** Returns the midi note that this voice is currently playing.
        Returns a value less than 0 if no note is playing.
    */
    int getCurrentlyPlayingNote() const noexcept                        { return currentlyPlayingNote; }

    /** Returns the sound that this voice is currently playing.
        Returns nullptr if it's not playing.
    */
     BKSynthesiserSound::Ptr getCurrentlyPlayingSound() const noexcept     { return currentlyPlayingSound; }

    /** Must return true if this voice object is capable of playing the given sound.

        If there are different classes of sound, and different classes of voice, a voice can
        choose which ones it wants to take on.

        A typical implementation of this method may just return true if there's only one type
        of voice and sound, or it might check the type of the sound object passed-in and
        see if it's one that it understands.
    */
    virtual bool canPlaySound (BKSynthesiserSound*)  {return true;};

    /** Called to start a new note.
        This will be called during the rendering callback, so must be fast and thread-safe.
    */

    /** Called to stop a note.

        This will be called during the rendering callback, so must be fast and thread-safe.

        The velocity indicates how quickly the note was released - 0 is slowly, 1 is quickly.

        If allowTailOff is false or the voice doesn't want to tail-off, then it must stop all
        sound immediately, and must call clearCurrentNote() to reset the state of this voice
        and allow the mainSynth to reassign it another sound.

        If allowTailOff is true and the voice decides to do a tail-off, then it's allowed to
        begin fading out its sound, and it can stop playing until it's finished. As soon as it
        finishes playing (during the rendering callback), it must make sure that it calls
        clearCurrentNote().
    */
    virtual void stopNote (float velocity, bool allowTailOff) = 0;

    /** Returns true if this voice is currently busy playing a sound.
        By default this just checks the getCurrentlyPlayingNote() value, but can
        be overridden for more advanced checking.
    */
    virtual bool isVoiceActive() const;

    /** Called to let the voice know that the pitch wheel has been moved.
        This will be called during the rendering callback, so must be fast and thread-safe.
    */
    virtual void pitchWheelMoved (int newPitchWheelValue) {}

    /** Called to let the voice know that a midi controller has been moved.
        This will be called during the rendering callback, so must be fast and thread-safe.
    */
    virtual void controllerMoved (int controllerNumber, int newControllerValue) {}

    /** Called to let the voice know that the aftertouch has changed.
        This will be called during the rendering callback, so must be fast and thread-safe.
    */
    virtual void aftertouchChanged (int newAftertouchValue);

    /** Called to let the voice know that the channel pressure has changed.
        This will be called during the rendering callback, so must be fast and thread-safe.
    */
    virtual void channelPressureChanged (int newChannelPressureValue);

    //==============================================================================
    /** Renders the next block of data for this voice.

        The output audio data must be added to the current contents of the buffer provided.
        Only the region of the buffer between startSample and (startSample + numSamples)
        should be altered by this method.

        If the voice is currently silent, it should just return without doing anything.

        If the sound that the voice is playing finishes during the course of this rendered
        block, it must call clearCurrentNote(), to tell the synthesiser that it has finished.

        The size of the blocks that are rendered can change each time it is called, and may
        involve rendering as little as 1 sample at a time. In between rendering callbacks,
        the voice's methods will be called to tell it about note and controller events.
    */
    virtual void renderNextBlock (juce::AudioBuffer<float>& outputBuffer,
                                  int startSample,
                                  int numSamples) = 0;



    /** Changes the voice's reference sample rate.

        The rate is set so that subclasses know the output rate and can set their pitch
        accordingly.

        This method is called by the mainSynth, and subclasses can access the current rate with
        the currentSampleRate member.
    */
    virtual void setCurrentPlaybackSampleRate (double newRate);

    /** Returns true if the voice is currently playing a sound which is mapped to the given
        midi channel.

        If it's not currently playing, this will return false.
    */
    virtual bool isPlayingChannel (int midiChannel) const;

    /** Returns the current target sample rate at which rendering is being done.
        Subclasses may need to know this so that they can pitch things correctly.
    */
    double getSampleRate() const noexcept                       { return currentSampleRate; }

    /** Returns true if the key that triggered this voice is still held down.
        Note that the voice may still be playing after the key was released (e.g because the
        sostenuto pedal is down).
    */
    bool isKeyDown() const noexcept                             { return keyIsDown; }

    /** Allows you to modify the flag indicating that the key that triggered this voice is still held down.
        @see isKeyDown
    */
    void setKeyDown (bool isNowDown) noexcept                   { keyIsDown = isNowDown; }

    /** Returns true if the sustain pedal is currently active for this voice. */
    bool isSustainPedalDown() const noexcept                    { return sustainPedalDown; }

    /** Modifies the sustain pedal flag. */
    void setSustainPedalDown (bool isNowDown) noexcept          { sustainPedalDown = isNowDown; }

    /** Returns true if the sostenuto pedal is currently active for this voice. */
    bool isSostenutoPedalDown() const noexcept                  { return sostenutoPedalDown; }

    /** Modifies the sostenuto pedal flag. */
    void setSostenutoPedalDown (bool isNowDown) noexcept        { sostenutoPedalDown = isNowDown; }

    /** Returns true if a voice is sounding in its release phase **/
    bool isPlayingButReleased() const noexcept
    {
        return isVoiceActive() && ! (isKeyDown() || isSostenutoPedalDown() || isSustainPedalDown());
    }

    virtual void startNote (int midiNoteNumber,
        float velocity,
        float transposition,
        bool tune_transpositions,
        BKSynthesiserSound * _sound,
        int currentPitchWheelPosition,
        float startTimeMS=0.f,
        Direction startDirection= Direction::forward) = 0;

    /** Returns true if this voice started playing its current note before the other voice did. */
    bool wasStartedBefore (const BKSynthesiserVoice& other) const noexcept;
    double currentSampleRate = 44100.0;

    juce::uint32 noteOnTime = 0; int currentlyPlayingNote = -1, currentPlayingMidiChannel = 0;
    BKSynthesiserSound::Ptr currentlyPlayingSound;

    bool keyIsDown = false, sustainPedalDown = false, sostenutoPedalDown = false;
    void updateAmpEnv(BKADSR::Parameters &parameters) {
        ampEnv.setParameters(parameters);
        // todo: update mod amount
    }

    void copyAmpEnv(BKADSR::Parameters parameters)
    {
        updateAmpEnv(parameters);
    }

    void setTargetSustainTime(float sustainTimeMS)
    {
        targetSustainTime_samples = sustainTimeMS * getSampleRate() * .001;
    }

    void setGain(float g)
    {
        voiceGain = g;
    }

    /*
    * for situations where we specify a sustain time at noteOn, we want to ignore noteOff messages
    */
    bool ignoreNoteOff = false;

protected:
    int64_t targetSustainTime_samples = -1;
    float voiceGain {1.};

    BKADSR ampEnv;

    void setTuning(TuningState* attachedTuning)
    {
        tuning = attachedTuning;
    }
    /** Resets the state of this voice after a sound has finished playing.

        The subclass must call this when it finishes playing a note and becomes available
        to play new ones.

        It must either call it in the stopNote() method, or if the voice is tailing off,
        then it should call it later during the renderNextBlock method, as soon as it
        finishes its tail-off.

        It can also be called at any time during the render callback if the sound happens
        to have finished, e.g. if it's playing a sample and the sample finishes.
    */
    void clearCurrentNote();

protected:
    TuningState* tuning = nullptr;

private:

    //==============================================================================
    friend class BKSynthesiser;


    juce::AudioBuffer<float> tempBuffer;

    JUCE_LEAK_DETECTOR (BKSynthesiserVoice)
};


//==============================================================================
/*
  ==============================================================================
   This file is part of the JUCE examples.
   Copyright (c) 2020 - Raw Material Software Limited
   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.
   THE SOFTWARE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES,
   WHETHER EXPRESSED OR IMPLIED, INCL]UDING MERCHANTABILITY AND FITNESS FOR
   PURPOSE, ARE DISCLAIMED.
  ==============================================================================
*/
template<typename T>

class BKSamplerVoice : public BKSynthesiserVoice
{
public:
    explicit BKSamplerVoice()
    {
        ampEnv.setParameters(BKADSR::Parameters(0.005, 0.5,1.0,0.1));
        m_Buffer.setSize(2, 1, false, true, false);
        // call the smooth.reset() here if we want smoothing for various smoothingFloat vars
    }

    void setCurrentPlaybackSampleRate(double newRate) override {

        if (newRate <= 0) {
            return;
        }

        ampEnv.setSampleRate(newRate);
        ampEnv.setParameters(ampEnv.getParameters());
    }

    void startNote (int midiNoteNumber,
         float velocity,
         float transposition,
         bool tune_transpositions,
         BKSynthesiserSound * _sound,
         int currentPitchWheelPosition,
         float startTimeMS = 0.f,
         Direction startDirection = Direction::forward) {
        auto* sampler = static_cast<BKSamplerSound<T>*>(_sound->getSamplerSoundBase());
        jassert(sampler != nullptr);
        myStartNote(midiNoteNumber,velocity,transposition,tune_transpositions,sampler,
            currentPitchWheelPosition,startTimeMS,startDirection);
    }

    void myStartNote (int midiNoteNumber,
        float velocity,
        float transposition,
        bool tune_transpositions,
        BKSamplerSound<T>* _sound,
        int currentPitchWheelPosition,
        float startTimeMS = 0.f,
        Direction startDirection = Direction::forward)
    {
        samplerSound = _sound;
        currentlyPlayingSound = _sound;
        currentlyPlayingNote = midiNoteNumber;
        currentTransposition = transposition;
        tuneTranspositions = tune_transpositions;

        /* this will adjust the loudness of this layer according to velocity, based on the
         *  - dB difference between this layer and the layer below
         */
        level.setTargetValue(samplerSound->getGainMultiplierFromVelocity(velocity) * voiceGain); // need gain setting for each synth

        /* set the sample increment, based on the target frequency for this note
         *  - we will update this every block for spring and regular tunings, but not for tuningType tunings
         */
        sampleIncrement.setTargetValue ((getTargetFrequency() / samplerSound->getCentreFrequencyInHz()) * samplerSound->getSample()->getSampleRate() / this->currentSampleRate);

        auto loopPoints = samplerSound->getLoopPointsInSeconds();
        loopBegin.setTargetValue(loopPoints.getStart() * samplerSound->getSample()->getSampleRate());
        loopEnd.setTargetValue(loopPoints.getEnd() * samplerSound->getSample()->getSampleRate());

        currentDirection = startDirection;
        currentSamplePos = startTimeMS * getSampleRate() * .001;

        /*
         * need to account for sampleIncrement when setting start time
         * - backwards playing samples with sampleIncrement > 1 will reach start of sample earlier than those with lower sampleIncrement, for instance
         */
        if (currentDirection == Direction::backward)
            currentSamplePos *= sampleIncrement.getTargetValue();

        tailOff = 0.0;

        currentSustainTime_samples = 0;
        ampEnv.noteOn();
        if constexpr (std::is_same_v<T, SFZRegion>)
        {
            auto& region = samplerSound->getSample()->getSourceRegion();
            ampeg.start_note(&region.ampeg, velocity, samplerSound->getSample()->getSampleRate(), &region.ampeg_veltrack);

            // need to override dBFS approach with SoundFonts
            level.setTargetValue(samplerSound->getGainMultiplierFromVelocity(velocity, 24.f) * voiceGain);

            // -----------------------
            // Offset / End
            // -----------------------
            currentSamplePos = region.offset;
            //sample_end = region.sample->num_samples;

            //if (region.end > 0 && region.end < sample_end)
                //sample_end = region.end + 1;

            // -----------------------
            // Looping
            // -----------------------
            loop_start = 0;
            loop_end   = 0;

            SFZRegion::LoopMode loop_mode = region.loop_mode;

            if (loop_mode == SFZRegion::sample_loop)
            {
                if (region.sample->loop_start < region.sample->loop_end)
                    loop_mode = SFZRegion::loop_continuous;
                else
                    loop_mode = SFZRegion::no_loop;
            }

            if (loop_mode != SFZRegion::no_loop &&
                loop_mode != SFZRegion::one_shot)
            {
                if (region.loop_start < region.loop_end)
                {
                    loop_start = region.loop_start;
                    loop_end   = region.loop_end;
                }
                else
                {
                    loop_start = region.sample->loop_start;
                    loop_end   = region.sample->loop_end;
                }
            }

            num_loops = 0;
        }
    }

    double getTargetFrequency()
    {
        // if there is no Tuning prep connected, just return the equal tempered frequency
        if (tuning == nullptr) return mtof ((double) currentlyPlayingNote + currentTransposition);

        // otherwise, get the target frequency from the attached Tuning pre
        return tuning->getTargetFrequency(currentlyPlayingNote, currentTransposition, tuneTranspositions);
    }

    void setDirection(Direction newdir)
    {
        currentDirection = newdir;
    }

    virtual void stopNote (float velocity, bool allowTailOff)
    {
        if (allowTailOff)
        {
            ampEnv.noteOff();
            tailOff = 1.;
        }
        else
            stopNote();

        /*
         * for some reason this was causing clicking when replaying a note with a long release time over and over
        if (allowTailOff && juce::approximatelyEqual (tailOff, 0.0))
            tailOff = 1.0;
        else
            stopNote();
            */
    }

    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                         int startSample,
                         int numSamples) override
    {
        if (isVoiceActive())
            render(outputBuffer, startSample, numSamples);
    }

    double getCurrentSamplePosition() const
    {
        return currentSamplePos;
    }

    void updateParams() {
        //updateAmpEnv();
    }

private:
    template <typename Element>
    void render(juce::AudioBuffer<Element>& outputBuffer, int startSample, int numSamples)
    {
        jassert(samplerSound->getSample() != nullptr);
        //updateParams(); // NB: important line (except this function doesn't do anything right now!)

        /*
         * don't change tuning after noteOn for adaptive tunings
         * do need to for spring, and probably for regular notes it might be handy
         */
        if(tuning != nullptr ) {
            if((tuning->getTuningType() == Static) || (tuning->getTuningType() == Spring_Tuning)) {
                sampleIncrement.setTargetValue ((getTargetFrequency() / samplerSound->getCentreFrequencyInHz()) * samplerSound->getSample()->getSampleRate() / this->currentSampleRate);
            }
            // skip for adaptive tunings
        }
        // otherwise just return ET
        else sampleIncrement.setTargetValue (mtof ((double) currentlyPlayingNote + currentTransposition) / samplerSound->getCentreFrequencyInHz() * samplerSound->getSample()->getSampleRate() / this->currentSampleRate);

        auto loopPoints = samplerSound->getLoopPointsInSeconds();
        loopBegin.setTargetValue(loopPoints.getStart() * samplerSound->getSample()->getSampleRate());
        loopEnd.setTargetValue(loopPoints.getEnd() * samplerSound->getSample()->getSampleRate());

        auto [inL, inR] = samplerSound->getSample()->getBuffer();

        auto outL = outputBuffer.getWritePointer(0, startSample);
        if (outL == nullptr)
            return;
        auto outR = outputBuffer.getNumChannels() > 1 ? outputBuffer.getWritePointer(1, startSample)
                                                      : nullptr;

        size_t writePos = 0;
        while (--numSamples >= 0 && renderNextSample(inL, inR, outL, outR, writePos))
            writePos += 1;
    }

    template <typename Element>
    bool renderNextSample(const float* inL,
                          const float* inR,
                          Element* outL,
                          Element* outR,
                          size_t writePos)
    {
        auto currentLoopBegin = loopBegin.getNextValue();
        auto currentLoopEnd = loopEnd.getNextValue();
        auto currentIncrement = sampleIncrement.getNextValue();

        /*
         * this should handle the case where we want to play backwards for a time longer than the sample length
         *  - will simply return 0's and decrement the currentSamplePos (in getNextState)
         */
        if(currentDirection == Direction::backward && currentSamplePos > samplerSound->getSample()->getLength())
        {
            std::tie(currentSamplePos, currentDirection) = getNextState(currentIncrement, currentLoopBegin, currentLoopEnd);

            if (outR != nullptr)
            {
                outL[writePos] += 0.f;
                outR[writePos] += 0.f;
            }
            else
            {
                outL[writePos] += 0.f;
            }

            return true;
        }

        float ampEnvLast = ampEnv.getNextSample();
        if (ampEnv.isActive() && isTailingOff())
        {
            if (ampEnvLast < 0.001)
            {
                stopNote();
                return false;
            }
        }

        auto pos = (int)currentSamplePos;
        auto nextPos = pos + 1;
        auto alpha = (Element)(currentSamplePos - pos);
        auto invAlpha = 1.0f - alpha;
        float l,r, ampeg_gain,ampeg_slope;
        long samples_until_next_amp_segment = ampeg.samples_until_next_segment;
        bool amp_segment_is_exponential = ampeg.segment_is_exponential;
        if constexpr (std::is_same_v<T, SFZRegion>) {
            auto& region = samplerSound->getSample()->getSourceRegion();
            SampleBuffer* in_buffer = region.sample->buffer;
            auto in_l = in_buffer->channel_start(0);
            auto in_r = in_buffer->num_channels > 1 ? in_buffer->channel_start(1) : nullptr;
            auto in_read = in_buffer->read_sample;
            if (loop_start < loop_end && nextPos > loop_end)
                nextPos = loop_start;
            // Simple linear interpolation.
            auto stride = in_buffer->stride;
            l =
                in_read(in_l + pos * stride) * invAlpha + in_read(in_l + nextPos * stride) * alpha;
            r =
                in_r ?
                (in_read(in_r + pos * stride) * invAlpha + in_read(in_r + nextPos * stride) * alpha) :
                l;
             ampeg_gain = ampeg.level;
            ampeg_slope = ampeg.slope;
            float gain_left = ampeg_gain;
            float gain_right = ampeg_gain;
            l *= gain_left;
            r *= gain_right;
        } else {
            // Very simple linear interpolation here because the Sampler class should have already upsampled.
            l = static_cast<Element> ((inL[pos] * invAlpha + inL[nextPos] * alpha));
            r = static_cast<Element> ((inR != nullptr) ? (inR[pos] * invAlpha + inR[nextPos] * alpha)
                                                            : l);
        }
        m_Buffer.setSample(0, 0, l);
        m_Buffer.setSample(1, 0, r);

        // apply velocity-> gain
        m_Buffer.applyGain(level.getTargetValue());

        // apply amplitude
        if (ampEnv.isActive()) {
            m_Buffer.applyGain(ampEnvLast);
        }

        if (outR != nullptr)
        {
            outL[writePos] += m_Buffer.getSample(0, 0);
            outR[writePos] += m_Buffer.getSample(1, 0);
        }
        else
        {
            outL[writePos] += (m_Buffer.getSample(0, 0) + m_Buffer.getSample(1, 0)) * 0.5f;
        }

        std::tie(currentSamplePos, currentDirection) = getNextState(currentIncrement, currentLoopBegin, currentLoopEnd);
        if constexpr (std::is_same_v<T, SFZRegion>) {
            if (loop_start < loop_end && currentSamplePos > loop_end) {
                currentSamplePos = loop_start;
                num_loops += 1;
            }
            // Update EG.
            if (amp_segment_is_exponential)
                ampeg_gain *= ampeg_slope;
            else
                ampeg_gain += ampeg_slope;
            if (--samples_until_next_amp_segment < 0) {
                ampeg.level = ampeg_gain;
                ampeg.next_segment();
                ampeg_gain = ampeg.level;
                ampeg_slope = ampeg.slope;
                samples_until_next_amp_segment = ampeg.samples_until_next_segment;
                amp_segment_is_exponential = ampeg.segment_is_exponential;
            }
            ampeg.level = ampeg_gain;
            ampeg.samples_until_next_segment = samples_until_next_amp_segment;
        }
        if (currentSamplePos > samplerSound->getSample()->getLength())
        {
            stopNote();
            return false;
        }

        /*
         * if we have a target sustain time set at noteOff, check the duration and stop note as needed
         */
        currentSustainTime_samples++;
        if (targetSustainTime_samples > 0)
        {
            if (currentSustainTime_samples > targetSustainTime_samples)
            {
                stopNote(64, true);
            }
        }

        return true;
    }

    bool isTailingOff() const
    {
        return ! juce::approximatelyEqual (tailOff, 0.0);
    }

    void stopNote()
    {
        // todo: are these necessary?
        ampEnv.reset();

        clearCurrentNote();
        currentSamplePos = 0.0;
    }

    [[nodiscard]] std::tuple<double, Direction> getNextState
        (
        double inc,
        double begin,
        double end
        ) const
    {
        auto nextPitchRatio = inc;
        auto nextSamplePos = currentSamplePos;
        auto nextDirection = currentDirection;

        // Move the current sample pos in the correct direction
        switch (currentDirection)
        {
            case Direction::forward:
                nextSamplePos += nextPitchRatio;
                break;

            case Direction::backward:
                nextSamplePos -= nextPitchRatio;
                break;

            default:
                break;
        }

        // Update current sample position, taking loop mode into account
        // If the loop mode was changed while we were travelling backwards, deal
        // with it gracefully.
        if (nextDirection == Direction::backward && nextSamplePos < begin)
        {
            nextSamplePos = begin;
            if (samplerSound->getLoopMode() == LoopMode::pingpong) nextDirection = Direction::forward;

            return std::tuple<double, Direction>(nextSamplePos, nextDirection);
        }

        if (samplerSound->getLoopMode() == LoopMode::none)
            return std::tuple<double, Direction>(nextSamplePos, nextDirection);

        if (nextDirection == Direction::forward && end < nextSamplePos && !isTailingOff())
        {
            if (samplerSound->getLoopMode() == LoopMode::forward)
                nextSamplePos = begin;
            else if (samplerSound->getLoopMode() == LoopMode::pingpong)
            {
                nextSamplePos = end;
                nextDirection = Direction::backward;
            }
        }
        return std::tuple<double, Direction>(nextSamplePos, nextDirection);
    }

    BKSamplerSound<T>* samplerSound = nullptr;

    double currentTransposition; // comes from Transposition sliders in Direct/Nostalgic/Synchronic
    bool tuneTranspositions = false; // if this is true, then Transposition slider values will be tuned using the current tuning system (in TuningState)

    juce::SmoothedValue<double> level { 0 };
    juce::SmoothedValue<double> loopBegin;
    juce::SmoothedValue<double> loopEnd;
    juce::SmoothedValue<double> sampleIncrement { 0. }; // how far to move through sample, to effect transpositions
    float loop_end;
    float loop_start;
    float num_loops;
    SFZEG ampeg;
    double currentSamplePos { 0 };
    double tailOff { 0 };
    Direction currentDirection{ Direction::forward };
    juce::uint64 currentSustainTime_samples = 0;

    juce::AudioBuffer<float> m_Buffer;
};

