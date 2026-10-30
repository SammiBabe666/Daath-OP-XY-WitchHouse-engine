// Daath: a witch house bass voice.
//
// Five drifting saws (the swarm) over a sine one octave down (the grave),
// pushed into an asymmetric cubic drive (hex), sunk under a pitch-tracking
// two-pole lowpass (murk), then rotted by sample-and-hold and bit loss (rot).
// Wow bends the whole voice like warped tape.
//
// All audio is integer: signals are Q27 (1.0 = 2^27) so sums keep headroom,
// coefficients are Q31. No floating point, no 64-bit arithmetic, and at most
// a handful of divisions per block and none per sample.
#include "core.h"
#include <opxy/q31.hpp>
#include "tables.h"

namespace daath {
namespace {

using opxy::q31::multiply;

const i32 one = 1 << 27;                       // Q27 unity
const i32 q31_max = 0x7fffffff;

// 2^(k/16) for k = 0..16, Q15. Sixteenth-octave steps for Murk and Rot.
const u32 octave_q15[17] = {32768, 34219, 35734, 37316, 38968, 40693, 42495, 44376,
                            46341, 48393, 50535, 52773, 55109, 57549, 60097, 62757, 65536};

// Swarm spread at full turn: the outer ghosts sit 40 cents away. Q31 ratio
// offset 2^(40/1200) - 1. Ghost positions are uneven so the beating never
// locks into a regular pulse.
const i32 swarm_spread = 50195051;
const i32 ghost_offset[saw_count] = {0, q31_max, -q31_max, 923417968, -794568949};  // 0, +1, -1, +.43, -.37

// Wow at full turn: up to 45 cents of warp. Q31 ratio offset.
const i32 wow_depth = 56551370;
// Wow rates per 64-sample block: about 0.43 Hz and 1.1 Hz.
const u32 wow_rate[2] = {2680000u, 6856000u};

const i32 smooth_rate = 0x20000000;             // 1/4 per block, ~5 ms control glide
const i32 output_peak = 0x10000000;             // definition.json outputPeakQ31

inline i32 nonnegative(i32 value) { return value < 0 ? 0 : value; }
inline i32 clamp(i32 value, i32 low, i32 high) { return value < low ? low : value > high ? high : value; }

// Q27 naive saw from a phase: -1 at phase 0 rising to +1.
inline i32 saw(u32 phase) { return static_cast<i32>(phase >> 4) - one; }

// (1 - x)^2 for x in [0, 1], Q15 in, Q27 out: the polyBLEP residual.
inline i32 residual(u32 x_q15) {
    const i32 r = 32768 - static_cast<i32>(x_q15 > 32768u ? 32768u : x_q15);
    return (r * r) >> 3;  // Q30 >> 3 = Q27
}

// Table sine of a full-cycle phase, Q27.
inline i32 sine(u32 phase) { return sub_sine[phase >> 22] * 4096; }

// Unsigned 2^(-steps/16), Q16, for steps in sixteenths of an octave.
inline u32 octaves_down(u32 steps) {
    const u32 whole = steps >> 4, part = steps & 15;
    return whole > 16 ? 0 : (octave_q15[16 - part] >> whole);  // Q15 * 2 = Q16 at part 0
}

// Hex: gain, bias, then the soft clip x - 4x^3/27 from a table. Q27 in and
// out, |out| <= 1/2. x is clamped so that |x * gain| <= 1.74; with at most
// 0.25 of bias the table index always lands inside [-2, 2).
inline i32 hex(i32 x, i32 gain, i32 bias, i32 x_limit) {
    x = clamp(x, -x_limit, x_limit);
    const i32 v = multiply(x, gain) * 32 + bias;               // Q26 gain
    return hex_curve[static_cast<u32>(v + 2 * one) >> 17] * 4096;
}

struct Block {
    i32 ghost_gain;          // ghosts relative to the centre saw, Q31
    i32 grave_gain;          // sub relative to the centre saw, Q29 (up to 3.3)
    i32 gain, bias, x_limit; // Hex gain including the swarm normalisation, Q26
    i32 level;               // output gain, Q29: x3 makeup, less as Hex compresses
    i32 murk;                // lowpass coefficient, Q31
    u32 hold_step;           // Rot clock, Q16 per sample; 65536 holds nothing
    u32 rot_mask;            // bits Rot discards from the Q27 sample
    i32 delta[saw_count];    // swarm offsets from the played increment, this block
    u32 shift[saw_count];    // polyBLEP position: ((phase >> shift) * reciprocal) >> 16 is Q15
    u32 reciprocal[saw_count];
};

inline void prepare(Voice& voice, const i32* parameters, u32 increment, Block& b) {
    for (u32 c = 0; c < control_count; ++c) {
        const i32 target = nonnegative(parameters[c]);
        if (!voice.initialized) voice.control[c] = target;
        else voice.control[c] += multiply(target - voice.control[c], smooth_rate);
    }
    voice.initialized = 1;
    const i32 swarm = voice.control[0], grave = voice.control[1], drive = voice.control[2];
    const i32 murk = voice.control[3], rot = voice.control[4], wow = voice.control[5];

    // Ghosts fade in over the first quarter of Swarm, so zero is one clean saw.
    // The swarm is normalised by 1 / (1 + 2g) and mixed at half; the sub comes
    // in at up to 0.55. Normalisation rides on the Hex gain, so the mix is
    // summed relative to the centre saw: one multiply fewer per sample.
    b.ghost_gain = swarm >= 0x20000000 ? q31_max : swarm * 4;
    const u32 denominator = 32768u + 2u * static_cast<u32>(b.ghost_gain >> 16);  // (1 + 2g), Q15
    const i32 norm = static_cast<i32>((0x7fffffffu / denominator) << 14);         // 0.5 / (1 + 2g), Q31
    b.grave_gain = multiply(grave, static_cast<i32>(denominator * 18022u));       // 1.1 (1 + 2g) grave, Q29

    // Hex: gain 1..15 on a squared curve, and up to a quarter of bias for
    // even harmonics. The DC blocker removes the offset afterwards.
    const i32 drive_gain = (1 << 26) + multiply(multiply(drive, drive), 14 << 26);  // Q26
    b.gain = multiply(drive_gain, norm);
    b.bias = multiply(drive, one >> 2);
    b.x_limit = static_cast<i32>((233538355u / static_cast<u32>(b.gain >> 16)) * 1024u);  // |x * gain| <= 1.74
    // Drive raises loudness as it saturates; trim up to about 5.6 dB over the
    // first half of Hex so it changes tone more than level. 3 (1 - 0.95 d), Q29.
    const i32 trim = drive < 0x40000000 ? drive : 0x40000000;
    b.level = 3 * ((1 << 29) - multiply(trim, 510027366));

    // Wow: two slow sines summed, shared by every saw and the sub.
    voice.wow_phase[0] += wow_rate[0] + (voice.seed & 0x3ffffu);
    voice.wow_phase[1] += wow_rate[1] + ((voice.seed >> 8) & 0x3ffffu);
    const i32 lfo = multiply(sine(voice.wow_phase[0]), 1288490189) + multiply(sine(voice.wow_phase[1]), 858993459);
    const i32 bend = multiply(multiply(wow, wow_depth), lfo * 16);  // Q27 lfo -> Q31
    const i32 half = static_cast<i32>(increment >> 1);
    const i32 warp = multiply(half, bend) * 2;
    const i32 spread = multiply(swarm, swarm_spread);
    for (u32 k = 0; k < saw_count; ++k) {
        b.delta[k] = warp + multiply(half, multiply(spread, ghost_offset[k])) * 2;
        // A reciprocal per block replaces a division at every wrap.
        const u32 step = increment + static_cast<u32>(b.delta[k]);
        u32 shift = 0;
        while ((step >> shift) >= 65536u) ++shift;
        const u32 reciprocal = 0x7fffffffu / ((step >> shift) | 1u);
        b.shift[k] = shift;
        b.reciprocal[k] = reciprocal > 65535u ? 65535u : reciprocal;
    }

    // Murk: cutoff tracks the note, from 256 harmonics open (zero) down to the
    // fundamental itself (full). a ~ 2 pi fc / fs = pi * multiple * increment, Q31.
    const u32 steps = static_cast<u32>(murk >> 15) * 128u >> 16;  // 0..128 sixteenths
    const u32 k = (12864u * octaves_down(steps)) >> 12;          // pi * 256 harmonics * 256 (the >> 8), times 2^-(8 murk)
    const u32 scaled = (increment + static_cast<u32>(warp)) >> 8;
    const u32 ceiling = 0x7a000000u;                              // a <= 0.95
    b.murk = static_cast<i32>(k && scaled > ceiling / k ? ceiling : scaled * k);

    // Rot: zero is a bypass. Above it the hold clock falls over five octaves
    // (44.1 kHz to 1.4 kHz) and up to 21 of the 28 sample bits are lost.
    if (rot == 0) {
        b.hold_step = 65536u;
        b.rot_mask = 0xffffffffu;
    } else {
        const u32 r = static_cast<u32>(rot >> 16);                // Q15
        b.hold_step = octaves_down(r * 80u >> 15);
        if (b.hold_step == 0) b.hold_step = 1;
        const u32 lost = 1u + (r * 21u >> 15);
        b.rot_mask = ~((1u << lost) - 1u);
    }
}
} // namespace

inline void initialize(Voice& voice, Shared&, u32 owner) {
    voice.seed = owner * 2654435761u;
    // The centre saw starts at its zero crossing; ghosts scatter so the swarm
    // is already moving on the first cycle.
    voice.phase[0] = 0x80000000u;
    for (u32 k = 1; k < saw_count; ++k) voice.phase[k] = voice.seed * (2u * k + 1u) + 0x9e3779b9u * k;
    voice.sub_phase = 0;
    voice.wow_phase[0] = voice.seed;
    voice.wow_phase[1] = voice.seed ^ 0x55555555u;
    voice.low1 = voice.low2 = voice.dc_in = voice.dc_out = voice.held = 0;
    voice.hold_phase = 0;
    for (u32 c = 0; c < control_count; ++c) voice.control[c] = 0;
    voice.initialized = 0;
}

// Oscillators run free across notes so legato and mono lines never click;
// the stock amplitude envelope shapes each attack.
inline void start(Voice&, Shared&, u32) {}
inline void destroy(Voice&, Shared&, u32) {}

namespace {
// The Blackfin has eight data registers. Rendering in short passes, one per
// oscillator and one for the effects, keeps each pass's state in registers
// instead of spilling it every sample. Integer sums are exact, so the order
// of the passes does not change a single output bit.

// One polyBLEP saw across the block, written to or added into out. The
// residual is applied just before and just after each wrap, using the
// position within one sample step.
template <bool accumulate>
inline void saw_pass(u32& phase_state, opxy::Pitch pitch, u32 delta, u32 shift, u32 reciprocal,
                     i32* __restrict out) {
    u32 phase = phase_state;
    for (unsigned i = 0; i != opxy::block_frames; ++i) {
        const u32 increment = pitch[i] + delta;
        i32 value = saw(phase);
        // Within one step either side of the wrap: one branch, rarely taken.
        if (phase + increment < increment * 2u) {
            if (phase < increment) value += residual(((phase >> shift) * reciprocal) >> 16);
            else value -= residual((((0u - phase) >> shift) * reciprocal) >> 16);
        }
        phase += increment;
        if (accumulate) out[i] += value;
        else out[i] = value;
    }
    phase_state = phase;
}
} // namespace

inline void render(Voice& voice, Shared&, u32, opxy::Output output,
                   opxy::Parameters parameters, opxy::Pitch pitch) {
    Block b;
    prepare(voice, parameters, pitch[0], b);

    // Swarm: the centre saw goes to the output buffer, the four ghosts to
    // their own. At Swarm 0 the ghosts are silent: they only advance.
    saw_pass<false>(voice.phase[0], pitch, static_cast<u32>(b.delta[0]), b.shift[0], b.reciprocal[0], output);
    i32 ghosts[opxy::block_frames];
    if (b.ghost_gain != 0) {
        saw_pass<false>(voice.phase[1], pitch, static_cast<u32>(b.delta[1]), b.shift[1], b.reciprocal[1], ghosts);
        for (u32 k = 2; k < saw_count; ++k)
            saw_pass<true>(voice.phase[k], pitch, static_cast<u32>(b.delta[k]), b.shift[k], b.reciprocal[k], ghosts);
        const i32 ghost_gain = b.ghost_gain;
        for (unsigned i = 0; i != opxy::block_frames; ++i) output[i] += multiply(ghosts[i], ghost_gain);
    } else {
        u32 total = 0;
        for (unsigned i = 0; i != opxy::block_frames; ++i) total += pitch[i];
        for (u32 k = 1; k < saw_count; ++k)
            voice.phase[k] += total + static_cast<u32>(b.delta[k]) * opxy::block_frames;
    }

    // Grave: a sine an octave under the warped centre pitch, added to the centre.
    {
        u32 sub_phase = voice.sub_phase;
        const u32 delta = static_cast<u32>(b.delta[0]);
        const i32 grave = b.grave_gain;
        for (unsigned i = 0; i != opxy::block_frames; ++i) {
            output[i] += multiply(sine(sub_phase), grave) * 4;
            sub_phase += (pitch[i] + delta) >> 1;
        }
        voice.sub_phase = sub_phase;
    }

    // Hex, the DC blocker, Murk and Rot, on local copies of their state.
    i32 dc_in = voice.dc_in, dc_out = voice.dc_out, low1 = voice.low1, low2 = voice.low2, held = voice.held;
    u32 hold_phase = voice.hold_phase;
    for (unsigned i = 0; i != opxy::block_frames; ++i) {
        const i32 driven = hex(output[i], b.gain, b.bias, b.x_limit);

        // DC blocker (pole 1 - 1/512, about 14 Hz). Its input is at most 1/2, so
        // the guard at 1 only catches pathological overshoot.
        const i32 blocked = clamp(driven - dc_in + dc_out - (dc_out >> 9), -one, one);
        dc_in = driven;
        dc_out = blocked;

        // Murk: two one-pole lowpasses; never overshoots its input.
        low1 += multiply(blocked - low1, b.murk);
        low2 += multiply(low1 - low2, b.murk);

        // Rot: sample-and-hold, then bit loss on the held sample.
        // Branchless: the clock carries into bit 16 exactly when a new sample is taken.
        hold_phase += b.hold_step;
        const u32 take = 0u - (hold_phase >> 16);
        hold_phase &= 65535u;
        held = static_cast<i32>((static_cast<u32>(held) & ~take) | (static_cast<u32>(low2) & b.rot_mask & take));
        output[i] = clamp(multiply(held, b.level) * 4, -output_peak, output_peak);
    }
    voice.dc_in = dc_in;
    voice.dc_out = dc_out;
    voice.low1 = low1;
    voice.low2 = low2;
    voice.held = held;
    voice.hold_phase = hold_phase;
}

} // namespace daath
