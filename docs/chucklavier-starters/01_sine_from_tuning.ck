// 01_sine_from_tuning.ck — 8-voice polyphonic SinOsc synth, tuning-aware
// Globals used: bkMidiIn, bkMidiInEvent, bkTuningTable
// Trigger: MIDI notes from a connected Keymap
//
// Maintains a pool of 8 voices. Note-on grabs a free voice (or steals the oldest
// if all are busy). Note-off releases the matching voice. Each voice polls
// bkTuningTable[note] every millisecond so live Tuning changes retune it in real time.

global int bkMidiIn[4];
global Event bkMidiInEvent;
global float bkTuningTable[128];

8 => int NUM_VOICES;
SinOsc voices[NUM_VOICES];
int voiceNote[NUM_VOICES];         // -1 = free
int voiceAge[NUM_VOICES];          // counter for LRU steal
int ageCounter;

for (0 => int i; i < NUM_VOICES; i++) {
    voices[i] => dac;
    0.0 => voices[i].gain;
    -1 => voiceNote[i];
    0 => voiceAge[i];
}

fun int allocVoice(int note) {
    // reuse same-note voice if already sounding
    for (0 => int i; i < NUM_VOICES; i++)
        if (voiceNote[i] == note) return i;
    // grab first free voice
    for (0 => int i; i < NUM_VOICES; i++)
        if (voiceNote[i] == -1) return i;
    // steal oldest
    0 => int oldest; voiceAge[0] => int minAge;
    for (1 => int i; i < NUM_VOICES; i++)
        if (voiceAge[i] < minAge) { i => oldest; voiceAge[i] => minAge; }
    return oldest;
}

fun void runVoice(int v) {
    while (voiceNote[v] != -1) {
        bkTuningTable[voiceNote[v]] => voices[v].freq;
        1::ms => now;
    }
    0.0 => voices[v].gain;
}

fun void midiHandler() {
    while (true) {
        bkMidiInEvent => now;
        (bkMidiIn[0] & 0xF0) => int status;
        bkMidiIn[1] => int note;
        bkMidiIn[2] => int vel;

        if (status == 0x90 && vel > 0) {
            allocVoice(note) => int v;
            note => voiceNote[v];
            ageCounter++ => voiceAge[v];
            vel / 127.0 * 0.25 => voices[v].gain;   // scale so 8 voices don't clip
            spork ~ runVoice(v);
        } else if (status == 0x80 || (status == 0x90 && vel == 0)) {
            for (0 => int i; i < NUM_VOICES; i++)
                if (voiceNote[i] == note) -1 => voiceNote[i];
        }
    }
}
spork ~ midiHandler();

while (true) { 1::samp => now; }
