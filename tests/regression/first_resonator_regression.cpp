#include "resonant/Engine.hpp"
#include "resonant/FirstResonator.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>

namespace {

double renderSignature(resonant::Seed seed, std::uint32_t block_size) {
    resonant::Engine<resonant::FirstResonatorVoice> engine{
        resonant::FirstResonatorVoice{seed}};
    if (!engine.prepare({48'000.0, block_size, 1, 1})) {
        return -1.0;
    }

    std::vector<resonant::Sample> input(block_size, 0.0F);
    std::vector<resonant::Sample> output(block_size, 0.0F);
    constexpr std::uint32_t total_frames = 8'192U;
    std::uint32_t rendered = 0U;
    bool first = true;
    double signature = 0.0;

    while (rendered < total_frames) {
        const auto frames = std::min(block_size, total_frames - rendered);
        const resonant::Sample* inputs[1]{input.data()};
        resonant::Sample* outputs[1]{output.data()};
        resonant::AudioBlockView block{inputs, outputs, 1, 1, frames};
        resonant::FixedEventBuffer<8> events;
        if (first) {
            if (!events.push({0, resonant::EventType::Pitch, 0, 0, 293.6648F, 0.0F}) ||
                !events.push({0, resonant::EventType::Pressure, 0, 0, 0.38F, 0.0F}) ||
                !events.push({0, resonant::EventType::ParameterChange,
                              resonant::FirstResonatorVoice::kTurbulence, 0, 0.76F, 0.0F}) ||
                !events.push({0, resonant::EventType::ParameterChange,
                              resonant::FirstResonatorVoice::kFeedback, 0, 0.44F, 0.0F}) ||
                !events.push({0, resonant::EventType::ParameterChange,
                              resonant::FirstResonatorVoice::kNonlinearity, 0, 0.41F, 0.0F})) {
                return -2.0;
            }
            first = false;
        }

        if (engine.process(block, events.span()) != resonant::ProcessStatus::Ok) {
            return -3.0;
        }
        for (std::uint32_t frame = 0; frame < frames; ++frame) {
            const auto absolute_index = static_cast<std::size_t>(rendered + frame + 1U);
            signature += static_cast<double>(output[frame]) *
                         static_cast<double>(absolute_index);
        }
        rendered += frames;
    }
    return signature;
}

} // namespace

int main() {
    const auto a = renderSignature(777, 32U);
    const auto b = renderSignature(777, 32U);
    const auto c = renderSignature(778, 32U);
    const auto block_one = renderSignature(777, 1U);
    const auto block_sixty_four = renderSignature(777, 64U);

    if (!std::isfinite(a) || a == 0.0 || a != b || a == c) {
        std::cerr << "M1 deterministic regression failed: "
                  << a << ' ' << b << ' ' << c << '\n';
        return 1;
    }
    if (block_one != block_sixty_four) {
        std::cerr << "M1 block-size invariant regression failed: "
                  << block_one << ' ' << block_sixty_four << '\n';
        return 1;
    }

    std::cout << "PASS: M1 deterministic regression signature=" << a
              << " block-invariant=" << block_one << '\n';
    return 0;
}
