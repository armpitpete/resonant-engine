#include "resonant/Engine.hpp"

#include <array>
#include <cstdint>
#include <iostream>
#include <limits>
#include <span>

namespace {

class NonFiniteModel {
public:
    [[nodiscard]] bool prepare(const resonant::ProcessSpec& spec) noexcept {
        return spec.valid();
    }

    void reset() noexcept { sample_index_ = 0; }
    void handleEvent(const resonant::Event&) noexcept {}

    [[nodiscard]] bool processSample(std::span<const resonant::Sample>,
                                     std::span<resonant::Sample> output) noexcept {
        for (auto& sample : output) {
            sample = 0.5F;
        }
        if (sample_index_ == 1 && output.size() > 1) {
            output[1] = std::numeric_limits<resonant::Sample>::quiet_NaN();
        }
        ++sample_index_;
        return true;
    }

private:
    std::uint32_t sample_index_{0};
};

} // namespace

int main() {
    resonant::Engine<NonFiniteModel> engine;
    if (!engine.prepare({48'000.0, 8, 0, 2})) {
        std::cerr << "failed to prepare non-finite test model\n";
        return 1;
    }

    std::array<resonant::Sample, 8> left{};
    std::array<resonant::Sample, 8> right{};
    left.fill(0.75F);
    right.fill(0.75F);
    resonant::Sample* outputs[2]{left.data(), right.data()};
    resonant::AudioBlockView block{nullptr, outputs, 0, 2, 8};

    if (engine.process(block) != resonant::ProcessStatus::NumericalFailure) {
        std::cerr << "non-finite output did not report numerical failure\n";
        return 1;
    }
    if (left[0] != 0.5F || right[0] != 0.5F) {
        std::cerr << "samples before failure were unexpectedly cleared\n";
        return 1;
    }
    for (std::size_t i = 1; i < left.size(); ++i) {
        if (left[i] != 0.0F || right[i] != 0.0F) {
            std::cerr << "failure left stale output at frame " << i << '\n';
            return 1;
        }
    }

    std::cout << "PASS: numerical failure clears current and remaining output\n";
    return 0;
}
