#pragma once

#include "resonant/BreathPipe.hpp"
#include "resonant/BreathPipeState.hpp"
#include "resonant/Engine.hpp"

#include <cstdint>
#include <span>

namespace resonant::vst3 {

class BreathPipeCoreAdapter {
public:
    [[nodiscard]] bool prepare(double sample_rate,
                               std::uint32_t max_block_size,
                               std::uint32_t input_channels = 0U,
                               std::uint32_t output_channels = 2U) noexcept {
        const ProcessSpec spec{
            sample_rate,
            max_block_size,
            input_channels,
            output_channels,
        };
        prepared_ = engine_.prepare(spec);
        return prepared_;
    }

    [[nodiscard]] bool reset() noexcept {
        if (!prepared_) {
            return false;
        }
        BreathPipeState state{};
        return captureState(state) && restoreState(state);
    }

    [[nodiscard]] bool captureState(BreathPipeState& state) const noexcept {
        return prepared_ && captureBreathPipeState(engine_.model(), state);
    }

    [[nodiscard]] bool restoreState(const BreathPipeState& state) noexcept {
        return prepared_ && restoreBreathPipeState(engine_.model(), state);
    }

    [[nodiscard]] ProcessStatus process(const Sample* const* inputs,
                                        std::uint32_t input_channels,
                                        Sample* const* outputs,
                                        std::uint32_t output_channels,
                                        std::uint32_t frames,
                                        std::span<const Event> events = {}) noexcept {
        const AudioBlockView block{
            inputs,
            outputs,
            input_channels,
            output_channels,
            frames,
        };
        return engine_.process(block, events);
    }

    [[nodiscard]] bool prepared() const noexcept { return prepared_; }
    [[nodiscard]] const ProcessSpec& spec() const noexcept { return engine_.spec(); }

private:
    Engine<BreathPipeVoice> engine_{BreathPipeVoice{kDefaultSeed}};
    bool prepared_{false};
};

} // namespace resonant::vst3
