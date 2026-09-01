#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>

namespace resonant {

using Sample = float;
using Accumulator = double;

inline constexpr std::uint32_t kMaxChannels = 2;
inline constexpr std::uint32_t kMaxBlockSize = 4096;
inline constexpr std::uint32_t kMaxEventsPerBlock = 1024;
inline constexpr double kMinSampleRate = 8'000.0;
inline constexpr double kMaxSampleRate = 384'000.0;

struct ProcessSpec {
    double sample_rate{48'000.0};
    std::uint32_t max_block_size{128};
    std::uint32_t input_channels{1};
    std::uint32_t output_channels{1};

    [[nodiscard]] constexpr bool valid() const noexcept {
        return sample_rate >= kMinSampleRate && sample_rate <= kMaxSampleRate &&
               max_block_size > 0 && max_block_size <= kMaxBlockSize &&
               input_channels <= kMaxChannels && output_channels > 0 &&
               output_channels <= kMaxChannels;
    }
};

enum class ProcessStatus : std::uint8_t {
    Ok,
    NotPrepared,
    InvalidContext,
    InvalidEventOrder,
    NumericalFailure,
};

struct AudioBlockView {
    const Sample* const* inputs{nullptr};
    Sample* const* outputs{nullptr};
    std::uint32_t input_channels{0};
    std::uint32_t output_channels{0};
    std::uint32_t frames{0};

    [[nodiscard]] bool structurallyValid(const ProcessSpec& spec) const noexcept {
        if (frames > spec.max_block_size || input_channels > spec.input_channels ||
            output_channels == 0 || output_channels > spec.output_channels ||
            output_channels > kMaxChannels || input_channels > kMaxChannels) {
            return false;
        }
        if (frames == 0) {
            return true;
        }
        if (outputs == nullptr) {
            return false;
        }
        for (std::uint32_t ch = 0; ch < output_channels; ++ch) {
            if (outputs[ch] == nullptr) {
                return false;
            }
        }
        if (input_channels > 0) {
            if (inputs == nullptr) {
                return false;
            }
            for (std::uint32_t ch = 0; ch < input_channels; ++ch) {
                if (inputs[ch] == nullptr) {
                    return false;
                }
            }
        }
        return true;
    }

    void clearOutputs(std::uint32_t maximum_frames = kMaxBlockSize) const noexcept {
        if (outputs == nullptr) {
            return;
        }
        const auto safe_frames = std::min(frames, maximum_frames);
        const auto safe_channels = std::min(output_channels, kMaxChannels);
        for (std::uint32_t ch = 0; ch < safe_channels; ++ch) {
            if (outputs[ch] != nullptr) {
                std::fill_n(outputs[ch], safe_frames, Sample{0});
            }
        }
    }
};

[[nodiscard]] inline Sample finiteOrZero(Sample value) noexcept {
    return std::isfinite(value) ? value : Sample{0};
}

[[nodiscard]] inline Sample clampFinite(Sample value, Sample low, Sample high,
                                        Sample fallback = Sample{0}) noexcept {
    if (!std::isfinite(value)) {
        return std::clamp(fallback, low, high);
    }
    return std::clamp(value, low, high);
}

} // namespace resonant
