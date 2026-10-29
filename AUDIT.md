# Daath validation notes

Validated against Open-XY SDK commit `4a8abf4ead97e870c79bc193dd4fd41265732ee7`
(`staging` at release 0.0.3, SDK 1.5, OS 1.2.1), the commit pinned in CI. The
machine keeps the SDK 1.4 format and its engine and page numbers.

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
| `tests/display.cpp` | Every frame valid through 20,000 random steps; the four outlines keep 64 distinct points and enclose area; points never change without a new key; reset and track changes never fabricate a note; idle and reduced-motion screens stay still; a 20-minute hold at full Wow stays valid |

Offline measurements (host render, steady tones):

| Check | Result |
| --- | --- |
| Pitch, MIDI 24/36/48, Swarm 0 | Within one FFT bin (0.7 Hz) of the expected fundamental |
| Aliasing, C6 saw, everything else off | Non-harmonic energy below 15 kHz at −41 dB |
| Murk 0 → 100, C2 | Spectral centroid 3.0 kHz → 80 Hz, monotonic |
| Hex 0 → 100, C2 | RMS 0.21 → 0.35 of peak with level trim; DC under 0.002 |
| Preset DC, phrase render | Under 0.0015 of peak for all six |

## CPU

The SDK's host-timed estimate, which is not a device measurement, was about
17,300 Blackfin instructions per voice block at the heaviest setting (median of
seven runs; range 13,600 to 23,700 with timing noise) on SDK commit `4a8abf4`.
The aim is 15,000. The inner loop has six Q31 multiplies per sample, table
lookups for the sub and soft clip, and no division; per-block work computes the
reciprocals, the filter coefficient and the level trim. `voiceCapacity` is 8,
stock's per-track note limit.

## Review notes

- The Hex soft-clip index stays within 258..4086 for every Hex and Swarm
  setting (brute-forced), and the sub sine table index is always 0..1023.
- Fixed: the display's Wow sway multiplied the held-note clock by 2520 in
  signed 32-bit arithmetic, which overflowed after about 14 minutes of held
  notes. It is now unsigned, with the same result; `tests/display.cpp` covers it.
- The panel artwork meets the SDK artwork rules: at most four decimals, no
  exponents, styles, events or references, explicit paint, translate-only
  transforms on bound elements, four outlines of `M`, 63 `L` and `Z` with 64
  distinct points and no fill, explicit-font text slots without `x`/`y`, and
  sixteen `#RRGGBB` palette entries.

**Pending:** a native compile on the SDK's Windows host (the CI `blackfin`
job) and a hardware CPU readout per the SDK's
[profiling guide](https://github.com/etcetc000/open-xy/blob/staging/docs/profiling.md).
Neither has been run from this repository yet; host checks do not establish
device timings, polyphony limits or recovery.
