#pragma once

#include "resonant/Event.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string_view>

namespace resonant {

enum class ParameterKind : std::uint8_t {
    Continuous,
    Discrete,
    Topology,
    Internal,
};

enum class SmoothingMode : std::uint8_t {
    None,
    Linear,
    OnePole,
};

struct ParameterSpec {
    ParameterId id{0};
    std::string_view name{};
    std::string_view unit{};
    Sample minimum{0.0F};
    Sample maximum{1.0F};
    Sample default_value{0.0F};
    ParameterKind kind{ParameterKind::Continuous};
    SmoothingMode smoothing{SmoothingMode::None};
    double smoothing_seconds{0.0};
    bool host_visible{true};

    [[nodiscard]] bool valid() const noexcept {
        return id != 0 && !name.empty() && std::isfinite(minimum) &&
               std::isfinite(maximum) && minimum < maximum &&
               std::isfinite(default_value) && default_value >= minimum &&
               default_value <= maximum && smoothing_seconds >= 0.0 &&
               std::isfinite(smoothing_seconds);
    }

    [[nodiscard]] Sample clamp(Sample native) const noexcept {
        return clampFinite(native, minimum, maximum, default_value);
    }

    [[nodiscard]] Sample normalize(Sample native) const noexcept {
        const auto c = clamp(native);
        return (c - minimum) / (maximum - minimum);
    }

    [[nodiscard]] Sample denormalize(Sample normalized) const noexcept {
        const auto n = clampFinite(normalized, 0.0F, 1.0F, normalize(default_value));
        return minimum + n * (maximum - minimum);
    }
};

class ParameterSmoother {
public:
    void prepare(double sample_rate, SmoothingMode mode,
                 double smoothing_seconds) noexcept {
        sample_rate_ = (std::isfinite(sample_rate) && sample_rate > 0.0)
                           ? sample_rate
                           : 48'000.0;
        mode_ = mode;
        smoothing_seconds_ = std::max(0.0, smoothing_seconds);
        updateCoefficients();
        reset(current_);
    }

    void reset(Sample value) noexcept {
        current_ = finiteOrZero(value);
        target_ = current_;
        linear_step_ = 0.0F;
        linear_remaining_ = 0;
    }

    void setTarget(Sample target) noexcept {
        if (!std::isfinite(target)) {
            return;
        }
        target_ = target;
        if (mode_ == SmoothingMode::None || smoothing_samples_ == 0) {
            current_ = target_;
            linear_remaining_ = 0;
            return;
        }
        if (mode_ == SmoothingMode::Linear) {
            linear_remaining_ = smoothing_samples_;
            linear_step_ = (target_ - current_) /
                           static_cast<Sample>(linear_remaining_);
        }
    }

    [[nodiscard]] Sample next() noexcept {
        switch (mode_) {
        case SmoothingMode::None:
            current_ = target_;
            break;
        case SmoothingMode::Linear:
            if (linear_remaining_ > 0) {
                current_ += linear_step_;
                --linear_remaining_;
                if (linear_remaining_ == 0) {
                    current_ = target_;
                }
            }
            break;
        case SmoothingMode::OnePole:
            current_ = target_ + one_pole_coefficient_ * (current_ - target_);
            if (std::abs(current_ - target_) < 1.0e-7F) {
                current_ = target_;
            }
            break;
        }
        return current_;
    }

    [[nodiscard]] Sample current() const noexcept { return current_; }
    [[nodiscard]] Sample target() const noexcept { return target_; }

private:
    void updateCoefficients() noexcept {
        if (smoothing_seconds_ <= 0.0) {
            smoothing_samples_ = 0;
            one_pole_coefficient_ = 0.0F;
            return;
        }
        const auto exact_samples = smoothing_seconds_ * sample_rate_;
        smoothing_samples_ = static_cast<std::uint32_t>(
            std::clamp(exact_samples, 1.0, static_cast<double>(kMaxBlockSize) * 4096.0));
        one_pole_coefficient_ = static_cast<Sample>(
            std::exp(-1.0 / std::max(1.0, exact_samples)));
    }

    double sample_rate_{48'000.0};
    double smoothing_seconds_{0.0};
    SmoothingMode mode_{SmoothingMode::None};
    std::uint32_t smoothing_samples_{0};
    std::uint32_t linear_remaining_{0};
    Sample linear_step_{0.0F};
    Sample one_pole_coefficient_{0.0F};
    Sample current_{0.0F};
    Sample target_{0.0F};
};

} // namespace resonant
