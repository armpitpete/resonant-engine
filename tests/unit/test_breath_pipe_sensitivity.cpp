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
    double roughness{0.0};
    double lag1_correlation{0.0};
    double crest_db{0.0};
    double overblow{0.0};
    double mode_radius_0{0.0};
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
        if (stats.samples.size() > 1U) {
            double delta_squares = 0.0;
            double lag_product = 0.0;
            double lag_energy_a = 0.0;
            double lag_energy_b = 0.0;
            for (std::size_t i = 1U; i < stats.samples.size(); ++i) {
                const auto a = static_cast<double>(stats.samples[i - 1U]);
                const auto b = static_cast<double>(stats.samples[i]);
                const auto delta = b - a;
                delta_squares += delta * delta;
                lag_product += a * b;
                lag_energy_a += a * a;
                lag_energy_b += b * b;
            }
            const auto delta_rms = std::sqrt(
                delta_squares / static_cast<double>(stats.samples.size() - 1U));
            stats.roughness = delta_rms / std::max(stats.rms, 1.0e-12);
            stats.lag1_correlation = lag_product /
                std::sqrt(std::max(lag_energy_a * lag_energy_b, 1.0e-24));
        }
        stats.crest_db = 20.0 * std::log10(
            std::max(stats.peak, 1.0e-12) / std::max(stats.rms, 1.0e-12));
        stats.overblow = engine.model().resonator().overblowAmount();
        stats.mode_radius_0 = engine.model().resonator().modeRadius(0);
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

void checkStablePipeMacroIdentity() {
    constexpr resonant::Seed seed = 0x5a17U;
    const resonant::ProcessSpec spec{kSampleRate, kBlockSize, 1, 1};
    Voice voice{seed};
    resonant::BreathPipeExciter exciter;
    resonant::BreathPipeModalResonator resonator;
    const std::array<float, 10> stable_values{{
        220.0F, 0.55F, 0.18F, 0.72F, 0.08F,
        0.38F, 0.25F, 0.10F, 0.0F, 0.25F,
    }};
    const auto prepared = voice.prepare(spec) && exciter.prepare(spec, seed) &&
                          resonator.prepare(spec) &&
                          voice.restorePersistentState(seed, stable_values);
    if (!prepared) {
        ++failures;
        std::cerr << "FAIL: Stable Pipe identity fixture prepares\n";
        return;
    }

    voice.handleEvent({0, resonant::EventType::NoteOn, 0, 1, 0.55F, 0.0F});
    float returned = 0.0F;
    double maximum_error = 0.0;
    for (std::uint32_t frame = 0; frame < 24'000U; ++frame) {
        float voice_sample = 0.0F;
        if (!voice.processSample({}, std::span<float>{&voice_sample, 1U})) {
            ++failures;
            std::cerr << "FAIL: Stable Pipe voice render remains healthy\n";
            return;
        }
        const auto excitation = exciter.processSample({
            0.0F, 0.0F, 0.55F, 0.18F, 0.72F, returned, 0.10F,
            frame == 0U ? 0.55F : 0.0F,
        });
        const auto direct = resonator.processSample(
            excitation, {220.0F, 0.08F, 0.55F, 0.72F, 0.38F,
                         0.25F, 0.10F, 0.25F});
        returned = direct.feedback_tap;
        maximum_error = std::max(maximum_error,
                                 std::abs(static_cast<double>(voice_sample) -
                                          static_cast<double>(direct.sample)));
    }
    if (maximum_error > 1.0e-7) {
        ++failures;
        std::cerr << "FAIL: M3.7 macros alter canonical Stable Pipe identity\n";
    }
    std::cout << "StablePipe macro_identity_max_error=" << maximum_error << '\n';
}

double checkProgression(std::string_view name, resonant::ParameterId id,
                        const std::array<float, 5>& values,
                        double minimum_step_difference) {
    std::array<RenderStats, 5> renders{};
    for (std::size_t i = 0U; i < values.size(); ++i) {
        renders[i] = render(id, values[i]);
    }
    auto minimum_observed = 1.0e9;
    std::cout << name << " progression";
    for (std::size_t i = 1U; i < renders.size(); ++i) {
        const auto comparison = compare(renders[i - 1U], renders[i]);
        minimum_observed = std::min(minimum_observed, comparison.normalized_difference);
        std::cout << " step" << i << "=" << comparison.normalized_difference;
        if (!renders[i - 1U].ok || !renders[i].ok ||
            comparison.normalized_difference < minimum_step_difference) {
            ++failures;
            std::cerr << "FAIL: " << name << " has a macro dead zone at step "
                      << i << '\n';
        }
    }
    std::cout << " minimum=" << minimum_observed
              << " required=" << minimum_step_difference << '\n';
    return minimum_observed;
}

void checkPrimaryMacroContracts() {
    constexpr double kMinimumTravelStep = 0.10;
    (void)checkProgression("Pressure", Voice::kPressure,
                           {0.15F, 0.35F, 0.55F, 0.75F, 0.95F},
                           kMinimumTravelStep);
    (void)checkProgression("Turbulence", Voice::kTurbulence,
                           {0.00F, 0.18F, 0.40F, 0.70F, 1.00F},
                           kMinimumTravelStep);
    (void)checkProgression("Damping", Voice::kDamping,
                           {0.00F, 0.08F, 0.30F, 0.60F, 1.00F},
                           kMinimumTravelStep);
    (void)checkProgression("NonlinearDrive", Voice::kNonlinearDrive,
                           {0.00F, 0.10F, 0.35F, 0.65F, 1.00F},
                           kMinimumTravelStep);

    const auto pressure_low = render(Voice::kPressure, 0.25F);
    const auto pressure_high = render(Voice::kPressure, 0.85F);
    const auto pressure = compare(pressure_low, pressure_high);
    if (!pressure_low.ok || !pressure_high.ok || pressure.rms_ratio_db < 12.0 ||
        pressure_high.overblow - pressure_low.overblow < 0.40) {
        ++failures;
        std::cerr << "FAIL: Pressure macro contract (energy + regime movement)\n";
    }

    const auto turbulence_low = render(Voice::kTurbulence, 0.05F);
    const auto turbulence_high = render(Voice::kTurbulence, 0.85F);
    const auto turbulence = compare(turbulence_low, turbulence_high);
    if (!turbulence_low.ok || !turbulence_high.ok || turbulence.rms_ratio_db < 9.0 ||
        turbulence_high.roughness < turbulence_low.roughness * 1.10) {
        ++failures;
        std::cerr << "FAIL: Turbulence macro contract (air/noise texture)\n";
    }

    const auto damping_low = render(Voice::kDamping, 0.03F);
    const auto damping_high = render(Voice::kDamping, 0.65F);
    const auto damping = compare(damping_low, damping_high);
    if (!damping_low.ok || !damping_high.ok || damping.rms_ratio_db > -9.0 ||
        damping_low.mode_radius_0 - damping_high.mode_radius_0 < 0.001) {
        ++failures;
        std::cerr << "FAIL: Damping macro contract (resonant loss)\n";
    }

    const auto drive_low = render(Voice::kNonlinearDrive, 0.05F);
    const auto drive_high = render(Voice::kNonlinearDrive, 0.90F);
    const auto drive = compare(drive_low, drive_high);
    if (!drive_low.ok || !drive_high.ok || drive.rms_ratio_db < 3.0 ||
        drive_high.roughness < drive_low.roughness * 1.15 ||
        drive_high.overblow - drive_low.overblow < 0.10) {
        ++failures;
        std::cerr << "FAIL: Nonlinear Drive macro contract (aggression/richness)\n";
    }

    std::cout << "Primary macro contracts: "
              << "pressure_db=" << pressure.rms_ratio_db
              << " pressure_overblow_delta="
              << (pressure_high.overblow - pressure_low.overblow)
              << " turbulence_db=" << turbulence.rms_ratio_db
              << " turbulence_roughness_ratio="
              << turbulence_high.roughness / std::max(turbulence_low.roughness, 1.0e-12)
              << " damping_db=" << damping.rms_ratio_db
              << " damping_radius_delta="
              << (damping_low.mode_radius_0 - damping_high.mode_radius_0)
              << " drive_db=" << drive.rms_ratio_db
              << " drive_roughness_ratio="
              << drive_high.roughness / std::max(drive_low.roughness, 1.0e-12)
              << " drive_overblow_delta="
              << (drive_high.overblow - drive_low.overblow) << '\n';
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
              << " low_roughness=" << low_render.roughness
              << " high_roughness=" << high_render.roughness
              << " low_lag1=" << low_render.lag1_correlation
              << " high_lag1=" << high_render.lag1_correlation
              << " low_crest_db=" << low_render.crest_db
              << " high_crest_db=" << high_render.crest_db
              << " low_overblow=" << low_render.overblow
              << " high_overblow=" << high_render.overblow
              << " low_radius0=" << low_render.mode_radius_0
              << " high_radius0=" << high_render.mode_radius_0
              << '\n';
}

} // namespace

int main() {
    checkStablePipeMacroIdentity();
    checkPrimaryMacroContracts();
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
