#include "resonant/BreathPipe.hpp"
#include "resonant/Engine.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <span>
#include <string_view>
#include <vector>

namespace {

using Voice = resonant::BreathPipeVoice;
constexpr double kSampleRate = 48'000.0;
constexpr std::uint32_t kBlockSize = 128U;
constexpr std::uint32_t kWarmupFrames = 4'800U;
constexpr std::uint32_t kMeasureFrames = 19'200U;

struct RenderStats {
    bool ok{false};
    double rms{0.0};
    double peak{0.0};
    std::vector<float> samples{};
};

struct Comparison {
    double normalized_difference{0.0};
    double rms_ratio_db{0.0};
};

int failures = 0;
[[nodiscard]] resonant::FixedEventBuffer<>
baseEvents(resonant::ParameterId varied, float value) {
    resonant::FixedEventBuffer<> events;
    const std::array<resonant::Event, 11> source{{
        {0, resonant::EventType::Pitch, 0, 0, 261.6256F, 0.0F},
        {0, resonant::EventType::Pressure, 0, 0, 0.65F, 0.0F},
        {0, resonant::EventType::NoteOn, 0, 1, 0.65F, 0.0F},
        {0, resonant::EventType::ParameterChange, Voice::kTurbulence, 0, 0.25F, 0.0F},
        {0, resonant::EventType::ParameterChange, Voice::kInteraction, 0, 0.55F, 0.0F},
        {0, resonant::EventType::ParameterChange, Voice::kDamping, 0, 0.12F, 0.0F},
        {0, resonant::EventType::ParameterChange, Voice::kRegeneration, 0, 0.18F, 0.0F},
        {0, resonant::EventType::ParameterChange, Voice::kFeedbackColor, 0, 0.20F, 0.0F},
        {0, resonant::EventType::ParameterChange, Voice::kNonlinearDrive, 0, 0.30F, 0.0F},
        {0, resonant::EventType::ParameterChange, Voice::kTimbre, 0, 0.25F, 0.0F},
        {0, resonant::EventType::ParameterChange, varied, 0, value, 0.0F},
    }};
    for (const auto& event : source) {
        (void)events.push(event);
    }
    return events;
}

[[nodiscard]] RenderStats render(resonant::ParameterId varied, float value) {
    resonant::Engine<Voice> engine{Voice{0x5a17U}};
    RenderStats stats;
    if (!engine.prepare({kSampleRate, kBlockSize, 1, 1})) {
        return stats;
    }

    std::array<float, kBlockSize> input{};
    std::array<float, kBlockSize> output{};
    const float* inputs[1]{input.data()};
    float* outputs[1]{output.data()};
    resonant::AudioBlockView block{inputs, outputs, 1, 1, kBlockSize};
    const auto events = baseEvents(varied, value);

    stats.samples.reserve(kMeasureFrames);
    const auto total_frames = kWarmupFrames + kMeasureFrames;
    double sum_squares = 0.0;

    for (std::uint32_t start = 0; start < total_frames; start += kBlockSize) {
        const auto scheduled = start == 0U
                                   ? events.span()
                                   : std::span<const resonant::Event>{};
        const auto status = engine.process(block, scheduled);
        if (status != resonant::ProcessStatus::Ok) {
            std::cerr << "render failed for parameter " << varied
                      << " at value " << value
                      << " with status " << static_cast<int>(status) << '\n';
            return stats;
        }
        for (std::uint32_t i = 0; i < kBlockSize && start + i < total_frames; ++i) {
            if (start + i < kWarmupFrames) {
                continue;
            }
            const auto sample = static_cast<double>(output[i]);
            stats.samples.push_back(output[i]);
            sum_squares += sample * sample;
            stats.peak = std::max(stats.peak, std::abs(sample));
        }
    }
    if (!stats.samples.empty()) {
        stats.rms = std::sqrt(sum_squares / static_cast<double>(stats.samples.size()));
        stats.ok = true;
    }
    return stats;
}

[[nodiscard]] Comparison compare(const RenderStats& a, const RenderStats& b) {
    Comparison result;
    const auto count = std::min(a.samples.size(), b.samples.size());
    if (!a.ok || !b.ok || count == 0U) {
        return result;
    }

    double diff_squares = 0.0;
    for (std::size_t i = 0; i < count; ++i) {
        const auto delta = static_cast<double>(a.samples[i]) -
                           static_cast<double>(b.samples[i]);
        diff_squares += delta * delta;
    }
    const auto diff_rms = std::sqrt(diff_squares / static_cast<double>(count));
    const auto reference = std::max({a.rms, b.rms, 1.0e-12});
    result.normalized_difference = diff_rms / reference;
    result.rms_ratio_db = 20.0 * std::log10(std::max(b.rms, 1.0e-12) /
                                            std::max(a.rms, 1.0e-12));
    return result;
}

void checkLeverage(std::string_view name, resonant::ParameterId id, float low,
                   float high, double minimum_difference) {
    const auto low_render = render(id, low);
    const auto high_render = render(id, high);
    const auto comparison = compare(low_render, high_render);
    const auto passed = low_render.ok && high_render.ok &&
                        comparison.normalized_difference >= minimum_difference;
    if (!passed) {
        ++failures;
        std::cerr << "FAIL: " << name << " control leverage regression\n";
    }
    std::cout << name
              << " ok=" << (low_render.ok && high_render.ok ? "yes" : "no")
              << " low_rms=" << low_render.rms
              << " high_rms=" << high_render.rms
              << " low_peak=" << low_render.peak
              << " high_peak=" << high_render.peak
              << " normalized_diff=" << comparison.normalized_difference
              << " minimum_diff=" << minimum_difference
              << " rms_delta_db=" << comparison.rms_ratio_db
              << '\n';
}

} // namespace

int main() {
    // These floors are deterministic regression guards, not psychoacoustic
    // audibility claims. Human H04 remains authoritative for audibility.
    checkLeverage("Pressure", Voice::kPressure, 0.25F, 0.85F, 0.30);
    checkLeverage("Turbulence", Voice::kTurbulence, 0.05F, 0.85F, 0.50);
    checkLeverage("Interaction", Voice::kInteraction, 0.15F, 0.90F, 0.08);
    checkLeverage("Damping", Voice::kDamping, 0.03F, 0.65F, 0.35);
    checkLeverage("Regeneration", Voice::kRegeneration, 0.05F, 0.80F, 0.20);
    checkLeverage("FeedbackColour", Voice::kFeedbackColor, 0.05F, 0.90F, 0.05);
    checkLeverage("NonlinearDrive", Voice::kNonlinearDrive, 0.05F, 0.90F, 0.50);
    checkLeverage("Timbre", Voice::kTimbre, 0.00F, 1.00F, 0.01);
    if (failures != 0) {
        std::cerr << failures << " Breath Pipe control-leverage regression(s) failed\n";
        return 1;
    }
    std::cout << "PASS: Breath Pipe control-leverage regression\n";
    return 0;
}
