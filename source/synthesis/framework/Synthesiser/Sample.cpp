//
// Created by Davis Polito on 8/7/24.
//
#include "Sample.h"

//==============================================================================
BKSynthesiserVoice::BKSynthesiserVoice() {}

BKSynthesiserVoice::~BKSynthesiserVoice() {}

bool BKSynthesiserVoice::isPlayingChannel (const int midiChannel) const
{
    return currentPlayingMidiChannel == midiChannel;
}

void BKSynthesiserVoice::setCurrentPlaybackSampleRate (const double newRate)
{
    DBG ("BKSynthesiserVoice sample rate changed to " + juce::String (newRate));
    currentSampleRate = newRate;
}

bool BKSynthesiserVoice::isVoiceActive() const
{
    return getCurrentlyPlayingNote() >= 0;
}

void BKSynthesiserVoice::clearCurrentNote()
{
    currentlyPlayingNote = -1;
    currentlyPlayingSound = nullptr;
    currentPlayingMidiChannel = 0;
}

void BKSynthesiserVoice::aftertouchChanged (int) {}

void BKSynthesiserVoice::channelPressureChanged (int) {}

bool BKSynthesiserVoice::wasStartedBefore (const BKSynthesiserVoice& other) const noexcept
{
    return noteOnTime < other.noteOnTime;
}

