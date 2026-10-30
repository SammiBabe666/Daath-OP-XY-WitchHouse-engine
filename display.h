#pragma once
#include <opxy/page_header.hpp>

namespace daath {
// Matches resources.display in definition.json: paths, points, nodes, texts, palette.
// Four outlines (summoning ring, moon, two flames), the two-page header's eight
// bars, the open book and its sigils, page tears and forty runes.
using Frame = opxy::draw::Frame<4, 64, 63, 16, 16>;
struct DisplayState {
    opxy::draw::PageHeader header;   // 20 bytes
    opxy::u32 track;
    opxy::u32 previous_serial;
    opxy::u32 initialized;
    opxy::i32 pulse_age;             // Q10 seconds since the last note, saturating
    opxy::i32 held_time;             // Q10 seconds accumulated while notes are held
    opxy::i32 angle;                 // sigil turn in tenths of a degree, while held
    opxy::i32 lfo;                   // Murk LFO's phase in tenths of a degree, while held
};
}
