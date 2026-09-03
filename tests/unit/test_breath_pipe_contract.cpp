#include "resonant/BreathPipe.hpp"
#include "resonant/Engine.hpp"

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

[[nodiscard]] double midiToHz(std::uint32_t note) {
    return 440.0 * std::pow(2.0, (static_cast<double>(note) - 69.0) / 12.0);
}

[[nodiscard]] resonant::BreathPipeModalResonator::Control
stableControl(double target_hz) {
    return {
        static_cast<float>(target_hz),
        0.08F,
        0.55F,
        0.72F,
        0.38F,
        0.25F,
        0.35F,
        0.25F,
    };
}

[[nodiscard]] double estimateModeFrequency(
    double sample_rate,
    resonant::BreathPipeModalResonator::Control control) {
    resonant::BreathPipeModalResonator resonator;
    if (!resonator.prepare({sample_rate, 128, 1, 1})) {
        return 0.0;
    }

    double previous = 0.0;
    std::array<double, 16> crossings{};
    std::size_t crossing_count = 0U;
    const auto maximum_frames = static_cast<std::uint32_t>(sample_rate * 0.75);
    const auto measurement_start = static_cast<std::uint32_t>(sample_rate * 0.02);
    for (std::uint32_t frame = 0U;
         frame < maximum_frames && crossing_count < crossings.size(); ++frame) {
        const auto excitation = frame == 0U ? 1.0e-5F : 0.0F;
        (void)resonator.processSample(excitation, control);
        const auto current = static_cast<double>(resonator.modeSample(0));
        if (frame > measurement_start && previous <= 0.0 && current > 0.0) {
            crossings[crossing_count++] = static_cast<double>(frame);
        }
        previous = current;
    }
    if (crossing_count < 4U) {
        return 0.0;
    }

    double period_sum = 0.0;
    for (std::size_t index = 1U; index < crossing_count; ++index) {
        period_sum += crossings[index] - crossings[index - 1U];
    }
    const auto mean_period = period_sum / static_cast<double>(crossing_count - 1U);
    return mean_period > 0.0 ? sample_rate / mean_period : 0.0;
}

[[nodiscard]] double centsError(double measured, double target) {
    return measured > 0.0 && target > 0.0
               ? 1'200.0 * std::log2(measured / target)
               : 1.0e9;
}

void testProcessedOperatingEnvelope() {
    constexpr std::array<double, 3> sample_rates{{44'100.0, 48'000.0, 96'000.0}};
    constexpr std::array<std::uint32_t, 7> block_sizes{{32U, 64U, 128U, 256U,
                                                        512U, 1024U, 4096U}};
    std::array<resonant::Sample, 4096> input{};
    std::array<resonant::Sample, 4096> output{};
    const resonant::Sample* inputs[1]{input.data()};
    resonant::Sample* outputs[1]{output.data()};

    for (const auto sample_rate : sample_rates) {
        for (const auto block_size : block_sizes) {
            resonant::Engine<resonant::BreathPipeVoice> engine{
                resonant::BreathPipeVoice{777U}};
            check(engine.prepare({sample_rate, block_size, 1, 1}),
                  "M3 processed envelope prepares");
            resonant::AudioBlockView block{inputs, outputs, 1, 1, block_size};
            const std::array<resonant::Event, 4> events{{
                {0, resonant::EventType::Pitch, 0, 0, 220.0F, 0.0F},
                {0, resonant::EventType::Pressure, 0, 0, 0.55F, 0.0F},
                {0, resonant::EventType::ParameterChange,
                 resonant::BreathPipeVoice::kInteraction, 0, 0.72F, 0.0F},
                {0, resonant::EventType::ParameterChange,
                 resonant::BreathPipeVoice::kRegeneration, 0, 0.38F, 0.0F},
            }};
            check(engine.process(block, events) == resonant::ProcessStatus::Ok,
                  "M3 processed envelope executes");
            check(std::all_of(output.begin(), output.begin() + block_size,
                              [](resonant::Sample sample) { return std::isfinite(sample); }),
                  "M3 processed envelope stays finite");
        }
    }
}

void testCanonicalStablePointTuning() {
    constexpr std::array<std::uint32_t, 5> notes{{36U, 48U, 60U, 72U, 84U}};
    constexpr std::array<double, 3> sample_rates{{44'100.0, 48'000.0, 96'000.0}};
    double absolute_error_sum = 0.0;
    std::size_t measured_count = 0U;
    double maximum_absolute_error = 0.0;

    for (const auto sample_rate : sample_rates) {
        for (const auto note : notes) {
            const auto target = midiToHz(note);
            const auto measured = estimateModeFrequency(sample_rate, stableControl(target));
            check(measured > 0.0, "canonical stable-pipe pitch measurement exists");
            if (measured <= 0.0) {
                continue;
            }
            const auto absolute_error = std::abs(centsError(measured, target));
            absolute_error_sum += absolute_error;
            maximum_absolute_error = std::max(maximum_absolute_error, absolute_error);
            ++measured_count;
        }
    }

    const auto mean_absolute_error = measured_count == 0U
                                         ? 1.0e9
                                         : absolute_error_sum /
                                               static_cast<double>(measured_count);
    check(mean_absolute_error <= 15.0,
          "canonical stable-pipe C2-C6 mean tuning error is within 15 cents");
    check(maximum_absolute_error <= 30.0,
          "canonical stable-pipe C2-C6 has no note beyond 30 cents");

    auto low_pressure = stableControl(220.0);
    low_pressure.pressure = 0.25F;
    auto high_pressure = stableControl(220.0);
    high_pressure.pressure = 0.75F;
    const auto low_pressure_hz = estimateModeFrequency(48'000.0, low_pressure);
    const auto high_pressure_hz = estimateModeFrequency(48'000.0, high_pressure);
    check(low_pressure_hz > 0.0 && high_pressure_hz > 0.0,
          "pressure-dependent pitch measurements exist");
    if (low_pressure_hz > 0.0 && high_pressure_hz > 0.0) {
        check(std::abs(centsError(high_pressure_hz, low_pressure_hz)) <= 30.0,
              "pressure-dependent pitch movement remains bounded");
    }

    auto low_loss = stableControl(220.0);
    low_loss.damping = 0.02F;
    auto high_loss = stableControl(220.0);
    high_loss.damping = 0.80F;
    const auto low_loss_hz = estimateModeFrequency(48'000.0, low_loss);
    const auto high_loss_hz = estimateModeFrequency(48'000.0, high_loss);
    check(low_loss_hz > 0.0 && high_loss_hz > 0.0,
          "damping-dependent pitch measurements exist");
    if (low_loss_hz > 0.0 && high_loss_hz > 0.0) {
        check(std::abs(centsError(high_loss_hz, low_loss_hz)) <= 30.0,
              "damping-dependent pitch movement remains bounded");
    }
}

[[nodiscard]] double closedLoopModeSignature(float feedback_color) {
    resonant::Engine<resonant::BreathPipeVoice> engine{resonant::BreathPipeVoice{991U}};
    if (!engine.prepare({48'000.0, 128, 1, 1})) {
        return 0.0;
    }
    std::array<resonant::Sample, 128> input{};
    std::array<resonant::Sample, 128> output{};
    const resonant::Sample* inputs[1]{input.data()};
    resonant::Sample* outputs[1]{output.data()};
    resonant::AudioBlockView block{inputs, outputs, 1, 1, 128};
    const std::array<resonant::Event, 7> events{{
        {0, resonant::EventType::Pitch, 0, 0, 220.0F, 0.0F},
        {0, resonant::EventType::Pressure, 0, 0, 0.72F, 0.0F},
        {0, resonant::EventType::ParameterChange,
         resonant::BreathPipeVoice::kTurbulence, 0, 0.18F, 0.0F},
        {0, resonant::EventType::ParameterChange,
         resonant::BreathPipeVoice::kInteraction, 0, 1.0F, 0.0F},
        {0, resonant::EventType::ParameterChange,
         resonant::BreathPipeVoice::kRegeneration, 0, 0.80F, 0.0F},
        {0, resonant::EventType::ParameterChange,
         resonant::BreathPipeVoice::kFeedbackColor, 0, feedback_color, 0.0F},
        {0, resonant::EventType::ParameterChange,
         resonant::BreathPipeVoice::kNonlinearDrive, 0, 0.60F, 0.0F},
    }};

    double signature = 0.0;
    for (std::uint32_t block_index = 0U; block_index < 220U; ++block_index) {
        const auto scheduled = block_index == 0U
                                   ? std::span<const resonant::Event>{events}
                                   : std::span<const resonant::Event>{};
        if (engine.process(block, scheduled) != resonant::ProcessStatus::Ok) {
            return 0.0;
        }
        if (block_index > 80U) {
            const auto& resonator = engine.model().resonator();
            signature += static_cast<double>(resonator.modeSample(0)) * 1.0 +
                         static_cast<double>(resonator.modeSample(1)) * 3.0 +
                         static_cast<double>(resonator.modeSample(2)) * 7.0;
        }
    }
    return signature;
}

[[nodiscard]] double nonlinearModeSignature(float nonlinear_drive) {
    resonant::BreathPipeModalResonator resonator;
    if (!resonator.prepare({48'000.0, 128, 1, 1})) {
        return 0.0;
    }
    auto control = stableControl(220.0);
    control.pressure = 0.70F;
    control.regeneration = 0.80F;
    control.nonlinear_drive = nonlinear_drive;

    double signature = 0.0;
    for (std::uint32_t frame = 0U; frame < 48'000U; ++frame) {
        const auto centred = static_cast<int>(frame % 17U) - 8;
        const auto excitation = 0.00008F * static_cast<float>(centred);
        (void)resonator.processSample(excitation, control);
        if (frame > 12'000U) {
            signature += std::abs(static_cast<double>(resonator.modeSample(0))) +
                         3.0 * std::abs(static_cast<double>(resonator.modeSample(1))) +
                         7.0 * std::abs(static_cast<double>(resonator.modeSample(2)));
        }
    }
    return signature;
}

void testLoopShapingAndReversibleOverblow() {
    const auto dark_feedback = closedLoopModeSignature(0.0F);
    const auto bright_feedback = closedLoopModeSignature(1.0F);
    check(std::abs(dark_feedback - bright_feedback) > 1.0e-7,
          "feedback colour changes resonator state through the closed interaction loop");

    const auto linear_signature = nonlinearModeSignature(0.0F);
    const auto nonlinear_signature = nonlinearModeSignature(1.0F);
    check(std::abs(linear_signature - nonlinear_signature) > 1.0e-6,
          "nonlinear drive changes modal recursion rather than only output shaping");

    resonant::BreathPipeModalResonator resonator;
    check(resonator.prepare({48'000.0, 128, 1, 1}),
          "reversible overblow resonator prepares");
    auto low = stableControl(220.0);
    low.pressure = 0.42F;
    low.interaction = 0.85F;
    low.nonlinear_drive = 0.85F;
    auto high = low;
    high.pressure = 0.95F;

    for (std::size_t frame = 0U; frame < 16U; ++frame) {
        (void)resonator.processSample(frame == 0U ? 0.0004F : 0.0F, low);
    }
    const auto low_radius_0 = resonator.modeRadius(0);
    const auto low_radius_1 = resonator.modeRadius(1);
    for (std::size_t frame = 0U; frame < 16U; ++frame) {
        (void)resonator.processSample(0.0F, high);
    }
    check(resonator.overblowAmount() > 0.50F &&
              resonator.modeRadius(1) > resonator.modeRadius(0),
          "continuous high-pressure state reorganises toward upper mode");
    for (std::size_t frame = 0U; frame < 16U; ++frame) {
        (void)resonator.processSample(0.0F, low);
    }
    check(resonator.overblowAmount() < 0.01F,
          "overblow state returns without reset or topology switch");
    check(std::abs(resonator.modeRadius(0) - low_radius_0) < 1.0e-7F &&
              std::abs(resonator.modeRadius(1) - low_radius_1) < 1.0e-7F,
          "reverse traversal restores the low-pressure modal operating point");
}

} // namespace

int main() {
    testProcessedOperatingEnvelope();
    testCanonicalStablePointTuning();
    testLoopShapingAndReversibleOverblow();
    if (failures != 0) {
        std::cerr << failures << " Breath Pipe contract test(s) failed\n";
        return 1;
    }
    std::cout << "PASS: M3 Breath Pipe operating, tuning and loop contracts\n";
    return 0;
}
