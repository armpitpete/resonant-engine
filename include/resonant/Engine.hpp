#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>

namespace resonant {

struct ProcessSpec {
    double sample_rate = 48000.0;
    std::uint32_t max_block_size = 128;
};

// M0 architecture probe, not the final host-facing Engine API. The canonical
// processing, event, parameter and exciter/resonator contracts are defined by
// M0.6-M0.10. This type exists to prove that host block delivery can contain a
// model-owned sample-by-sample closed loop without host-framework types.
template <typename Model>
class Engine final {
public:
    using Input = typename Model::Input;
    using Output = typename Model::Output;

    static_assert(noexcept(std::declval<Model&>().prepare(std::declval<const ProcessSpec&>())),
                  "Model::prepare must be noexcept");
    static_assert(noexcept(std::declval<Model&>().reset()),
                  "Model::reset must be noexcept");
    static_assert(noexcept(std::declval<Model&>().tick(std::declval<const Input&>())),
                  "Model::tick must be noexcept");

    bool prepare(const ProcessSpec& spec) noexcept {
        if (spec.sample_rate <= 0.0 || spec.max_block_size == 0) {
            prepared_ = false;
            return false;
        }

        spec_ = spec;
        model_.prepare(spec_);
        model_.reset();
        prepared_ = true;
        return true;
    }

    void reset() noexcept {
        model_.reset();
    }

    Output processSample(const Input& input) noexcept {
        if (!prepared_) {
            return Output{};
        }
        return model_.tick(input);
    }

    void processBlock(const Input* inputs, Output* outputs, std::size_t frames) noexcept {
        if (!prepared_ || inputs == nullptr || outputs == nullptr) {
            return;
        }

        for (std::size_t i = 0; i < frames; ++i) {
            outputs[i] = model_.tick(inputs[i]);
        }
    }

    const ProcessSpec& spec() const noexcept {
        return spec_;
    }

    Model& model() noexcept {
        return model_;
    }

    const Model& model() const noexcept {
        return model_;
    }

private:
    ProcessSpec spec_{};
    Model model_{};
    bool prepared_ = false;
};

} // namespace resonant
