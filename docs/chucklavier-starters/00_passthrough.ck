// 00_passthrough.ck — audio + MIDI passthrough
// Globals used: bkMidiIn, bkMidiOut, bkMidiInEvent, bkMidiOutEvent
// Trigger: any MIDI note from a connected Keymap; audio from Internal or External inputs
//
// Routes adc → dac and mirrors every incoming MIDI message back out.
// Use this as a baseline / reset template.

global int bkMidiIn[4];
global int bkMidiOut[4];
global Event bkMidiInEvent;
global Event bkMidiOutEvent;
global float bkTuningTable[128];
global float bkTempoBPM;

adc => dac;

fun void midiPassthrough() {
    while (true) {
        bkMidiInEvent => now;
        bkMidiIn[0] => bkMidiOut[0];
        bkMidiIn[1] => bkMidiOut[1];
        bkMidiIn[2] => bkMidiOut[2];
        bkMidiIn[3] => bkMidiOut[3];
        bkMidiOutEvent.broadcast();
    }
}
spork ~ midiPassthrough();

while (true) { 1::samp => now; }
