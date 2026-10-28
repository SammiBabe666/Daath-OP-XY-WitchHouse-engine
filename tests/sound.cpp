// Every control corner, restarts and pitch extremes stay bounded, audible and finite.
#include "../engine.cpp"
#include "check.h"

int main() {
    static daath::Voice voice;
    static daath::Shared shared;
    opxy::i32 parameters[8] = {};
    opxy::u32 pitch[opxy::block_frames];
    opxy::i32 output[opxy::block_frames];
    const double notes[] = {12, 24, 36, 48, 72, 96, 120};
    for (unsigned corner = 0; corner < 64; ++corner) {
        for (unsigned c = 0; c < 6; ++c) parameters[c] = (corner >> c) & 1 ? 0x7fffffff : 0;
        for (unsigned n = 0; n < sizeof(notes) / sizeof(notes[0]); ++n) {
            std::memset(&voice, 0, sizeof(voice));
            daath::initialize(voice, shared, 1 + corner);
            daath::start(voice, shared, 1 + corner);
            const opxy::u32 inc = increment(notes[n]);
            bool sounding = false;
            for (unsigned block = 0; block < 120; ++block) {
                for (unsigned i = 0; i < opxy::block_frames; ++i) pitch[i] = inc;
                if (block == 60) daath::start(voice, shared, 1 + corner);
                daath::render(voice, shared, 1 + corner, output, parameters, pitch);
                for (unsigned i = 0; i < opxy::block_frames; ++i) {
                    require(output[i] >= -peak && output[i] <= peak, "output above the declared peak");
                    sounding = sounding || output[i] > peak / 64 || output[i] < -peak / 64;
                }
            }
            require(sounding, "a corner is silent");
        }
    }
    // Hostile control words: negative and full-scale, changed every block.
    std::memset(&voice, 0, sizeof(voice));
    daath::initialize(voice, shared, 7);
    opxy::u32 seed = 1;
    for (unsigned block = 0; block < 4000; ++block) {
        for (unsigned c = 0; c < 8; ++c) parameters[c] = static_cast<opxy::i32>(seed = seed * 1664525u + 1013904223u);
        for (unsigned i = 0; i < opxy::block_frames; ++i) pitch[i] = (seed >> 3) + i * 997u;
        daath::render(voice, shared, 7, output, parameters, pitch);
        for (unsigned i = 0; i < opxy::block_frames; ++i)
            require(output[i] >= -peak && output[i] <= peak, "random controls exceed the peak");
    }
    return 0;
}
