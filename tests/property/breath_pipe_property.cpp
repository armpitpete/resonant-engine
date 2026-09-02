#include "resonant/BreathPipe.hpp"
#include "resonant/Engine.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <span>
#include <string_view>
#include <vector>

namespace {

int failures = 0;

void check(bool condition, std::string_view name) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << name << '\n';
    }
}

[[nodiscard]] std::vector<resonant::Sample>
render(resonant::Seed seed, double sample_rate, std::uint32_t block_size,
       std::uint32_t total_frames, float interaction = 0.70F) {
    resonant::Engine<resonant::BreathPipeVoice> engine{resonant::BreathPipeVoice{seed}};
    if (!engine.prepare({sample_rate, block_size, 1, 1})) {
        return {};
    }
    std::vector<resonant::Sample> input(block_size, 0.0F);
    std::vector<resonant::Sample> output(block_size, 0.0F);
    std::vector<resonant::Sample> result;
    result.reserve(total_frames);
    bool first = true;
    std::uint32_t rendered = 0U;
    while (rendered < total_frames) {
        const auto frames = std::min(block_size, total_frames - rendered);
        const resonant::Sample* inputs[1]{input.data()};
        resonant::Sample* outputs[1]{output.data()};
        resonant::AudioBlockView block{inputs, outputs, 1, 1, frames};
        resonant::FixedEventBuffer<8> events;
        if (first) {
            (void)events.push({0, resonant::EventType::Pitch, 0, 0, 329.6276F, 0.0F});
            (void)events.push({0, resonant::EventType::Pressure, 0, 0, 0.64F, 0.0F});
            (void)events.push({0, resonant::EventType::ParameterChange,
                               resonant::BreathPipeVoice::kTurbulence, 0, 0.31F, 0.0F});
            (void)events.push({0, resonant::EventType::ParameterChange,
                               resonant::BreathPipeVoice::kInteraction, 0, interaction, 0.0F});
            (void)events.push({0, resonant::EventType::ParameterChange,
                               resonant::BreathPipeVoice::kRegeneration, 0, 0.42F, 0.0F});
            (void)events.push({0, resonant::EventType::ParameterChange,
                               resonant::BreathPipeVoice::kNonlinearDrive, 0, 0.56F, 0.0F});
            first = false;
        }
        const auto status = engine.process(block, events.span());
        if (status != resonant::ProcessStatus::Ok) {
            return {};
        }
        result.insert(result.end(), output.begin(), output.begin() + frames);
        rendered += frames;
    }
    return result;
}

void testSeedDeterminismAndBlockInvariance() {
    const auto a = render(777, 48'000.0, 32, 48'000);
    const auto b = render(777, 48'000.0, 128, 48'000);
    const auto c = render(778, 48'000.0, 128, 48'000);
    check(!a.empty() && a.size() == b.size() && b.size() == c.size(),
          "Breath Pipe property renders have matching size");
    if (a.size() != b.size() || b.size() != c.size() || a.empty()) {
        return;
    }
    double max_block_difference = 0.0;
    double seed_difference = 0.0;
    for (std::size_t index = 0; index < a.size(); ++index) {
        max_block_difference = std::max(
            max_block_difference,
            std::abs(static_cast<double>(a[index]) - static_cast<double>(b[index])));
        seed_difference += std::abs(static_cast<double>(b[index]) -
                                    static_cast<double>(c[index]));
    }
    check(max_block_difference <= 1.0e-6,
          "Breath Pipe output is invariant to Host block segmentation");
    check(seed_difference > 1.0e-3,
          "different Breath Pipe seeds produce different turbulence trajectories");
}

void testResetDeterminism() {
    resonant::Engine<resonant::BreathPipeVoice> engine{resonant::BreathPipeVoice{555}};
    check(engine.prepare({48'000.0, 64, 1, 1}), "reset determinism prepare");
    std::vector<resonant::Sample> input(64, 0.0F);
    std::vector<resonant::Sample> output(64, 0.0F);
    const resonant::Sample* inputs[1]{input.data()};
    resonant::Sample* outputs[1]{output.data()};
    resonant::AudioBlockView block{inputs, outputs, 1, 1, 64};

    auto pass = [&]() {
        std::vector<resonant::Sample> captured;
        captured.reserve(8'192U);
        for (std::uint32_t block_index = 0; block_index < 128U; ++block_index) {
            resonant::FixedEventBuffer<8> events;
            if (block_index == 0U) {
                (void)events.push({0, resonant::EventType::Pitch, 0, 0, 261.6256F, 0.0F});
                (void)events.push({0, resonant::EventType::Pressure, 0, 0, 0.58F, 0.0F});
                (void)events.push({0, resonant::EventType::ParameterChange,
                                   resonant::BreathPipeVoice::kTurbulence, 0, 0.40F, 0.0F});
            }
            if (engine.process(block, events.span()) != resonant::ProcessStatus::Ok) {
                return std::vector<resonant::Sample>{};
            }
            captured.insert(captured.end(), output.begin(), output.end());
        }
        return captured;
    };

    const auto first = pass();
    check(engine.reset(), "Breath Pipe reset succeeds");
    const auto second = pass();
    check(first == second, "Breath Pipe reset restores deterministic model and RNG state");
}

void testSampleRateQualitativeConsistency() {
    for (const auto sample_rate : {44'100.0, 48'000.0, 96'000.0}) {
        const auto frames = static_cast<std::uint32_t>(sample_rate * 0.5);
        const auto signal = render(777, sample_rate, 128, frames);
        check(!signal.empty(), "Breath Pipe renders at required sample rate");
        check(std::all_of(signal.begin(), signal.end(),
                          [](resonant::Sample sample) { return std::isfinite(sample); }),
              "Breath Pipe required-rate render remains finite");
        const auto peak = signal.empty()
                              ? 0.0F
                              : *std::max_element(signal.begin(), signal.end(),
                                  [](resonant::Sample a, resonant::Sample b) {
                                      return std::abs(a) < std::abs(b);
                                  });
        check(std::abs(peak) > 1.0e-5F,
              "Breath Pipe remains audibly active across required sample rates");
    }
}

} // namespace

int main() {
    testSeedDeterminismAndBlockInvariance();
    testResetDeterminism();
    testSampleRateQualitativeConsistency();
    if (failures != 0) {
        std::cerr << failures << " Breath Pipe property test(s) failed\n";
        return 1;
    }
    std::cout << "PASS: M3 Breath Pipe deterministic properties\n";
    return 0;
}
