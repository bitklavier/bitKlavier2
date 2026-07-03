// 02_tempo_delay.ck — tempo-synced delay with feedback
// Globals used: bkTempoBPM
// Trigger: audio from Internal input (connect a Direct or other prep upstream)
//
// Passes adc (internal bussed audio) through a Delay whose time is one quarter-note
// at the current tempo. Feedback recirculates 40% of the delayed signal. Tempo
// changes are tracked every 100ms so the delay time stays musically in sync.
//
// delay.max is fixed at 4 seconds at init (covers ~15 BPM and up).
// Resizing delay.max after initialization has no effect in ChucK — so we set it
// large once and only update delay.delay in the loop.

global float bkTempoBPM;

adc => Delay delay => dac;
adc => dac;             // dry signal
delay => Gain fb => delay;  // explicit feedback gain
0.4 => fb.gain;         // 40% feedback

fun dur quarterNote() {
    if (bkTempoBPM <= 0.0) return 500::ms;
    return (60.0 / bkTempoBPM)::second;
}

4.0::second => delay.max;   // fixed max — must be large enough for any tempo
quarterNote() => delay.delay;

while (true) {
    quarterNote() => delay.delay;
    100::ms => now;
}
