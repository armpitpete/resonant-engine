#include "hosts/vst3/CoreAdapter.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <span>
#include <string_view>

namespace {

int failures = 0;

void check(bool condition, std::string_view name) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << name << '\n';
    }
}

void testPrepareAndSilence() {
    resonant::vst3::BreathPipeCoreAdapter adapter;
    check(!adapter.prepare(1.0, 128U), "reject invalid sample rate");
    check(adapter.prepare(48'000.0, 128U, 0U, 2U), "prepare VST3 core adapter");
    check(adapter.spec().output_channels == 2U, "adapter owns stereo output spec");

    std::array<resonant::Sample, 128> left{};
    std::array<resonant::Sample, 128> right{};
    resonant::Sample* outputs[2]{left.data(), right.data()};

    check(adapter.process(nullptr, 0U, outputs, 2U, 128U) ==
              resonant::ProcessStatus::Ok,
          "silent block processes");
    check(std::all_of(left.begin(), left.end(),
                      [](auto sample) { return sample == 0.0F; }) &&
              std::all_of(right.begin(), right.end(),
                          [](auto sample) { return sample == 0.0F; }),
          "fresh adapter is exactly silent");
}

void testEventsAndStereoTranslation() {
    resonant::vst3::BreathPipeCoreAdapter adapter;
    check(adapter.prepare(48'000.0, 128U, 0U, 2U), "prepare excitation adapter");

    std::array<resonant::Sample, 128> left{};
    std::array<resonant::Sample, 128> right{};
    resonant::Sample* outputs[2]{left.data(), right.data()};

    resonant::FixedEventBuffer<8> events;
    (void)events.push({0U, resonant::EventType::Pitch, 0U, 0U, 220.0F, 0.0F});
    (void)events.push({0U, resonant::EventType::Pressure, 0U, 0U, 0.65F, 0.0F});
    (void)events.push({0U, resonant::EventType::ParameterChange,
                       resonant::BreathPipeVoice::kTurbulence, 0U, 0.15F, 0.0F});
    (void)events.push({0U, resonant::EventType::NoteOn, 0U, 1U, 0.8F, 0.0F});

    double absolute_sum = 0.0;
    for (std::uint32_t block = 0; block < 80U; ++block) {
        const auto scheduled = block == 0U ? events.span()
                                           : std::span<const resonant::Event>{};
        check(adapter.process(nullptr, 0U, outputs, 2U, 128U, scheduled) ==
                  resonant::ProcessStatus::Ok,
              "event-driven adapter block remains finite");
        for (std::size_t frame = 0; frame < left.size(); ++frame) {
            absolute_sum += std::abs(static_cast<double>(left[frame]));
            check(left[frame] == right[frame], "mono model duplicates identically to stereo");
        }
    }
    check(absolute_sum > 1.0e-4, "adapter exposes real Breath Pipe output");

    check(adapter.reset(), "adapter reset");
    check(adapter.process(nullptr, 0U, outputs, 2U, 129U) ==
              resonant::ProcessStatus::InvalidContext,
          "adapter rejects host block larger than prepared maximum");
}

} // namespace

int main() {
    testPrepareAndSilence();
    testEventsAndStereoTranslation();
    if (failures != 0) {
        std::cerr << failures << " M4 VST3 adapter test(s) failed\n";
        return 1;
    }
    std::cout << "PASS: M4 VST3 core adapter lifecycle and audio translation\n";
    return 0;
}
