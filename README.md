# Daath

A witch house bass engine for the OP-XY, built on the
[Open-XY SDK](https://github.com/etcetc000/open-xy) (OS 1.2.1, tested on SDK
`staging` at release 0.0.3; the machine format is SDK 1.4).

Five drifting saws over a sine an octave below, pushed through an asymmetric
drive, sunk under a pitch-tracking lowpass, rotted by sample-and-hold and bit
loss, and warped like slowed tape. The screen is a monochrome grimoire that
every knob rewrites.

![Daath's screen](media/screen.png)

▶ **[Sound and screen tour (MP4)](media/daath-tour.mp4)**: every control, then
every preset.

## Controls

Pressing any encoder switches both rows.

| Encoder | Page 1 | Page 2 |
| --- | --- | --- |
| Blue | **Swarm**: the lower half detunes four ghost saws around the centre saw, 0 to 40 cents (0 is one clean saw). The upper half turns them into a power stack, one at a time: a fifth up, an octave down, an octave up, a fifth below, each kept 6 cents off for a gentle beat | **Rot**: sample-and-hold from 44.1 kHz down to 1.4 kHz with up to 21 bits lost. 0 is a true bypass |
| Green | **Grave**: level of the sine one octave below | **Wow**: tape warp, two slow drifting rates, up to 45 cents |
| White | **Hex**: asymmetric cubic drive, gain 1 to 15 with even-harmonic bias and level trim | **Speed**: rate of Murk's own LFO, 0.1 to 10 Hz on an exponential turn |
| Orange | **Murk**: two-pole lowpass that tracks the note, from 256 harmonics down to the fundamental | **Amount**: how far the LFO swings Murk's cutoff, up to three octaves either way. 0 is off |

The header shows Swarm in cents, then the newest interval of its stack
(`5th`, `-8va`, `+8va`, `-5th`), Wow in cents, Speed in hertz and the rest as
amounts 0 to 100. Colouring and degrading controls (Hex, Rot, Wow, Amount)
default to off. The Murk LFO restarts on every note, so each note wobbles the
same way; the OP-XY's own LFOs stay free for anything else.

## Presets

Seven presets ship with the engine and install with the builder. Release 1.0's
six sound exactly as they did: Swarm's 0 to 40 cents now span its lower half,
so their Swarm values are halved.

| Preset | Swarm | Grave | Hex | Murk | Rot | Wow | Speed | Amount | Character |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| **Sigil** (default) | 17.5 | 50 | 0 | 30 | 0 | 0 | 0.5 Hz | 0 | The clean starting bass |
| **Crypt** | 30 | 80 | 35 | 55 | 0 | 25 | 0.5 Hz | 0 | Heavy, sub-led, slightly warped |
| **Coven** | 42.5 | 45 | 25 | 40 | 15 | 35 | 0.5 Hz | 0 | Wide choir of saws with a little grit |
| **Screwed** | 25 | 70 | 20 | 65 | 0 | 70 | 0.5 Hz | 0 | Dark, slowed and seasick |
| **Hexed** | 22.5 | 60 | 85 | 35 | 0 | 10 | 0.5 Hz | 0 | Driven and snarling |
| **Rotten** | 20 | 60 | 40 | 30 | 80 | 20 | 0.5 Hz | 0 | Crushed and decayed |
| **Seance** | 80 | 60 | 30 | 55 | 0 | 15 | 2 Hz | 50 | A fifth-and-octave stack wobbling through the murk |

## The grimoire

| Control | On the page |
| --- | --- |
| Swarm | Two ghost pentagrams split from the sigil and turn apart by up to 24 degrees |
| Grave | The moon waxes from a sliver to full, turns to a black sun past 75 and sinks |
| Hex | The candles flare taller and the summoning ring grows eight flames |
| Murk | The pages darken to black; the runes and sigil then glow white |
| Rot | Up to 55% of the runes crumble away, the rest shake loose, and the pages tear |
| Wow | While a note is held, the sigil sways and the rune lines waver |
| Speed, Amount | While a note is held, the sigil's inner circle breathes with the Murk LFO, up to 15% |

Each note flashes the sigil and throws a halo; holding a note turns the sigil
slowly. An idle screen stays still, and reduced motion shows the end state at
once.

## Build and test

Check out the SDK beside this repository at the commit pinned in
[the workflow](.github/workflows/machine.yml), then:

```sh
python ../open-xy/sdk.py preview --source . --out ../open-xy/local/daath-preview-001
python ../open-xy/tools/serve_harness.py --preview ../open-xy/local/daath-preview-001
python ../open-xy/sdk.py test --machine . --host-cxx g++
```

Open http://127.0.0.1:8917/, enable audio and play. For a device build, follow
the SDK's [build guide](https://github.com/etcetc000/open-xy/blob/staging/docs/build.md)
with `--machine` pointing here. Daath asks for engine 48 and page 96; a
firmware's layout may give it others.

| File | Owns |
| --- | --- |
| [definition.json](definition.json) | Controls, presets, resources and drawing budget |
| [core.h](core.h), [engine.cpp](engine.cpp) | Voice state and the integer DSP |
| [display.h](display.h), [display.cpp](display.cpp), [panel.svg.fragment](panel.svg.fragment) | The grimoire and two-row header |
| [presets/](presets) | The six device presets (generated from definition.json) |
| [generators/](generators) | Scripts that regenerate presets and artwork |
| [tests/](tests) | Host checks: bounds, presets, tuning, controls and display |
| [AUDIT.md](AUDIT.md) | DSP design, validation and what remains pending |

## Sound model

Everything is fixed point: Q27 signals with headroom, Q31 coefficients, no
floating point, no 64-bit arithmetic, no per-sample division and no lookup
tables.

- **Swarm**: five polyBLEP saws. Edge positions use a reciprocal computed once
  per block per saw. Ghost offsets are uneven (+1, −1, +0.43, −0.37 of the
  spread), so the beating never locks into a pulse. The ghosts fade in over
  the first eighth of the knob, and the mix is normalised by 1/(1 + 2g). Over
  the upper half each ghost moves to its interval as the turn passes the middle
  of its quarter (12.5%, 37.5%, 62.5%, 87.5% of the half); an interval that
  would land above about 21 kHz stays detuned instead.
- **Grave**: a computed sine (a refined parabola, within 0.1%; harmonics below
  −63 dB) at half the centre saw's increment, so it follows Wow and bends exactly.
- **Hex**: gain on a squared curve with up to a quarter of bias, through the
  soft clip x − 4x³/27 at half scale, computed with two multiplies. A DC blocker follows
  (about 14 Hz). Output trims by up to 5.6 dB over the first half of the knob.
- **Murk**: two cascaded one-poles. The coefficient follows the played
  increment, so the tone is the same in every octave. Its LFO is a sine
  updated once per block that moves the cutoff in sixteenth-octave steps,
  clamped at fully open and fully closed.
- **Rot**: a branchless sample-and-hold clock and a bit mask on the held sample.
- **Wow**: two slow sines (about 0.43 and 1.1 Hz, varied per voice) bend every
  oscillator.

Oscillators run freely across notes, so legato and mono lines never click;
the OP-XY's envelopes, filter, LFOs and FX shape everything after the engine.

## Polyphony and CPU

Daath plays **8 voices** at once (`voiceCapacity` 8). That matches stock's
limit of 8 notes per track, so one track never steals from itself beyond its
own voices. Voice memory is 88 bytes per voice, plus stock's one spare.

**Measured on an OP-XY (release 1.0):** at most **3.461%** of the 1.45 ms audio
block per voice, within the SDK's recommended 3.5%. That is the heaviest of 56
runs (each control through nine positions, the stock workload and every
control at its heaviest), each the median of three captures; the heaviest
setting is Rot 25%. The stock workload reads 3.406%, and every control at its
heaviest 2.661%. Full table: [hardware/cpu-1.0.md](hardware/cpu-1.0.md);
the catalog record is [hardware/catalog-cpu-1.0.json](hardware/catalog-cpu-1.0.json).
Raw captures stay private: they hold the device's identity.

Swarm's power stack and the Murk LFO came after release 1.0. They add a few
instructions per block, not per sample (the sample loops are unchanged), but
a new revision needs its own hardware readout before a catalog update.

How it got there, each step measured or counted on the device's processor:

| Version | What changed | Per voice on the OP-XY |
| --- | --- | --- |
| First build | One sample loop with 13 state values for 8 data registers | dropped about 17% of audio blocks while notes were held |
| `cb66ed4` | One pass per oscillator and one for the effects (about 190 → 130 instructions per sample) | 4.33% at its heaviest (Hex 37.5%) |
| `ea60d4a`, release 1.0 | Hex and the sub computed instead of read from tables (the Hex table missed the data cache), passes merged | **3.461%** at its heaviest (Rot 25%) |

The SDK's host-timed estimate (about 17,000 to 26,000 instructions per voice
block, depending on timing noise) times a desktop processor and does not see
the OP-XY's registers or cache. Fewer voices limit how many run at once; they
do not change the cost of each voice.

## Licence

MIT; see [LICENSE](LICENSE).
