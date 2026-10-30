// The seven shipped presets: audible, bounded, free of DC, with eight voices,
// stolen voices, pitch bends and control moves. Keep in step with definition.json.
#include "../engine.cpp"
#include "check.h"

struct Preset { const char* name; double values[8]; };
static const unsigned preset_count = 7;
static const Preset presets[preset_count] = {
    {"Sigil", {0.175, 0.5, 0.0, 0.3, 0.0, 0.0, 0.35, 0.0}},
    {"Crypt", {0.3, 0.8, 0.35, 0.55, 0.0, 0.25, 0.35, 0.0}},
    {"Coven", {0.425, 0.45, 0.25, 0.4, 0.15, 0.35, 0.35, 0.0}},
    {"Screwed", {0.25, 0.7, 0.2, 0.65, 0.0, 0.7, 0.35, 0.0}},
    {"Hexed", {0.225, 0.6, 0.85, 0.35, 0.0, 0.1, 0.35, 0.0}},
    {"Rotten", {0.2, 0.6, 0.4, 0.3, 0.8, 0.2, 0.35, 0.0}},
    {"Seance", {0.8, 0.6, 0.3, 0.55, 0.0, 0.15, 0.6505, 0.5}},
};

int main() {
    static daath::Shared shared;
    static daath::Voice voices[8];
    opxy::i32 parameters[8], output[opxy::block_frames];
    opxy::u32 pitch[opxy::block_frames];
    for (unsigned p = 0; p < preset_count; ++p) {
        std::memset(voices, 0, sizeof(voices));
        for (unsigned v = 0; v < 8; ++v) daath::initialize(voices[v], shared, v + 1);
        double sum = 0, energy = 0;
        unsigned count = 0;
        for (unsigned block = 0; block < 1400; ++block) {
            for (unsigned c = 0; c < 8; ++c) parameters[c] = q31(presets[p].values[c]);
            if (block >= 600 && block < 700) parameters[2] = 0x7fffffff;  // a Hex sweep mid-note
            if (block % 175 == 0) {
                // A stolen voice is initialised again for its new owner.
                const unsigned v = (block / 175) & 7;
                daath::initialize(voices[v], shared, 9 + block);
                daath::start(voices[v], shared, 9 + block);
            }
            for (unsigned v = 0; v < 8; ++v) {
                const opxy::u32 base = increment(24 + 3 * v);
                for (unsigned i = 0; i < opxy::block_frames; ++i)
                    pitch[i] = block >= 1000 ? base + ((block * 131u + i * 977u) & 0x3ffffu) : base;
                daath::render(voices[v], shared, v + 1, output, parameters, pitch);
                for (unsigned i = 0; i < opxy::block_frames; ++i) {
                    require(output[i] >= -peak && output[i] <= peak, "preset exceeds the declared peak");
                    if (block > 50) {
                        sum += output[i];
                        energy += static_cast<double>(output[i]) * output[i];
                        ++count;
                    }
                }
            }
        }
        const double rms = std::sqrt(energy / count) / peak, dc = sum / count / peak;
        std::printf("%-8s rms %.3f dc %+.4f\n", presets[p].name, rms, dc);
        require(rms > 0.08, "preset too quiet");
        require(dc < 0.01 && dc > -0.01, "preset carries DC");
    }
    return 0;
}
