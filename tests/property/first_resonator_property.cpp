#include "resonant/Engine.hpp"
#include "resonant/FirstResonator.hpp"
#include "resonant/Random.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>

int main() {
    resonant::Engine<resonant::FirstResonatorVoice> engine{
        resonant::FirstResonatorVoice{0x4d312d70726f70ULL}};
    if (!engine.prepare({96'000.0, 64, 1, 1})) {
        std::cerr << "M1 property engine failed to prepare\n";
        return 1;
    }

    resonant::Pcg32 rng{0x4d312d7374726573ULL};
    std::array<resonant::Sample, 64> input{};
    std::array<resonant::Sample, 64> output{};
    const resonant::Sample* inputs[1]{input.data()};
    resonant::Sample* outputs[1]{output.data()};

    constexpr std::array<resonant::ParameterId, 7> parameters{
        resonant::FirstResonatorVoice::kTuningHz,
        resonant::FirstResonatorVoice::kDamping,
        resonant::FirstResonatorVoice::kFeedback,
        resonant::FirstResonatorVoice::kNonlinearity,
        resonant::FirstResonatorVoice::kExcitation,
        resonant::FirstResonatorVoice::kTurbulence,
        resonant::FirstResonatorVoice::kInteraction,
    };

    for (std::uint32_t block_index = 0; block_index < 2'000U; ++block_index) {
        for (auto& sample : input) {
            sample = rng.nextSignedFloat() * 0.05F;
        }

        resonant::FixedEventBuffer<16> events;
        for (std::uint32_t event_index = 0; event_index < 4U; ++event_index) {
            const auto sample_offset = event_index * 16U;
            const auto parameter_index = static_cast<std::size_t>(
                rng.nextUInt() % static_cast<std::uint32_t>(parameters.size()));
            const auto target = parameters[parameter_index];
            resonant::Sample value = rng.nextSignedFloat() * 4.0F;
            if (target == resonant::FirstResonatorVoice::kTuningHz) {
                value = 20.0F + rng.nextUnitFloat() * 10'000.0F;
            }
            if (!events.push({sample_offset, resonant::EventType::ParameterChange,
                              target, 0, value, 0.0F})) {
                std::cerr << "M1 property event buffer overflow\n";
                return 1;
            }
        }

        resonant::AudioBlockView block{inputs, outputs, 1, 1, 64};
        const auto status = engine.process(block, events.span());
        if (status != resonant::ProcessStatus::Ok) {
            std::cerr << "M1 randomized process failed at block " << block_index << '\n';
            return 1;
        }
        for (const auto sample : output) {
            if (!std::isfinite(sample) || std::abs(sample) > 1.5001F) {
                std::cerr << "M1 randomized output escaped finite bound\n";
                return 1;
            }
        }
    }

    if (engine.model().resonator().numericalFailure()) {
        std::cerr << "M1 randomized stress recorded numerical failure\n";
        return 1;
    }

    std::cout << "PASS: M1 randomized first-resonator property stress\n";
    return 0;
}
