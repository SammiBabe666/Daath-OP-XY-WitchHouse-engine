# Daath validation notes

Validated against Open-XY SDK commit `4a8abf4ead97e870c79bc193dd4fd41265732ee7`
(`staging` at release 0.0.3, SDK 1.5, OS 1.2.1), the commit pinned in CI. The
machine keeps the SDK 1.4 format and its engine and page numbers.

## Passing on the host

- `sdk.py test --machine .`: definition, artwork, preview build and the
  SDK's ABI, fixed-memory, input-ownership, declared-peak, voice-lifecycle and
  drawing smoke checks, plus the five test files below.
- All tests also pass under GCC's UndefinedBehaviorSanitizer and
  AddressSanitizer with `-Wall -Wextra -Werror` (run in CI).
- The browser workbench plays audio and draws every control's state without
  console errors.

| Test | Covers |
| --- | --- |
| `tests/sound.cpp` | All 256 control corners at seven pitches from MIDI 12 to 120, with restarts: bounded by the declared peak and audible. Random hostile control words and pitches for 4,000 blocks |
| `tests/presets.cpp` | The seven presets with eight voices, stolen voices, a Hex sweep and pitch bends: bounded, audible (RMS above 0.08 of peak) and free of DC |
| `tests/render.cpp` | Bit-exact output: a fixed workload (random and extreme controls, Swarm 0, pitch steps and bends) hashes to the value recorded from the original single-loop render |
| `tests/tuning.cpp` | The saw fundamental is the played pitch; Grave adds exactly the octave below; Swarm 0 is one saw for any voice; Swarm's upper half adds the fifth and the octave below; Amount 0 makes Speed inaudible and Amount moves the cutoff; a tiny Rot barely changes the signal; Murk only darkens; Hex leaves no DC |
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
The aim is 15,000. The audio loops have six Q31 multiplies per sample, table
lookups for the sub and soft clip, and no division; per-block work computes the
reciprocals, the filter coefficient and the level trim. `voiceCapacity` is 8,
stock's per-track note limit.

The first profiling build dropped single audio blocks on an OP-XY while notes
were held (a phone recording: 1.45 ms gaps, about 17% of blocks). One sample
loop carried about 13 state values and as many coefficients, far more than the
Blackfin's eight data registers, so the compiled loop spilled and reloaded
state every sample (about 190 instructions per sample on the common path). The
render now runs one pass per saw, one for the sub and one for the effects,
about 130 instructions per sample with the pinned GCC 8. At Swarm 0 the ghost
saws only advance their phases. The output is bit-for-bit identical to the
previous render over 18,000 blocks covering random and extreme controls,
Swarm 0, pitch bends and pitch steps. The host estimate (median about 17,800
after, 17,300 before) does not see register pressure.

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

The CI `blackfin` job compiles natively with the pinned tools.

**Hardware CPU (release 1.0, commit `ea60d4a`):** 3.461% of the stock 580,352-cycle
block budget per voice at its heaviest setting (Rot 25%), the median of three
captures of the full control sweep on an OP-XY running OS 1.2.1; within the
recommended 3.5%. See [hardware/cpu-1.0.md](hardware/cpu-1.0.md). The
captures came from a browser port of the SDK collector (Web MIDI, same frames
and checks; its SHA-256 is recorded in each capture) and were accepted by
`sdk.py profile report`. Host checks do not establish polyphony limits or
recovery.
