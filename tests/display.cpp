// The grimoire: every frame valid; mutable outlines keep 64 distinct points,
// enclose area and change their key whenever their points change; reset and
// track changes never fabricate a note; an idle screen stays still; reduced
// motion shows the end state at once.
#include "../display.cpp"
#include "check.h"

static long long area(const daath::Frame::path_type& path) {
    long long twice = 0;
    for (unsigned i = 0; i < daath::Frame::point_count; ++i) {
        const auto& a = path.points[i];
        const auto& b = path.points[(i + 1) % daath::Frame::point_count];
        twice += static_cast<long long>(a.x) * b.y - static_cast<long long>(b.x) * a.y;
    }
    return twice < 0 ? -twice : twice;
}

static void check_outlines(const daath::Frame& frame) {
    for (unsigned p = 0; p < daath::Frame::path_count; ++p) {
        const auto& path = frame.paths[p];
        require(path.key != 0, "outline without a key");
        require(area(path) > 4LL * opxy::draw::unit * opxy::draw::unit, "outline encloses no area");
        for (unsigned i = 0; i < daath::Frame::point_count; ++i)
            for (unsigned j = i + 1; j < daath::Frame::point_count; ++j)
                require(path.points[i].x != path.points[j].x || path.points[i].y != path.points[j].y,
                        "outline repeats a point; the device would merge it");
    }
}

int main() {
    using daath::DisplayState;
    using daath::display;
    using daath::display_reset;
    typedef daath::Frame Frame;
    static DisplayState state;
    static Frame frame, previous;
    opxy::draw::Input input = {};
    input.elapsed = 17;
    input.track = 1;
    input.note_serial = 42;
    for (unsigned i = 0; i < 6; ++i) input.slots[i] = 0x40000000;
    for (unsigned i = 0; i < 4; ++i) { input.parameters[i] = input.slots[i]; input.shown[i] = static_cast<opxy::u8>(i); }

    // First frame after reset: valid, no pulse even with a nonzero serial.
    display_reset(state);
    display(state, input, frame);
    require(opxy::draw::valid(frame), "invalid first frame");
    require(state.pulse_age >= 4096, "reset fabricated a note");
    check_outlines(frame);

    // Idle: nothing held, nothing turned, the frame does not change.
    for (unsigned step = 0; step < 50; ++step) {
        previous = frame;
        display(state, input, frame);
        require(std::memcmp(&previous, &frame, sizeof(frame)) == 0, "idle screen moves");
    }

    // A track change is a baseline, never a note.
    ++input.track;
    ++input.note_serial;
    display(state, input, frame);
    require(state.pulse_age >= 4096, "track change fabricated a note");

    // Random play: controls, notes, holds, late and negative frame times.
    opxy::u32 seed = 7;
    for (unsigned step = 0; step < 20000; ++step) {
        seed = seed * 1664525u + 1013904223u;
        input.elapsed = step % 13 == 0 ? -5 : step % 17 == 0 ? 100000 : 17;
        if (step % 3 == 0)
            for (unsigned i = 0; i < 6; ++i)
                input.slots[i] = static_cast<opxy::i32>((seed ^ (i * 0x9e3779b9u)) & 0x7fffffffu);
        if (step % 19 == 0) ++input.note_serial;
        input.held_notes = (step / 40) & 1 ? 1 + (seed & 3) : 0;
        if (step % 997 == 0) input.held_notes = opxy::draw::notes_unavailable;
        for (unsigned i = 0; i < 4; ++i) input.shown[i] = static_cast<opxy::u8>(step & 64 ? (i < 2 ? i + 4 : i) : i);
        if (step % 2111 == 0) ++input.track;
        input.reduced_motion = step % 500 < 20;
        const Frame before = frame;
        display(state, input, frame);
        require(opxy::draw::valid(frame), "invalid animated frame");
        check_outlines(frame);
        for (unsigned p = 0; p < Frame::path_count; ++p)
            if (before.paths[p].key == frame.paths[p].key)
                require(std::memcmp(&before.paths[p], &frame.paths[p], sizeof(frame.paths[p])) == 0,
                        "points changed without a new key");
    }

    // Reduced motion: a held note and a new note leave the frame unchanged.
    input.reduced_motion = 1;
    input.held_notes = 1;
    display(state, input, frame);
    for (unsigned step = 0; step < 20; ++step) {
        previous = frame;
        ++input.note_serial;
        display(state, input, frame);
        require(std::memcmp(&previous, &frame, sizeof(frame)) == 0, "reduced motion animates");
    }
    return 0;
}
