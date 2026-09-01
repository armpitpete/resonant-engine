#pragma once

#include "resonant/Types.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace resonant {

enum class EnergyState : std::uint8_t {
    Silent,
    Passive,
    Regenerative,
    SelfSustaining,
    Unstable,
    NumericalRunaway,
};

struct EnergyDiagnostics {
    Accumulator excitation_rms{0.0};
    Accumulator resonator_rms{0.0};
    Accumulator output_rms{0.0};
    Accumulator feedback_rms{0.0};
    Sample peak{0.0F};
    EnergyState state{EnergyState::Silent};
    bool nan_detected{false};
    bool infinity_detected{false};
    bool runaway_detected{false};
};

class EnergyMonitor {
public:
    void prepare(std::uint32_t observation_frames = 256,
                 Sample runaway_peak = 8.0F) noexcept {
        observation_frames_ = std::clamp<std::uint32_t>(observation_frames, 1U, 8192U);
        runaway_peak_ = std::max(1.0F, finiteOrZero(runaway_peak));
        reset();
    }

    void reset() noexcept {
        count_ = 0;
        excitation_sum_ = 0.0;
        resonator_sum_ = 0.0;
        output_sum_ = 0.0;
        feedback_sum_ = 0.0;
        diagnostics_ = {};
    }

    void observe(Sample excitation, Sample resonator, Sample output,
                 Sample feedback) noexcept {
        const Sample values[] = {excitation, resonator, output, feedback};
        for (Sample value : values) {
            if (std::isnan(value)) {
                diagnostics_.nan_detected = true;
            } else if (std::isinf(value)) {
                diagnostics_.infinity_detected = true;
            }
        }
        if (diagnostics_.nan_detected || diagnostics_.infinity_detected) {
            diagnostics_.runaway_detected = true;
            diagnostics_.state = EnergyState::NumericalRunaway;
            return;
        }

        const auto ex = static_cast<Accumulator>(excitation);
        const auto re = static_cast<Accumulator>(resonator);
        const auto ou = static_cast<Accumulator>(output);
        const auto fb = static_cast<Accumulator>(feedback);
        excitation_sum_ += ex * ex;
        resonator_sum_ += re * re;
        output_sum_ += ou * ou;
        feedback_sum_ += fb * fb;
        diagnostics_.peak = std::max(diagnostics_.peak, std::abs(output));
        ++count_;

        if (diagnostics_.peak >= runaway_peak_) {
            diagnostics_.runaway_detected = true;
        }
        if (count_ >= observation_frames_) {
            finalizeWindow();
        }
    }

    [[nodiscard]] const EnergyDiagnostics& diagnostics() const noexcept {
        return diagnostics_;
    }

    [[nodiscard]] static Sample contain(Sample value,
                                        Sample ceiling = 1.0F) noexcept {
        if (!std::isfinite(value)) {
            return 0.0F;
        }
        return std::clamp(value, -std::abs(ceiling), std::abs(ceiling));
    }

private:
    void finalizeWindow() noexcept {
        const auto divisor = static_cast<Accumulator>(count_);
        diagnostics_.excitation_rms = std::sqrt(excitation_sum_ / divisor);
        diagnostics_.resonator_rms = std::sqrt(resonator_sum_ / divisor);
        diagnostics_.output_rms = std::sqrt(output_sum_ / divisor);
        diagnostics_.feedback_rms = std::sqrt(feedback_sum_ / divisor);

        constexpr Accumulator silence = 1.0e-7;
        if (diagnostics_.runaway_detected) {
            diagnostics_.state = EnergyState::Unstable;
        } else if (diagnostics_.output_rms < silence && diagnostics_.excitation_rms < silence) {
            diagnostics_.state = EnergyState::Silent;
        } else if (diagnostics_.feedback_rms < diagnostics_.excitation_rms * 0.1) {
            diagnostics_.state = EnergyState::Passive;
        } else if (diagnostics_.excitation_rms < silence && diagnostics_.output_rms > silence) {
            diagnostics_.state = EnergyState::SelfSustaining;
        } else {
            diagnostics_.state = EnergyState::Regenerative;
        }

        count_ = 0;
        excitation_sum_ = resonator_sum_ = output_sum_ = feedback_sum_ = 0.0;
        diagnostics_.peak = 0.0F;
    }

    std::uint32_t observation_frames_{256};
    std::uint32_t count_{0};
    Sample runaway_peak_{8.0F};
    Accumulator excitation_sum_{0.0};
    Accumulator resonator_sum_{0.0};
    Accumulator output_sum_{0.0};
    Accumulator feedback_sum_{0.0};
    EnergyDiagnostics diagnostics_{};
};

} // namespace resonant
