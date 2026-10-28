#pragma once
#include <opxy/audio.hpp>

namespace daath {
using opxy::i32;
using opxy::u32;

const u32 saw_count = 5;
const u32 control_count = 6;

// Only sound state. The SDK owns firmware headers, slot padding and allocation.
struct Voice {
    u32 phase[saw_count];   // the swarm: one centre saw and four detuned ghosts
    u32 sub_phase;          // Grave: a sine one octave below
    u32 wow_phase[2];       // two slow, unrelated tape-wow rates
    i32 low1, low2;         // Murk: two cascaded one-pole lowpasses
    i32 dc_in, dc_out;      // DC blocker after the asymmetric Hex drive
    u32 hold_phase;         // Rot: sample-and-hold clock, Q16
    i32 held;               // Rot: the held sample
    i32 control[control_count];  // block-smoothed controls, Q31
    u32 seed;               // per-voice phase scatter
    u32 initialized;
};
// Voices share nothing.
struct Shared {};
}
