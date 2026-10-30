// Daath's exact output: a fixed workload of random and extreme controls,
// Swarm 0, pitch steps and bends, hashed. Integer DSP makes it bit-exact on
// every compiler. Recorded when Swarm gained its power stack and Murk its LFO
// (the workload sets all eight controls; release 1.0's presets render
// bit-for-bit as before); any further change to the sound fails here until it
// is recorded again.
#include "../engine.cpp"
#include "check.h"

int main() {
    using namespace daath;
    unsigned seed = 12345, hash = 2166136261u;
    static Voice voice;
    Shared shared;
    for (int scene = 0; scene < 300; ++scene) {
        initialize(voice, shared, static_cast<u32>(scene % 9));
        i32 p[8];
        for (int c = 0; c < 8; ++c) {
            seed = seed * 1664525u + 1013904223u;
            p[c] = scene % 5 == 0 ? static_cast<i32>(seed & 0x7fffffff) * static_cast<i32>((seed >> 3) & 1)
                                  : static_cast<i32>(seed >> 1);
        }
        if (scene % 7 == 0)
            for (int c = 0; c < 6; ++c) p[c] = (c * 17 + scene) % 3 == 0 ? 0 : 0x7fffffff;
        if (scene % 11 == 0) p[2] = -5;
        const u32 base = 2000000u + static_cast<u32>(scene * 2654435761u % 400000000u);
        for (int block = 0; block < 60; ++block) {
            u32 pitch[64];
            i32 out[64];
            for (int i = 0; i < 64; ++i)
                pitch[i] = base + (scene % 3 == 0 ? static_cast<u32>(i * block * 1000) : 0u) +
                           (scene % 13 == 0 ? static_cast<u32>((i & 1) * 50000000u) : 0u);
            if (block % 20 == 10)
                for (int c = 0; c < 6; ++c) p[c] ^= 0x15555555;
            render(voice, shared, 0, out, p, pitch);
            for (int i = 0; i < 64; ++i) {
                hash ^= static_cast<unsigned>(out[i]);
                hash *= 16777619u;
            }
        }
    }
    require(hash == 0xc550d09du, "render output changed");
    return 0;
}
