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
#include "TuningProcessor.h"

//==============================================================================
// Represents the constant parts of an audio sample: its name, sample rate,
// length, and the audio sample data itself.
// Samples might be pretty big, so we'll keep shared_ptrs to them most of the
// time, to reduce duplication and copying.


// Declaration of the concept “Hashable”, which is satisfied by any type “T”
// such that for values “a” of type “T”, the expression std::hash<T>{}(a)
// compiles and its result is convertible to std::size_t
// concept
template<class T>
concept LoadImmediate = not (std::is_base_of_v<juce::MemoryMappedAudioFormatReader, T> || std::is_base_of_v<juce::BufferingAudioReader, T>);
template<class T>
concept NonLoadImmediate = (std::is_base_of_v<juce::MemoryMappedAudioFormatReader, T> || std::is_base_of_v<juce::BufferingAudioReader, T>);

template<typename ReaderType>
class Sample
{
public:
    Sample(ReaderType& source, double maxSampleLengthSecs)
        : m_sourceSampleRate(source.sampleRate),
        m_length(juce::jmin(int(source.lengthInSamples),
            int(maxSampleLengthSecs* m_sourceSampleRate))),
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
//    template<T>
//    const juce::AudioBuffer<float>& getBuffer(int startSamp, int numSamples)
//    {
//        source.read(&m_data, startSamp, numSamples + 4, 0, true, true);
//    }


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
enum class LoopMode
{
    none,
    forward,
    pingpong
};
template<typename T>
class BKSamplerSound :  public juce::SynthesiserSound
{
public:


    BKSamplerSound( const juce::String& soundName,
                    std::shared_ptr<Sample<T>> samp,
                    const juce::BigInteger& midiNotes,
                    int rootMidiNote,
                    int transpose,
                    const juce::BigInteger& midiVelocities,
                    int numLayers,
                    //int layerId,
                    float dBFSBelow
                    ) :
                    dBFSBelow(dBFSBelow),
                    numLayers(numLayers), // i don't think we need this argument anymore
                    //layerId(layerId),
                    rootMidiNote(rootMidiNote),
                    transpose(transpose),
                    midiNotes(midiNotes),
                    midiVelocities(midiVelocities),
                    sample(std::move(samp))
    {
//        // Print the class name and action
//        DBG("Create BKSamplerSound");

        setCentreFrequencyInHz(mtof(rootMidiNote));
        dBFSLevel = sample->getRMS();

        // Print the highest bit and bit count for midiNotes
//        int midiNotesHighestBit = midiNotes.getHighestBit();
//        int midiNotesBitCount = midiNotes.countNumberOfSetBits();

        // Print the highest bit and bit count for midiVelocities
        //int midiVelocitiesHighestBit = midiVelocities.getHighestBit();
        //int midiVelocitiesBitCount = midiVelocities.countNumberOfSetBits();


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
        loopPoints = sample == nullptr ? value
                                       : juce::Range<double> (0, sample->getLength() / sample->getSampleRate())
                             .constrainRange (value);
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

    void setEnvelopeParameters (BKADSR::Parameters parametersToUse)    { params = parametersToUse; }
    /** The class is reference-counted, so this is a handy pointer class for it. */
    typedef juce::ReferenceCountedObjectPtr<BKSamplerSound<T>> Ptr;

    float dBFSLevel; // dBFS value of this velocity layer
    float dBFSBelow; // dBFS value of velocity layer below this layer
    int numLayers; // don't think we need this...
    //int layerId;
    int rootMidiNote;
    int transpose;
private:
    std::shared_ptr<Sample<T>> sample;
    double centreFrequencyInHz { 440.0 };
    juce::Range<double> loopPoints;
    LoopMode loopMode { LoopMode::none };
    juce::BigInteger midiNotes;
    juce::BigInteger midiVelocities;

    BKADSR::Parameters params;

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
    juce::SynthesiserSound::Ptr getCurrentlyPlayingSound() const noexcept     { return currentlyPlayingSound; }

    /** Must return true if this voice object is capable of playing the given sound.

        If there are different classes of sound, and different classes of voice, a voice can
        choose which ones it wants to take on.

        A typical implementation of this method may just return true if there's only one type
        of voice and sound, or it might check the type of the sound object passed-in and
        see if it's one that it understands.
    */
    virtual bool canPlaySound (juce::SynthesiserSound*)  {return true;};

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

    /** Returns true if this voice started playing its current note before the other voice did. */
    bool wasStartedBefore (const BKSynthesiserVoice& other) const noexcept;
    double currentSampleRate = 44100.0;

    juce::uint32 noteOnTime = 0; int currentlyPlayingNote = -1, currentPlayingMidiChannel = 0;
    juce::SynthesiserSound::Ptr currentlyPlayingSound;
    bool keyIsDown = false, sustainPedalDown = false, sostenutoPedalDown = false;

protected:
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
   WHETHER EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR
   PURPOSE, ARE DISCLAIMED.
  ==============================================================================
*/

class BKSamplerVoice : public BKSynthesiserVoice
{
public:
    explicit BKSamplerVoice()
    {
        ampEnv.setParameters(BKADSR::Parameters(0.005, 0.5,1.0,0.1));
        m_Buffer.setSize(2, 1, false, true, false);
        tuningAttached = false;
        // call the smooth.reset() here if we want smoothing for various smoothingFloat vars
    }

    void setCurrentPlaybackSampleRate(double newRate) override {

        if (newRate <= 0) {
            return;
        }

        ampEnv.setSampleRate(newRate);
        ampEnv.setParameters(ampEnv.getParameters());
    }

    virtual void startNote (int midiNoteNumber,
                            float velocity,
                            float transposition,
                            bool tune_transpositions,
                            BKSamplerSound<juce::AudioFormatReader> * _sound,
                            int currentPitchWheelPosition)
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
        currentSamplePos = 0.0;
        tailOff = 0.0;

        ampEnv.noteOn();
    }

    void setTuning(TuningState* attachedTuning)
    {
        if (attachedTuning == nullptr) return;
        tuning = attachedTuning;
        tuningAttached = true;
    }

    double getTargetFrequency()
    {
        // if there is no Tuning prep connected, just return the equal tempered frequency
        if (tuning == nullptr || !tuningAttached) return mtof ((double) currentlyPlayingNote + currentTransposition);

        // otherwise, get the target frequency from the attached Tuning pre
        return tuning->getTargetFrequency(currentlyPlayingNote, currentTransposition, tuneTranspositions);


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

    void setGain(float g)
    {
        voiceGain = g;
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

    void copyAmpEnv(BKADSR::Parameters parameters)
    {
        updateAmpEnv(parameters);
    }

private:

    void updateAmpEnv(BKADSR::Parameters &parameters) {
        ampEnv.setParameters(parameters);
        // todo: update mod amount
    }

    template <typename Element>
    void render(juce::AudioBuffer<Element>& outputBuffer, int startSample, int numSamples)
    {
        jassert(samplerSound->getSample() != nullptr);
        //updateParams(); // NB: important line (except this function doesn't do anything right now!)

        /*
         * don't change tuning after noteOn for tuningType tunings
         * do need to for spring, and probably for regular notes it might be handy
         */
         if((tuning->getTuningType() == Static) || (tuning->getTuningType() == Spring_Tuning)) {
             sampleIncrement.setTargetValue ((getTargetFrequency() / samplerSound->getCentreFrequencyInHz()) * samplerSound->getSample()->getSampleRate() / this->currentSampleRate);
         }

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

        // Very simple linear interpolation here because the Sampler class should have already upsampled.
        auto l = static_cast<Element> ((inL[pos] * invAlpha + inL[nextPos] * alpha));
        auto r = static_cast<Element> ((inR != nullptr) ? (inR[pos] * invAlpha + inR[nextPos] * alpha)
                                                        : l);
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

        std::tie(currentSamplePos, currentDirection) = getNextState(currentIncrement,currentLoopBegin,currentLoopEnd);

        if (currentSamplePos > samplerSound->getSample()->getLength())
        {
            stopNote();
            return false;
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

    enum class Direction
    {
        forward,
        backward
    };

    [[nodiscard]] std::tuple<double, Direction> getNextState(double inc,
                                               double begin,
                                               double end) const
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
            nextDirection = Direction::forward;

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

    BKSamplerSound<juce::AudioFormatReader>* samplerSound;
    float voiceGain {1.};

    double currentTransposition; // comes from Transposition sliders in Direct/Nostalgic/Synchronic
    bool tuneTranspositions = false; // if this is true, then Transposition slider values will be tuned using the current tuning system (in TuningState)

    juce::SmoothedValue<double> level { 0 };
    juce::SmoothedValue<double> loopBegin;
    juce::SmoothedValue<double> loopEnd;
    juce::SmoothedValue<double> sampleIncrement { 0. }; // how far to move through sample, to effect transpositions

    double currentSamplePos { 0 };
    double tailOff { 0 };
    Direction currentDirection{ Direction::forward };

    BKADSR ampEnv;
    TuningState* tuning;
    bool tuningAttached;

    juce::AudioBuffer<float> m_Buffer;
};

