#pragma once

#include "resonant/BreathPipe.hpp"

#include <cmath>
#include <cstddef>
#include <cstdint>

namespace resonant::vst3 {

using HostParamId = std::uint32_t;

class HostParameterMapping {
public:
    [[nodiscard]] static constexpr bool exposed(const ParameterSpec& spec) noexcept {
        return spec.host_visible &&
               spec.kind != ParameterKind::Internal &&
               spec.kind != ParameterKind::Topology;
    }

    [[nodiscard]] static constexpr HostParamId toHostId(ParameterId id) noexcept {
        return static_cast<HostParamId>(id);
    }

    [[nodiscard]] static constexpr const ParameterSpec*
    specForHostId(HostParamId host_id) noexcept {
        for (const auto& spec : BreathPipeVoice::kParameterSpecs) {
            if (exposed(spec) && toHostId(spec.id) == host_id) {
                return &spec;
            }
        }
        return nullptr;
    }

    [[nodiscard]] static constexpr std::size_t exposedCount() noexcept {
        std::size_t count = 0U;
        for (const auto& spec : BreathPipeVoice::kParameterSpecs) {
            if (exposed(spec)) {
                ++count;
            }
        }
        return count;
    }

    [[nodiscard]] static bool normalizedToNative(HostParamId host_id,
                                                  double normalized,
                                                  Sample& native) noexcept {
        const auto* spec = specForHostId(host_id);
        if (spec == nullptr || !std::isfinite(normalized) ||
            normalized < 0.0 || normalized > 1.0) {
            return false;
        }
        native = spec->denormalize(static_cast<Sample>(normalized));
        return std::isfinite(native);
    }

    [[nodiscard]] static double nativeToNormalized(HostParamId host_id,
                                                   Sample native) noexcept {
        const auto* spec = specForHostId(host_id);
        if (spec == nullptr || !std::isfinite(native)) {
            return 0.0;
        }
        return static_cast<double>(spec->normalize(native));
    }
};

} // namespace resonant::vst3
