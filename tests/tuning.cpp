// Pitch and control behaviour: the played note is the saw fundamental, Grave
// adds exactly one octave below, Swarm 0 and Wow 0 sound identical for any
// voice, Rot 0 is a bypass, Murk only darkens and Hex leaves no DC.
#include "../engine.cpp"
#include "check.h"

static const unsigned frames = 689 * opxy::block_frames;  // whole blocks, about one second
static double buffer[frames];

static void tone(double midi, const double* controls, opxy::u32 owner, double* out = buffer) {
    static daath::Voice voice;
    static daath::Shared shared;
    std::memset(&voice, 0, sizeof(voice));
    daath::initialize(voice, shared, owner);
    opxy::i32 parameters[8] = {};
    for (unsigned c = 0; c < 6; ++c) parameters[c] = q31(controls[c]);
    opxy::u32 pitch[opxy::block_frames];
    opxy::i32 output[opxy::block_frames];
    const opxy::u32 inc = increment(midi);
    for (unsigned i = 0; i < opxy::block_frames; ++i) pitch[i] = inc;
    for (unsigned block = 0; block < 100; ++block) daath::render(voice, shared, owner, output, parameters, pitch);
    for (unsigned at = 0; at < frames; at += opxy::block_frames) {
        daath::render(voice, shared, owner, output, parameters, pitch);
        for (unsigned i = 0; i < opxy::block_frames; ++i) out[at + i] = output[i] / static_cast<double>(peak);
    }
}

static double brightness(const double* x) {  // mean |first difference| / mean |x|
    double d = 0, a = 0;
    for (unsigned i = 1; i < frames; ++i) { d += std::fabs(x[i] - x[i - 1]); a += std::fabs(x[i]); }
    return d / a;
}

int main() {
    for (double midi = 24; midi <= 60; midi += 12) {
        const double hz = 440 * std::pow(2.0, (midi - 69) / 12);
        const double plain[6] = {0, 0, 0, 0, 0, 0};
        tone(midi, plain, 1);
        require(magnitude(buffer, frames, hz) > 20 * magnitude(buffer, frames, hz * 1.03),
                "the saw fundamental is not at the played pitch");
        require(magnitude(buffer, frames, hz / 2) < magnitude(buffer, frames, hz) / 30,
                "Grave 0 still sounds the sub octave");
        const double grave[6] = {0, 1, 0, 0, 0, 0};
        tone(midi, grave, 1);
        require(magnitude(buffer, frames, hz / 2) > magnitude(buffer, frames, hz) / 2,
                "Grave does not add the octave below");
    }

    // Swarm 0 is one saw: the ghosts' scattered phases must not be heard.
    static double other[frames];
    const double clean[6] = {0, 0.5, 0.3, 0.4, 0, 0};
    tone(36, clean, 1);
    tone(36, clean, 5, other);
    for (unsigned i = 0; i < frames; ++i) require(buffer[i] == other[i], "Swarm 0 depends on the ghost phases");

    // Rot 0 is a bypass: lowering Rot from a tiny value changes the sound.
    const double rot_zero[6] = {0.35, 0.5, 0, 0.3, 0, 0}, rot_tiny[6] = {0.35, 0.5, 0, 0.3, 0.001, 0};
    tone(36, rot_zero, 1);
    tone(36, rot_tiny, 1, other);
    double difference = 0;
    for (unsigned i = 0; i < frames; ++i) difference += std::fabs(buffer[i] - other[i]);
    require(difference / frames < 0.01, "a tiny Rot already wrecks the signal");

    // Murk only darkens.
    double previous = 1e9;
    for (int m = 0; m <= 8; ++m) {
        const double murky[6] = {0.35, 0.5, 0, m / 8.0, 0, 0};
        tone(36, murky, 1);
        const double b = brightness(buffer);
        require(b <= previous * 1.001, "Murk brightened the sound");
        previous = b;
    }

    // Hex at full adds harmonics and leaves no DC.
    const double hexed[6] = {0.35, 0.5, 1, 0, 0, 0};
    tone(36, hexed, 1);
    double mean = 0;
    for (unsigned i = 0; i < frames; ++i) mean += buffer[i];
    require(std::fabs(mean / frames) < 0.005, "Hex leaves DC");
    return 0;
}
