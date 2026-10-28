# Daath validation notes

Validated against Open-XY SDK commit `1474100079395a88560a62e064612c4a21c0f0a0`
(SDK 1.4, OS 1.2.1), the commit pinned in CI.

## Passing on the host

- `sdk.py test --machine .`: definition, artwork, preview build and the
  SDK's ABI, fixed-memory, input-ownership, declared-peak, voice-lifecycle and
  drawing smoke checks, plus the four test files below.
- All tests also pass under GCC's UndefinedBehaviorSanitizer and
  AddressSanitizer with `-Wall -Wextra -Werror` (run in CI).
- The browser workbench plays audio and draws every control's state without
  console errors.

| Test | Covers |
| --- | --- |
| `tests/sound.cpp` | All 64 control corners at seven pitches from MIDI 12 to 120, with restarts: bounded by the declared peak and audible. Random hostile control words and pitches for 4,000 blocks |
| `tests/presets.cpp` | The six presets with eight voices, stolen voices, a Hex sweep and pitch bends: bounded, audible (RMS above 0.08 of peak) and free of DC |
| `tests/tuning.cpp` | The saw fundamental is the played pitch; Grave adds exactly the octave below; Swarm 0 is one saw for any voice; a tiny Rot barely changes the signal; Murk only darkens; Hex leaves no DC |
| `tests/display.cpp` | Every frame valid through 20,000 random steps; the four outlines keep 64 distinct points and enclose area; points never change without a new key; reset and track changes never fabricate a note; idle and reduced-motion screens stay still |

Offline measurements (host render, steady tones):

| Check | Result |
| --- | --- |
| Pitch, MIDI 24/36/48, Swarm 0 | Within one FFT bin (0.7 Hz) of the expected fundamental |
| Aliasing, C6 saw, everything else off | Non-harmonic energy below 15 kHz at −41 dB |
| Murk 0 → 100, C2 | Spectral centroid 3.0 kHz → 80 Hz, monotonic |
| Hex 0 → 100, C2 | RMS 0.21 → 0.35 of peak with level trim; DC under 0.002 |
| Preset DC, phrase render | Under 0.0015 of peak for all six |

## CPU

The SDK's host-timed estimate, which is not a device measurement, ranges
from about 12,500 to 22,300 Blackfin instructions per voice block on this
development machine, depending on compiler and timing noise. The aim is
15,000. The inner loop has six Q31 multiplies per sample, table lookups for
the sub and soft clip, and no division; per-block work computes the
reciprocals, the filter coefficient and the level trim. `voiceCapacity` is 16.

**Pending:** a native compile on the SDK's Windows host (the CI `blackfin`
job) and a hardware CPU readout per the SDK's
[profiling guide](https://github.com/etcetc000/open-xy/blob/staging/docs/profiling.md).
Neither has been run from this repository yet; host checks do not establish
device timings, polyphony limits or recovery.
