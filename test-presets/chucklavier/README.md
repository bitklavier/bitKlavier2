# ChucKlavier Starter Galleries

Six `.bk2` galleries demonstrating ChucKlavier capabilities. Each contains a
Direct + Keymap + Tuning + ChucKlavier topology.

Human-readable source scripts live alongside in
`docs/chucklavier-starters/` (same base filename, `.ck` extension).

## Files

| File | Piano name | What it shows |
|---|---|---|
| `00_passthrough.bk2` | Audio + MIDI Passthrough | Audio `adc → dac`; MIDI mirrored to output. Baseline template. |
| `01_sine_from_tuning.bk2` | 8-Voice Sine Synth | 8-voice polyphonic `SinOsc`; frequency tracks `bkTuningTable` live. |
| `02_tempo_delay.bk2` | Tempo-Synced Delay | `Delay` with feedback; delay time = one quarter-note at `bkTempoBPM`. |
| `03_arpeggiator.bk2` | MIDI Arpeggiator | Emits root + 3rd + 5th via `bkMidiOut` each eighth-note. |
| `04_resonz_instrument.bk2` | ResonZ Instrument | `adc → ResonZ → Envelope → dac`; centre freq from `bkTuningTable`. Requires an upstream audio source on the Internal bus. |
| `05_flute_instrument.bk2` | STK Flute Instrument | STK `Flute` physical model; `noteOn`/`noteOff` with tuning tracking. |

## Local install

Copy to your bitKlavier galleries folder to get a "ChucKlavier" submenu
in the header gallery selector:

```bash
mkdir -p ~/Documents/bitKlavier/galleries/ChucKlavier
cp *.bk2 ~/Documents/bitKlavier/galleries/ChucKlavier/
```

## Shipping

To include these in the installer, add the `.bk2` files to
`bitKlavier_InstallerResources.zip` on the siteGround server under
`bitKlavier/galleries/ChucKlavier/`, then push to trigger a CI run
with `INCLUDE_RESOURCES: true`.
