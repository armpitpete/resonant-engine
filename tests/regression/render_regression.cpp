#include "resonant/Engine.hpp"
#include "resonant/Random.hpp"
#include "resonant/ReferenceFeedbackProbe.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>

static double renderSignature(resonant::Seed seed) {
    resonant::Engine<resonant::ReferenceFeedbackProbe> engine;
    if (!engine.prepare({48'000.0, 32, 1, 1})) {
        return -1.0;
    }
    resonant::Pcg32 rng{seed};
    double signature = 0.0;
    std::array<resonant::Sample, 32> input{};
    std::array<resonant::Sample, 32> output{};
    const resonant::Sample* inputs[1]{input.data()};
    resonant::Sample* outputs[1]{output.data()};
    for (int block_index = 0; block_index < 16; ++block_index) {
        for (auto& sample : input) {
            sample = rng.nextSignedFloat() * 0.01F;
        }
        resonant::FixedEventBuffer<4> events;
        if (block_index == 0) {
            if (!events.push({0, resonant::EventType::ParameterChange,
                              resonant::ReferenceFeedbackProbe::kExcitation, 0, 0.12F, 0.0F}) ||
                !events.push({0, resonant::EventType::ParameterChange,
                              resonant::ReferenceFeedbackProbe::kFeedback, 0, 0.25F, 0.0F})) {
                return -3.0;
            }
        }
        resonant::AudioBlockView view{inputs, outputs, 1, 1, 32};
        if (engine.process(view, events.span()) != resonant::ProcessStatus::Ok) {
            return -2.0;
        }
        for (std::size_t i = 0; i < output.size(); ++i) {
            const auto absolute_index = i + 1U + static_cast<std::size_t>(block_index) * output.size();
            signature += static_cast<double>(output[i]) * static_cast<double>(absolute_index);
        }
    }
    return signature;
}

int main() {
    const auto a = renderSignature(777);
    const auto b = renderSignature(777);
    const auto c = renderSignature(778);
    if (!std::isfinite(a) || a != b || a == c) {
        std::cerr << "deterministic render regression failed: " << a << ' ' << b << ' ' << c << '\n';
        return 1;
    }
    std::cout << "PASS: deterministic render regression signature=" << a << '\n';
    return 0;
}
