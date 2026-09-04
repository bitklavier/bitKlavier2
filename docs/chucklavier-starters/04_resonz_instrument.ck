// 04_resonz_instrument.ck — resonant filter instrument driven by MIDI + Tuning
// Globals used: bkMidiIn, bkMidiInEvent, bkTuningTable
// Trigger: audio from Internal input (connect a Direct or Blendronic upstream);
//          MIDI from a connected Keymap
//
// Routes incoming audio through a ResonZ bandpass filter whose centre frequency
// tracks bkTuningTable[note]. An Envelope shapes note-on/off.
// Q=50 gives a narrow, bell-like resonance; lower Q for a broader filter colour.

global int bkMidiIn[4];
global Event bkMidiInEvent;
global float bkTuningTable[128];

adc => ResonZ reson => Envelope env => dac;

50.0 => reson.Q;
440.0 => reson.freq;
// compensate for narrow-band power loss: gain ~ Q/2 keeps perceived loudness stable
reson.Q() / 2.0 => reson.gain;

0.01::second => env.duration;   // 10ms attack
1.0 => env.value;               // start open; note-on/off will gate

0 => env.keyOn;                 // start silent

int currentNote;
-1 => currentNote;

fun void updateFreq() {
    while (currentNote >= 0) {
        bkTuningTable[currentNote] => reson.freq;
        reson.Q() / 2.0 => reson.gain;
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
            bkTuningTable[note] => reson.freq;
            reson.Q() / 2.0 => reson.gain;
            1 => env.keyOn;
            spork ~ updateFreq();
        } else if (status == 0x80 || (status == 0x90 && vel == 0)) {
            if (currentNote == note) {
                -1 => currentNote;
                1 => env.keyOff;
            }
        }
    }
}
spork ~ midiHandler();

while (true) { 1::samp => now; }
