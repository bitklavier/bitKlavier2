//
// Created by Davis Polito on 8/12/24.
//

#ifndef BITKLAVIER2_BKBKSynthesiser_H
#define BITKLAVIER2_BKBKSynthesiser_H
#include <juce_core/juce_core.h>
#include "Sample.h"
#include "EnvParams.h"
#include "TuningProcessor.h"
#include "utils.h"

//==============================================================================
/**
    Base class for a musical device that can play sounds.

    To create a BKSynthesiser, you'll need to create a subclass of BKSynthesiserSound
    to describe each sound available to your mainSynth, and a subclass of  juce::BKSamplerVoice
    which can play back one of these sounds.

    Then you can use the addVoice() and addSound() methods to give the BKSynthesiser a
    set of sounds, and a set of voices it can use to play them. If you only give it
    one voice it will be monophonic - the more voices it has, the more polyphony it'll
    have available.

    Then repeatedly call the renderNextBlock() method to produce the audio. Any midi
    events that go in will be scanned for note on/off messages, and these are used to
    start and stop the voices playing the appropriate sounds.

    While it's playing, you can also cause notes to be triggered by calling the noteOn(),
    noteOff() and other controller methods.

    Before rendering, be sure to call the setCurrentPlaybackSampleRate() to tell it
    what the target playback rate is. This value is passed on to the voices so that
    they can pitch their output correctly.

    @tags{Audio}
*/
class BKSynthesiser
        {
                public:
                //==============================================================================
                /** Creates a new BKSynthesiser.
                    You'll need to add some sounds and voices before it'll make any sound.
                */
                BKSynthesiser(EnvParams&, chowdsp::GainDBParameter&);

                /** Destructor. */
                virtual ~BKSynthesiser();

                //==============================================================================
                /** Deletes all voices. */
                void clearVoices();

                /** Returns the number of voices that have been added. */
                int getNumVoices() const noexcept { return voices.size(); }

                /** Returns one of the voices that have been added. */
                 BKSamplerVoice* getVoice (int index) const;

                /** Adds a new voice to the mainSynth.
            
                    All the voices should be the same class of object and are treated equally.
            
                    The object passed in will be managed by the BKSynthesiser, which will delete
                    it later on when no longer needed. The caller should not retain a pointer to the
                    voice.
                */
                BKSamplerVoice* addVoice (BKSamplerVoice* newVoice);

                /** Deletes one of the voices. */
                void removeVoice (int index);

                //==============================================================================
                /** Deletes all sounds. */
                //void clearSounds();
                bool hasSamples() {return sounds != nullptr;}

                /** Returns the number of sounds that have been added to the mainSynth. */
                int getNumSounds() const noexcept { return sounds->size(); }

                /** Returns one of the sounds. */
                BKSamplerSound<juce::AudioFormatReader>::Ptr getSound (int index) const noexcept { return (*sounds)[index]; }

                /** Adds a new sound to the BKSynthesiser.
            
                    The object passed in is reference counted, so will be deleted when the
                    BKSynthesiser and all voices are no longer using it.
                */
//                BKSamplerSound<juce::AudioFormatReader>* addSound (const BKSamplerSound<juce::AudioFormatReader>::Ptr& newSound);
                /** Removes and deletes one of the sounds. */
//                void removeSound (int index);

                void addSoundSet(juce::ReferenceCountedArray<BKSamplerSound<juce::AudioFormatReader>>*);
                //==============================================================================
                /** If set to true, then the mainSynth will try to take over an existing voice if
                    it runs out and needs to play another note.
            
                    The value of this boolean is passed into findFreeVoice(), so the result will
                    depend on the implementation of this method.
                */
                void setNoteStealingEnabled (bool shouldStealNotes);

                /** Returns true if note-stealing is enabled.
                    @see setNoteStealingEnabled
                */
                bool isNoteStealingEnabled() const noexcept { return shouldStealNotes; }

                //==============================================================================
                /** Triggers a note-on event.
            
                    The default method here will find all the sounds that want to be triggered by
                    this note/channel. For each sound, it'll try to find a free voice, and use the
                    voice to start playing the sound.
            
                    Subclasses might want to override this if they need a more complex algorithm.
            
                    This method will be called automatically according to the midi data passed into
                    renderNextBlock(), but may be called explicitly too.
            
                    The midiChannel parameter is the channel, between 1 and 16 inclusive.
                */
                virtual void noteOn (int midiChannel,
                int midiNoteNumber,
                float velocity);

                /** Triggers a note-off event.
            
                    This will turn off any voices that are playing a sound for the given note/channel.
            
                    If allowTailOff is true, the voices will be allowed to fade out the notes gracefully
                    (if they can do). If this is false, the notes will all be cut off immediately.
            
                    This method will be called automatically according to the midi data passed into
                    renderNextBlock(), but may be called explicitly too.
            
                    The midiChannel parameter is the channel, between 1 and 16 inclusive.
                */
                virtual void noteOff (int midiChannel,
                int midiNoteNumber,
                float velocity,
                bool allowTailOff);

                /** Turns off all notes.
            
                    This will turn off any voices that are playing a sound on the given midi channel.
            
                    If midiChannel is 0 or less, then all voices will be turned off, regardless of
                    which channel they're playing. Otherwise it represents a valid midi channel, from
                    1 to 16 inclusive.
            
                    If allowTailOff is true, the voices will be allowed to fade out the notes gracefully
                    (if they can do). If this is false, the notes will all be cut off immediately.
            
                    This method will be called automatically according to the midi data passed into
                    renderNextBlock(), but may be called explicitly too.
                */
                virtual void allNotesOff (int midiChannel,
                bool allowTailOff);

                /** Sends a pitch-wheel message to any active voices.
            
                    This will send a pitch-wheel message to any voices that are playing sounds on
                    the given midi channel.
            
                    This method will be called automatically according to the midi data passed into
                    renderNextBlock(), but may be called explicitly too.
            
                    @param midiChannel          the midi channel, from 1 to 16 inclusive
                    @param wheelValue           the wheel position, from 0 to 0x3fff, as returned by juce::MidiMessage::getPitchWheelValue()
                */
                virtual void handlePitchWheel (int midiChannel,
                int wheelValue);

                /** Sends a midi controller message to any active voices.
            
                    This will send a midi controller message to any voices that are playing sounds on
                    the given midi channel.
            
                    This method will be called automatically according to the midi data passed into
                    renderNextBlock(), but may be called explicitly too.
            
                    @param midiChannel          the midi channel, from 1 to 16 inclusive
                    @param controllerNumber     the midi controller type, as returned by juce::MidiMessage::getControllerNumber()
                    @param controllerValue      the midi controller value, between 0 and 127, as returned by juce::MidiMessage::getControllerValue()
                */
                virtual void handleController (int midiChannel,
                int controllerNumber,
                int controllerValue);

                /** Sends an aftertouch message.
            
                    This will send an aftertouch message to any voices that are playing sounds on
                    the given midi channel and note number.
            
                    This method will be called automatically according to the midi data passed into
                    renderNextBlock(), but may be called explicitly too.
            
                    @param midiChannel          the midi channel, from 1 to 16 inclusive
                    @param midiNoteNumber       the midi note number, 0 to 127
                    @param aftertouchValue      the aftertouch value, between 0 and 127,
                                                as returned by juce::MidiMessage::getAftertouchValue()
                */
                virtual void handleAftertouch (int midiChannel, int midiNoteNumber, int aftertouchValue);

                /** Sends a channel pressure message.
            
                    This will send a channel pressure message to any voices that are playing sounds on
                    the given midi channel.
            
                    This method will be called automatically according to the midi data passed into
                    renderNextBlock(), but may be called explicitly too.
            
                    @param midiChannel              the midi channel, from 1 to 16 inclusive
                    @param channelPressureValue     the pressure value, between 0 and 127, as returned
                                                    by juce::MidiMessage::getChannelPressureValue()
                */
                virtual void handleChannelPressure (int midiChannel, int channelPressureValue);

                /** Handles a sustain pedal event. */
                virtual void handleSustainPedal (int midiChannel, bool isDown);

                /** Handles a sostenuto pedal event. */
                virtual void handleSostenutoPedal (int midiChannel, bool isDown);

                /** Can be overridden to handle soft pedal events. */
                virtual void handleSoftPedal (int midiChannel, bool isDown);

                /** Can be overridden to handle an incoming program change message.
                    The base class implementation of this has no effect, but you may want to make your
                    own mainSynth react to program changes.
                */
                virtual void handleProgramChange (int midiChannel,
                int programNumber);

                //==============================================================================
                /** Tells the BKSynthesiser what the sample rate is for the audio it's being used to render.
            
                    This value is propagated to the voices so that they can use it to render the correct
                    pitches.
                */
                virtual void setCurrentPlaybackSampleRate (double sampleRate);

                /** Creates the next block of audio output.
            
                    This will process the next numSamples of data from all the voices, and add that output
                    to the audio block supplied, starting from the offset specified. Note that the
                    data will be added to the current contents of the buffer, so you should clear it
                    before calling this method if necessary.
            
                    The midi events in the inputMidi buffer are parsed for note and controller events,
                    and these are used to trigger the voices. Note that the startSample offset applies
                    both to the audio output buffer and the midi input buffer, so any midi events
                    with timestamps outside the specified region will be ignored.
                */
                void renderNextBlock (juce::AudioBuffer<float>& outputAudio,
                    const juce::MidiBuffer& inputMidi,
                    int startSample,
                    int numSamples);

//                void renderNextBlock (juce::AudioBuffer<double>& outputAudio,
//                const juce::MidiBuffer& inputMidi,
//                int startSample,
//                int numSamples);

                /** Returns the current target sample rate at which rendering is being done.
                    Subclasses may need to know this so that they can pitch things correctly.
                */
                double getSampleRate() const noexcept                       { return sampleRate; }

                /** Sets a minimum limit on the size to which audio sub-blocks will be divided when rendering.
            
                    When rendering, the audio blocks that are passed into renderNextBlock() will be split up
                    into smaller blocks that lie between all the incoming midi messages, and it is these smaller
                    sub-blocks that are rendered with multiple calls to renderVoices().
            
                    Obviously in a pathological case where there are midi messages on every sample, then
                    renderVoices() could be called once per sample and lead to poor performance, so this
                    setting allows you to set a lower limit on the block size.
            
                    The default setting is 32, which means that midi messages are accurate to about < 1ms
                    accuracy, which is probably fine for most purposes, but you may want to increase or
                    decrease this value for your mainSynth.
            
                    If shouldBeStrict is true, the audio sub-blocks will strictly never be smaller than numSamples.
            
                    If shouldBeStrict is false (default), the first audio sub-block in the buffer is allowed
                    to be smaller, to make sure that the first MIDI event in a buffer will always be sample-accurate
                    (this can sometimes help to avoid quantisation or phasing issues).
                */
                void setMinimumRenderingSubdivisionSize (int numSamples, bool shouldBeStrict = false) noexcept;

                /** when keyRelease Synth is true, this mainSynth triggers notes from noteOff message
                 * instead of noteOn messages
                 * if KeyMap is inverting noteOn/noteOff modes, then of course this would get reversed
                 * but bksynth wouldn't know/care
                */
                void isKeyReleaseSynth(bool mode) { keyReleaseSynth = mode; }

                /**
                 * pedal synths only play sounds when the sustain pedal is pressed or released
                 */
                void isPedalSynth(bool mode) { pedalSynth = mode; }

//                /**
//                 * todo: not sure we ever use this or synthGain?
//                 * @param g
//                 */
//                void setSynthGain(float g) {
//                    synthGain.setParameterValue(g);
//                }

                void setTuning(TuningState* attachedTuning)
                {
                    tuning = attachedTuning;
                }

//                void setPlaybackDirection(Direction newdir)
//                {
//                    playbackDirection = newdir;
//                }

                void setBypassed(bool by) { bypassed = by;}
                bool isBypassed() { return bypassed; }

                /**

                 * @param newOffsets
                 * @param tune_transpositions
                 */
                void updateMidiNoteTranspositions(juce::Array<float>& newOffsets, bool tune_transpositions) {
                    midiNoteTranspositions = newOffsets;
                    tuneTranspositions = tune_transpositions;
                }

                /**
                 * get reference to any noteOn specifications (startTime, direction, loopmode) needed
                 * for any particular note number (key), copy to internal map for use with sampler.h
                 * @param inspecs
                 */
//                void setNoteOnSpecMap(std::map<int, NoteOnSpec>& inspecs)
//                {
//                    noteOnSpecs = inspecs;
//                }
                void setNoteOnSpecMap(std::array<NoteOnSpec, MaxMidiNotes>& inspecs)
                {
                    noteOnSpecs = inspecs;
                }

                BKSynthesizerState getSynthesizerState()
                {
                    return lastSynthState;
                }

protected:
                //==============================================================================
                /** This is used to control access to the rendering callback and the note trigger methods. */
                juce::CriticalSection lock;

                juce::OwnedArray< BKSamplerVoice> voices;

                juce::ReferenceCountedArray<BKSamplerSound<juce::AudioFormatReader>>* sounds;

                /** The last pitch-wheel values for each midi channel. */
                int lastPitchWheelValues [16];

                /** Renders the voices for the given range.
                    By default this just calls renderNextBlock() on each voice, but you may need
                    to override it to handle custom cases.
                */
                virtual void renderVoices (juce::AudioBuffer<float>& outputAudio,
                int startSample, int numSamples);


                /** Searches through the voices to find one that's not currently playing, and
                    which can play the given sound.
            
                    Returns nullptr if all voices are busy and stealing isn't enabled.
            
                    To implement a custom note-stealing algorithm, you can either override this
                    method, or (preferably) override findVoiceToSteal().
                */
                virtual  BKSamplerVoice* findFreeVoice (BKSamplerSound<juce::AudioFormatReader>* soundToPlay,
                int midiChannel,
                int midiNoteNumber,
                bool stealIfNoneAvailable) const;

                /** Chooses a voice that is most suitable for being re-used.
                    The default method will attempt to find the oldest voice that isn't the
                    bottom or top note being played. If that's not suitable for your mainSynth,
                    you can override this method and do something more cunning instead.
                */
                virtual  BKSamplerVoice* findVoiceToSteal (BKSamplerSound<juce::AudioFormatReader>* soundToPlay,
                int midiChannel,
                int midiNoteNumber) const;

                /** Starts a specified voice playing a particular sound.
                    You'll probably never need to call this, it's used internally by noteOn(), but
                    may be needed by subclasses for custom behaviours.
                */
                void startVoice (
                    BKSamplerVoice* voice,
                    BKSamplerSound<juce::AudioFormatReader>* sound,
                    int midiChannel,
                    int midiNoteNumber,
                    float velocity,
                    float transposition);

                /** Stops a given voice.
                    You should never need to call this, it's used internally by noteOff, but is protected
                    in case it's useful for some custom subclasses. It basically just calls through to
                    BKSamplerVoice::stopNote(), and has some assertions to sanity-check a few things.
                */
                void stopVoice ( BKSamplerVoice*, float velocity, bool allowTailOff);

                /** Can be overridden to do custom handling of incoming midi events. */
                virtual void handleMidiEvent (const juce::MidiMessage&);

private:
                //==============================================================================
                double sampleRate = 0;
                juce::uint32 lastNoteOnCounter = 0;
                int minimumSubBlockSize = 32;
                bool subBlockSubdivisionIsStrict = false;
                bool shouldStealNotes = true;
                juce::BigInteger sustainPedalsDown;
                mutable juce::CriticalSection stealLock;
                mutable juce::Array<BKSamplerVoice*> usableVoicesToStealArray;

                bool keyReleaseSynth = false;           // by default, synths play on keyPress (noteOn), not the opposite!
                bool pedalSynth = false;                // for sustain pedal sounds; will ignore noteOn messages
                bool sustainPedalAlreadyDown = false;   // to avoid re-triggering of pedalDown sounds
                EnvParams& adsrParams;

                /**
                 * midiNoteTranspositions is an arrays of tuning offsets, in MidiNoteCents (.01 = 1 cent)
                 *      - these offsets are set by currentTransposition controls in Direct, Nostalgic, and Synchronic
                 *
                 * by default, there is only one offset in each subarray, with value 0., for regular un-transposed notes
                 *
                 * through the parent preparation owning this synth, the user might set this array in a variety of ways:
                 *      - {0.} will just be the default note, no tuning adjustment
                 *      - {0., 0.5} will be the default note, plus another note a quarter-tone away
                 *      - {0., 12.5} will be the default note, plus another an active and a quarter-tone away
                 *      - {-5, 4, 12.5} will be down a 4th, up a 3rd, and up an octave and a quarter-tone from the played note
                 *                      and no un-transposed note
                 *      - {-0.5} default note is tuned down a quarter-tone
                 *
                 * Synchronic will send one of these with each beat
                 *
                 * the "un-transposed" midiNoteNumber is the midi note played by the performer, and used for voice tracking
                 */
                juce::Array<float> midiNoteTranspositions = { 0.}; // needs to be set via UI, for additional transpositions
                bool tuneTranspositions = false;
                juce::Array<juce::Array<BKSamplerVoice*>> playingVoicesByNote; // Array of current voices playing for a particular midiNoteNumber
                std::bitset<MaxMidiNotes> activeNotes;

                //std::map<int, NoteOnSpec> noteOnSpecs;
                std::array<NoteOnSpec, MaxMidiNotes> noteOnSpecs;

                TuningState* tuning;

                template <typename floatType>
                void processNextBlock (juce::AudioBuffer<floatType>&, const juce::MidiBuffer&, int startSample, int numSamples);

                //global gain for this synth; applies at the noteOn stage, NOT to the audio
                chowdsp::GainDBParameter& synthGain;

                BKSynthesizerState lastSynthState;

                // will be true if this synth is not in the active Piano, but is in the Gallery otherwise, so part of the AudioGraph
                bool bypassed = false;

                // becomes false when there are no voices active
                bool someVoicesActive = true;

                JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BKSynthesiser)
        };

#endif //BITKLAVIER2_BKBKSynthesiser_H
