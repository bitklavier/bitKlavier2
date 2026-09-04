// 05_flute_instrument.ck — STK Flute physical model, tuning-aware
// Globals used: bkMidiIn, bkMidiInEvent, bkTuningTable
// Trigger: MIDI notes from a connected Keymap
//
// Uses the STK Flute physical model. Note-on sets frequency from bkTuningTable[note]
// and calls noteOn(velocity); note-off calls noteOff. Frequency continues tracking
// the Tuning table while the note is held so live Tuning changes retune it.

global int bkMidiIn[4];
global Event bkMidiInEvent;
global float bkTuningTable[128];

Flute flute => dac;

// sensible defaults
0.5 => flute.jetDelay;          // embouchure tightness (0=tight, 1=loose)
0.5 => flute.jetReflection;     // jet reflection coefficient
0.5 => flute.endReflection;     // bore end reflection
0.1 => flute.noiseGain;         // breath noise
0.5 => flute.vibratoFreq;       // vibrato rate (Hz)
0.1 => flute.vibratoGain;       // vibrato depth
0.7 => flute.gain;

int currentNote;
-1 => currentNote;

fun void trackTuning() {
    while (currentNote >= 0) {
        bkTuningTable[currentNote] => flute.freq;
        2::ms => now;
    }
}

fun void midiHandler() {
    while (true) {
        bkMidiInEvent => now;
        (bkMidiIn[0] & 0xF0) => int status;
        bkMidiIn[1] => int note;
        bkMidiIn[2] => int vel;

        if (status == 0x90 && vel > 0) {
            note => currentNote;
            bkTuningTable[note] => flute.freq;
            flute.noteOn(vel / 127.0);
            spork ~ trackTuning();
        } else if (status == 0x80 || (status == 0x90 && vel == 0)) {
            if (currentNote == note) {
                -1 => currentNote;
                flute.noteOff(0.5);
            }
        }
    }
}
spork ~ midiHandler();

while (true) { 1::samp => now; }
