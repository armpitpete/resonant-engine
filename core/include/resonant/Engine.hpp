#pragma once

#include "resonant/Event.hpp"
#include "resonant/Types.hpp"

#include <array>
#include <concepts>
#include <cstddef>
#include <span>
#include <type_traits>
#include <utility>

namespace resonant {

template <typename T>
concept EngineModel = requires(T model, const ProcessSpec& spec,
                               const Event& event,
                               std::span<const Sample> input,
                               std::span<Sample> output) {
    { model.prepare(spec) } noexcept -> std::same_as<bool>;
    { model.reset() } noexcept;
    { model.handleEvent(event) } noexcept;
    { model.processSample(input, output) } noexcept -> std::same_as<bool>;
};

template <EngineModel Model>
class Engine {
public:
    Engine() noexcept(std::is_nothrow_default_constructible_v<Model>) = default;
    explicit Engine(Model model) noexcept(std::is_nothrow_move_constructible_v<Model>)
        : model_(std::move(model)) {}

    [[nodiscard]] bool prepare(const ProcessSpec& spec) noexcept {
        if (!spec.valid()) {
            prepared_ = false;
            return false;
        }
        if (!model_.prepare(spec)) {
            prepared_ = false;
            return false;
        }
        spec_ = spec;
        model_.reset();
        prepared_ = true;
        return true;
    }

    [[nodiscard]] bool reset() noexcept {
        if (!prepared_) {
            return false;
        }
        model_.reset();
        return true;
    }

    [[nodiscard]] ProcessStatus process(const AudioBlockView& audio,
                                        std::span<const Event> events = {}) noexcept {
        if (!prepared_) {
            audio.clearOutputs(kMaxBlockSize);
            return ProcessStatus::NotPrepared;
        }
        if (!audio.structurallyValid(spec_)) {
            audio.clearOutputs(spec_.max_block_size);
            return ProcessStatus::InvalidContext;
        }
        if (audio.frames == 0) {
            return events.empty() ? ProcessStatus::Ok : ProcessStatus::InvalidContext;
        }
        if (events.size() > kMaxEventsPerBlock ||
            !eventsAreOrdered(events, audio.frames)) {
            audio.clearOutputs(spec_.max_block_size);
            return ProcessStatus::InvalidEventOrder;
        }

        std::array<Sample, kMaxChannels> input_frame{};
        std::array<Sample, kMaxChannels> output_frame{};
        std::size_t event_index = 0;

        for (std::uint32_t frame = 0; frame < audio.frames; ++frame) {
            while (event_index < events.size() &&
                   events[event_index].sample_offset == frame) {
                model_.handleEvent(events[event_index]);
                ++event_index;
            }

            for (std::uint32_t ch = 0; ch < audio.input_channels; ++ch) {
                input_frame[ch] = finiteOrZero(audio.inputs[ch][frame]);
            }
            for (std::uint32_t ch = audio.input_channels; ch < kMaxChannels; ++ch) {
                input_frame[ch] = 0.0F;
            }
            output_frame.fill(0.0F);

            const auto input_span = std::span<const Sample>{input_frame.data(), audio.input_channels};
            auto output_span = std::span<Sample>{output_frame.data(), audio.output_channels};
            if (!model_.processSample(input_span, output_span)) {
                zeroFromFrame(audio, frame);
                return ProcessStatus::NumericalFailure;
            }
            for (std::uint32_t ch = 0; ch < audio.output_channels; ++ch) {
                if (!std::isfinite(output_frame[ch])) {
                    zeroFromFrame(audio, frame);
                    return ProcessStatus::NumericalFailure;
                }
            }
            for (std::uint32_t ch = 0; ch < audio.output_channels; ++ch) {
                audio.outputs[ch][frame] = output_frame[ch];
            }
        }
        return ProcessStatus::Ok;
    }

    [[nodiscard]] bool prepared() const noexcept { return prepared_; }
    [[nodiscard]] const ProcessSpec& spec() const noexcept { return spec_; }
    [[nodiscard]] Model& model() noexcept { return model_; }
    [[nodiscard]] const Model& model() const noexcept { return model_; }

private:
    static void zeroFromFrame(const AudioBlockView& audio,
                              std::uint32_t start_frame) noexcept {
        for (std::uint32_t frame = start_frame; frame < audio.frames; ++frame) {
            for (std::uint32_t ch = 0; ch < audio.output_channels; ++ch) {
                audio.outputs[ch][frame] = 0.0F;
            }
        }
    }

    Model model_{};
    ProcessSpec spec_{};
    bool prepared_{false};
};

} // namespace resonant
