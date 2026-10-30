// Pitch and control behaviour: the played note is the saw fundamental, Grave
// adds exactly one octave below, Swarm 0 and Wow 0 sound identical for any
// voice, Swarm's upper half adds the fifth and the octave below, Rot 0 is a
// bypass, Murk only darkens, its LFO moves it only with Amount, and Hex
// leaves no DC.
#include "../engine.cpp"
#include "check.h"

static const unsigned frames = 689 * opxy::block_frames;  // whole blocks, about one second
static double buffer[frames];
static double speed = 0.35, amount = 0;  // the Murk LFO, controls 7 and 8

static void tone(double midi, const double* controls, opxy::u32 owner, double* out = buffer) {
    static daath::Voice voice;
    static daath::Shared shared;
    std::memset(&voice, 0, sizeof(voice));
    daath::initialize(voice, shared, owner);
    opxy::i32 parameters[8] = {};
    for (unsigned c = 0; c < 6; ++c) parameters[c] = q31(controls[c]);
    parameters[6] = q31(speed);
    parameters[7] = q31(amount);
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

static double brightness(const double* x, unsigned n = frames) {  // mean |first difference| / mean |x|
    double d = 0, a = 0;
    for (unsigned i = 1; i < n; ++i) { d += std::fabs(x[i] - x[i - 1]); a += std::fabs(x[i]); }
    return d / a;
}

// How far brightness moves across eighth-second windows: 1 for a steady tone.
static double brightness_swing(const double* x) {
    const unsigned window = frames / 8;
    double low = 1e9, high = 0;
    for (unsigned w = 0; w < 8; ++w) {
        const double b = brightness(x + w * window, window);
        low = b < low ? b : low;
        high = b > high ? b : high;
    }
    return high / low;
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

    // Swarm's upper half: a fifth up and an octave down join the played note.
    for (double midi = 36; midi <= 48; midi += 12) {
        const double hz = 440 * std::pow(2.0, (midi - 69) / 12);
        const double detuned[6] = {0.5, 0, 0, 0, 0, 0}, stacked[6] = {1, 0, 0, 0, 0, 0};
        tone(midi, detuned, 1);
        tone(midi, stacked, 1, other);
        require(magnitude(other, frames, hz * 1.5) > 5 * magnitude(buffer, frames, hz * 1.5),
                "Swarm's upper half does not add the fifth");
        require(magnitude(other, frames, hz / 2) > 5 * magnitude(buffer, frames, hz / 2),
                "Swarm's upper half does not add the octave below");
    }

    // The Murk LFO: Amount 0 is silent whatever Speed is; Amount moves the cutoff.
    const double lfo_tone[6] = {0, 0.5, 0, 0.5, 0, 0};  // one clean saw: no Swarm beating
    amount = 0;
    speed = 0;
    tone(36, lfo_tone, 1);
    speed = 1;
    tone(36, lfo_tone, 1, other);
    for (unsigned i = 0; i < frames; ++i) require(buffer[i] == other[i], "Speed changes the sound at Amount 0");
    const double steady = brightness_swing(buffer);  // windows cut the cycle unevenly: about 1.06
    speed = 0.65;  // about 2 Hz
    amount = 1;
    tone(36, lfo_tone, 1);
    require(brightness_swing(buffer) > 1.1 * steady, "the Murk LFO does not move the cutoff");
    speed = 0.35;
    amount = 0;

    // Hex at full adds harmonics and leaves no DC.
    const double hexed[6] = {0.35, 0.5, 1, 0, 0, 0};
    tone(36, hexed, 1);
    double mean = 0;
    for (unsigned i = 0; i < frames; ++i) mean += buffer[i];
    require(std::fabs(mean / frames) < 0.005, "Hex leaves DC");
    return 0;
}
