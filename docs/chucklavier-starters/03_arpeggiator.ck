// 03_arpeggiator.ck — note-triggered MIDI arpeggiator (root + 3rd + 5th)
// Globals used: bkMidiIn, bkMidiOut, bkMidiInEvent, bkMidiOutEvent, bkTuningTable, bkTempoBPM
// Trigger: MIDI note-on from a connected Keymap
//
// On each note-on, emits three follow-on notes at the played velocity:
//   root → +4 semitones (+3rd) → +7 semitones (+5th)
// spaced one eighth-note apart at the current tempo.
// The emitted MIDI-out events are picked up by the connected Direct preparation.

global int bkMidiIn[4];
global int bkMidiOut[4];
global Event bkMidiInEvent;
global Event bkMidiOutEvent;
global float bkTuningTable[128];
global float bkTempoBPM;

fun dur eighthNote() {
    if (bkTempoBPM <= 0.0) return 250::ms;
    return (30.0 / bkTempoBPM)::second;
}

fun void sendNote(int note, int vel, int chan) {
    note & 127 => note;
    if (note < 0) return;
    (vel > 0 ? 0x90 : 0x80) | (chan & 0x0F) => bkMidiOut[0];
    note => bkMidiOut[1];
    vel => bkMidiOut[2];
    0 => bkMidiOut[3];
    bkMidiOutEvent.broadcast();
}

fun void arpChord(int root, int vel) {
    int intervals[3];
    0 => intervals[0]; 4 => intervals[1]; 7 => intervals[2];
    eighthNote() => dur step;
    for (0 => int i; i < 3; i++) {
        sendNote(root + intervals[i], vel, 0);
        step => now;
        sendNote(root + intervals[i], 0, 0);  // note-off
    }
}

fun void midiHandler() {
    while (true) {
        bkMidiInEvent => now;
        (bkMidiIn[0] & 0xF0) => int status;
        bkMidiIn[1] => int note;
        bkMidiIn[2] => int vel;
        if (status == 0x90 && vel > 0)
            spork ~ arpChord(note, vel);
    }
}
spork ~ midiHandler();

while (true) { 1::samp => now; }
