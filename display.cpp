// Daath's screen: an open grimoire. Each control rewrites the book.
//
//   Swarm  ghost pentagrams split from the sigil and turn apart
//   Grave  the moon on the left page waxes from a sliver to full, turns to a black sun and sinks
//   Hex    the candles flare and the summoning ring grows flames
//   Murk   the parchment darkens until the runes glow white on black pages
//   Rot    runes crumble and fall out, the page edges tear
//   Wow    while a note is held the sigil sways and the rune lines waver
//
// Notes answer on screen: each note flashes the sigil and throws a halo; holding
// a note turns the sigil slowly. An idle screen stays still.
#include "display.h"
#include <opxy/q31.hpp>

namespace daath {
namespace {
using opxy::i32;
using opxy::u32;
using namespace opxy::draw;

enum : u32 {
    ring_node = 0, moon_node = 1, flame_left = 2, flame_right = 3,
    bar_node = 4,
    page_left = 12, page_right = 13,
    star_centre = 14, ghost_a = 15, ghost_b = 16, inner_circle = 17, halo = 18,
    tear_first = 19, tear_count = 4,
    rune_first = 23, rune_columns = 8, rune_rows = 5,
};

// Monochrome palette entries (panel.svg.fragment): page tones 1..6, inks, the
// sigil's four tones from shadow to white, two flame tones and the moon.
enum : u32 {
    ink_black = 0, page_darkest = 1, parchment = 6, ink = 7, faded_ink = 8,
    shadow = 9, sigil = 10, bright = 11, glow = 12, flame = 13, flame_core = 14, bone = 15,
};

const i32 expired = 4096;                       // 4 s, Q10: no pulse
const i32 sigil_x = 337 * unit, sigil_y = 129 * unit;
const i32 moon_x = 142 * unit, moon_y = 78 * unit;
const i32 candle_x[2] = {22 * unit, 458 * unit};
const i32 candle_top = 147 * unit;

inline i32 fraction(i32 value) { return clamp(value, 0, opxy::q31::maximum) >> 21; }  // Q31 -> 0..1023

// Lowercase cents caption: "24c".
inline void cents(char* out, u32 capacity, i32 value, u32 range) {
    const u32 n = (static_cast<u32>(clamp(value, 0, opxy::q31::maximum) >> 16) * range + 16384u) >> 15;
    char text[5];
    u32 at = 0;
    if (n >= 10) text[at++] = static_cast<char>('0' + n / 10 % 10);
    text[at++] = static_cast<char>('0' + n % 10);
    text[at++] = 'c';
    text[at] = 0;
    copy(out, capacity, text);
}

// The summoning ring: a circle with eight flame spikes that rise with Hex,
// swelling briefly on each note.
inline void draw_ring(Frame::path_type& path, i32 hex, i32 pulse) {
    const u32 key = 1u + static_cast<u32>(hex >> 4) + 64u * static_cast<u32>(pulse >> 6);
    if (path.key == key) return;
    static const i32 spike[8] = {1024, 560, 180, 0, -90, 0, 180, 560};
    const i32 h = (hex >> 4) << 4, p = (pulse >> 6) << 6;
    for (u32 i = 0; i < Frame::point_count; ++i) {
        const i32 tenths = static_cast<i32>(i) * 225 / 4;
        const i32 radius = 50 * unit + ((h * 10 * spike[i & 7]) >> 10) + p * 3;
        path.points[i].x = floor_q10(radius * sine(tenths + 900));
        path.points[i].y = floor_q10(radius * sine(tenths));
    }
    path.key = key;
}

// The moon: an outer limb and a terminator, 32 distinct points each. The
// terminator sweeps from a sliver (Grave 0) across to a full disc (Grave 100).
inline void draw_moon(Frame::path_type& path, i32 grave) {
    const u32 key = 1u + static_cast<u32>(grave >> 2);
    if (path.key == key) return;
    const i32 g = (grave >> 2) << 2;
    const i32 bulge = 850 - (g * 1874 >> 10);   // Q10, 0.83 .. -1.0
    const i32 radius = 22;
    for (u32 i = 0; i < 32; ++i) {
        const i32 tenths = -900 + 1800 * static_cast<i32>(2 * i + 1) / 64;
        path.points[i].x = radius * sine(tenths + 900);
        path.points[i].y = radius * sine(tenths);
        const i32 back = 900 - 1800 * static_cast<i32>(2 * i + 1) / 64;
        path.points[32 + i].x = floor_q10(radius * bulge * sine(back + 900));
        path.points[32 + i].y = radius * sine(back);
    }
    path.key = key;
}

// A candle flame: a round base and a tip that leans. Height and lean are
// quantized to quarter pixels, and the key follows them.
inline void draw_flame(Frame::path_type& path, i32 height, i32 lean, i32 width) {
    const i32 h = clamp(height >> 8, 32, 240), l = clamp(lean >> 8, -40, 40), w = clamp(width >> 8, 12, 40);
    const u32 key = 1u + static_cast<u32>(h) * 8192u + static_cast<u32>(l + 64) * 64u + static_cast<u32>(w);
    if (path.key == key) return;
    for (u32 i = 0; i < Frame::point_count; ++i) {
        const i32 tenths = static_cast<i32>(i) * 225 / 4;
        const i32 s = sine(tenths), c = sine(tenths + 900);
        if (c > 0) {
            // Upper half narrows towards the tip and leans with it.
            path.points[i].x = (w * s * (1024 - c / 2) >> 12) + (l * c * c >> 12);
            path.points[i].y = -(h * c >> 2);
        } else {
            path.points[i].x = (w * s) >> 2;
            path.points[i].y = -(w * c >> 2);
        }
    }
    path.key = key;
}

inline u32 rune_hash(u32 k) { return (k * 2654435761u) >> 24; }  // 0..255, fixed per rune
} // namespace

inline void display_reset(DisplayState& state) {
    state.header.reset();
    state.track = 0;
    state.previous_serial = 0;
    state.initialized = 0;
    state.pulse_age = expired;
    state.held_time = 0;
    state.angle = 0;
}

// Only screen work: no audio reads, device calls, allocation or wall-clock API.
inline void display(DisplayState& state, const opxy::draw::Input& input, Frame& frame) {
    // Header: two fixed rows; Hex and Murk are shared by both pages.
    const char* const titles[8] = {"Swarm", "Grave", "Hex", "Murk", "Rot", "Wow", "", ""};
    char values[8][header_capacity] = {};
    cents(values[0], header_capacity, input.slots[0], 40);
    for (u32 i = 1; i < 5; ++i) header_amount(values[i], header_capacity, input.slots[i]);
    cents(values[5], header_capacity, input.slots[5], 45);
    const opxy::u8 pages[2][4] = {{0, 1, 2, 3}, {4, 5, 2, 3}};
    page_header(state.header, input, frame, titles, values, bar_node, pages);

    const i32 swarm = fraction(input.slots[0]), grave = fraction(input.slots[1]);
    const i32 hex = fraction(input.slots[2]), murk = fraction(input.slots[3]);
    const i32 rot = fraction(input.slots[4]), wow = fraction(input.slots[5]);
    const bool still = input.reduced_motion != 0;
    const i32 elapsed = clamp(input.elapsed, 0, 102);   // late frames advance at most 0.1 s

    // Note activity. A reset or track change is a baseline, never a note.
    const bool snapshot = !state.initialized || state.track != input.track;
    if (snapshot || still) {
        state.pulse_age = expired;
    } else {
        state.pulse_age = clamp(state.pulse_age + elapsed, 0, expired);
        if (input.note_serial != state.previous_serial) state.pulse_age = 0;
    }
    state.previous_serial = input.note_serial;
    state.track = input.track;
    state.initialized = 1;
    const bool held = input.held_notes != 0 && input.held_notes <= 128;
    if (held && !still) {
        state.held_time = (state.held_time + elapsed) & 0xfffff;
        state.angle = (state.angle + (elapsed * 60 >> 10)) % 3600;  // 6 degrees a second
    }
    const i32 pulse = clamp(1024 - state.pulse_age * 5 / 2, 0, 1024);  // fades in 0.4 s
    const i32 time = still ? 0 : state.held_time;

    // Wow: a slow sway of the sigil and a waver along the rune lines.
    // 0.7 Hz in tenths. Unsigned: time reaches 2^20, and 2^20 * 2520 needs 32 bits.
    const i32 sway_phase = static_cast<i32>(static_cast<u32>(time) * 2520u >> 10);
    const i32 sway = still ? 0 : (wow * 80 >> 10) * sine(sway_phase) >> 10;

    // Murk: the parchment sinks from shade 6 to the darkest page.
    const u32 page_shade = static_cast<u32>(clamp(6 - (murk * 6 >> 10), page_darkest, parchment));
    place(frame, page_left, 0, 0, unit, page_shade, fill_and_stroke);
    place(frame, page_right, 0, 0, unit, page_shade, fill_and_stroke);
    const u32 rune_shade = page_shade >= 4 ? ink : bright;

    // Rot: tears open along the page edges, one per fifth of the turn.
    for (u32 t = 0; t < tear_count; ++t) {
        if (rot > 160 + static_cast<i32>(t) * 200) place(frame, tear_first + t, 0, 0, unit, ink_black, fill_and_stroke);
        else hide(frame, tear_first + t);
    }

    // Runes: rot drops some and shakes the rest; wow makes the lines waver.
    for (u32 r = 0; r < rune_rows; ++r) {
        for (u32 c = 0; c < rune_columns; ++c) {
            const u32 k = r * rune_columns + c, node = rune_first + k;
            const u32 h = rune_hash(k + 1);
            if (static_cast<i32>(h) < (rot * 140 >> 10)) {   // up to 55% crumble away
                hide(frame, node);
                continue;
            }
            const i32 jitter_x = ((static_cast<i32>(h & 7) - 3) * rot) >> 8;     // up to 3 px
            const i32 jitter_y = ((static_cast<i32>((h >> 3) & 7) - 3) * rot) >> 9;
            const i32 waver = still ? 0 : (wow * 3 * sine(sway_phase * 3 + static_cast<i32>(c) * 450)) >> 10;
            const i32 x = (72 + 19 * static_cast<i32>(c)) * unit + jitter_x * unit / 4;
            const i32 y = (122 + 14 * static_cast<i32>(r)) * unit + jitter_y * unit / 4 + waver;
            place(frame, node, x, y, unit, rune_shade, stroke);
        }
    }

    // Grave: the moon waxes, turns to a black sun past three quarters and sinks.
    // On pages too dark to show a black sun it stays pale.
    draw_moon(frame.paths[moon_node], grave);
    place(frame, moon_node, moon_x, moon_y + grave * 8, unit, grave > 768 && page_shade >= 4 ? ink : bone, stroke);

    // The sigil: halo on each note, the ring and pentagram turn while held.
    const i32 turn_tenths = (still ? 0 : state.angle) + sway;
    if (pulse > 0) {
        place(frame, halo, sigil_x, sigil_y, 700 + ((1024 - pulse) * 560 >> 10), shadow, fill_and_stroke);
    } else {
        hide(frame, halo);
    }
    draw_ring(frame.paths[ring_node], hex, pulse);
    place(frame, ring_node, sigil_x, sigil_y, unit, pulse > 512 ? glow : hex > 512 || page_shade < 4 ? bright : sigil, stroke);
    turn(frame, ring_node, rotation(-turn_tenths / 2));
    place(frame, inner_circle, sigil_x, sigil_y, unit + (pulse >> 4), pulse > 256 || page_shade < 4 ? bright : sigil, stroke);
    place(frame, star_centre, sigil_x, sigil_y, unit,
          pulse > 512 ? glow : pulse > 128 || page_shade < 4 ? bright : sigil, stroke);
    turn(frame, star_centre, rotation(turn_tenths));

    // Swarm: two ghost pentagrams part from the sigil by up to 24 degrees.
    if (swarm < 16) {
        hide(frame, ghost_a);
        hide(frame, ghost_b);
    } else {
        const i32 apart = swarm * 240 >> 10;
        const i32 grow = unit + (swarm * 90 >> 10);
        const u32 shade = page_shade < 4 ? faded_ink : swarm > 512 ? sigil : shadow;
        place(frame, ghost_a, sigil_x, sigil_y, grow, shade, stroke);
        turn(frame, ghost_a, rotation(turn_tenths + apart));
        place(frame, ghost_b, sigil_x, sigil_y, grow, shade, stroke);
        turn(frame, ghost_b, rotation(turn_tenths - apart));
    }

    // Hex: candle flames grow and flare; they flicker only while played.
    const i32 flicker_phase = time * 18;
    for (u32 side = 0; side < 2; ++side) {
        const i32 offset = static_cast<i32>(side) * 1300;
        const i32 live = still ? 0 : (held || pulse > 0 ? 1 : 0);
        const i32 flicker = live * ((300 + hex) * sine(flicker_phase + offset) >> 10) * 3;
        const i32 height = (10 * unit) + hex * 30 + pulse * 10 + flicker;
        const i32 lean = live * ((1024 + hex) * 2 * sine((flicker_phase * 7 >> 3) + offset + 700) >> 10)
                         + (wow * 6 * sine(sway_phase + offset) >> 10) * (still ? 0 : 1);
        const i32 width = 5 * unit + (hex * 2);
        const u32 node = side ? flame_right : flame_left;
        draw_flame(frame.paths[node], height, lean, width);
        place(frame, node, candle_x[side], candle_top, unit, pulse > 300 || hex > 700 ? flame_core : flame, stroke);
    }
}
} // namespace daath
