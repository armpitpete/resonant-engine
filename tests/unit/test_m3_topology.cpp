#include "resonant/BreathPipe.hpp"
#include "resonant/FirstResonator.hpp"
#include "resonant/Types.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <string_view>

namespace {

int failures = 0;

void check(bool condition, std::string_view name) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << name << '\n';
    }
}

struct CandidateEvidence {
    std::string_view name{};
    std::size_t memory_bytes{0U};
    bool exact_silence{false};
    bool external_excitation{false};
    bool bidirectional_interaction{false};
    bool continuous_pitch{false};
    bool continuous_overblow_reorganisation{false};
    bool bounded_work{false};

    [[nodiscard]] int score() const noexcept {
        return static_cast<int>(exact_silence) + static_cast<int>(external_excitation) +
               static_cast<int>(bidirectional_interaction) + static_cast<int>(continuous_pitch) +
               2 * static_cast<int>(continuous_overblow_reorganisation) +
               static_cast<int>(bounded_work);
    }
};

// Deliberately small M3.1 prototype: two travelling-wave delay rails with a
// nonlinear reflection junction. It establishes that a scattering approach is
// computationally credible and bidirectional, but this bounded prototype has no
// continuously controlled pitch or register-reorganisation mechanism.
class ScatteringWaveguidePrototype {
public:
    static constexpr std::size_t kCapacity = 1024U;

    void reset() noexcept {
        outgoing_.fill(0.0F);
        returning_.fill(0.0F);
        index_ = 0U;
    }

    [[nodiscard]] float process(float excitation, float reflection) noexcept {
        const auto returned = returning_[index_];
        const auto junction = std::tanh(excitation + reflection * returned);
        outgoing_[index_] = junction;
        returning_[index_] = -0.985F * outgoing_[(index_ + 1U) % kCapacity];
        index_ = (index_ + 1U) % kCapacity;
        return returned;
    }

    [[nodiscard]] static constexpr std::size_t memoryBytes() noexcept {
        return 2U * kCapacity * sizeof(float);
    }

private:
    std::array<float, kCapacity> outgoing_{};
    std::array<float, kCapacity> returning_{};
    std::size_t index_{0U};
};

[[nodiscard]] CandidateEvidence probeTunedDelay() {
    resonant::TunedDelayResonator resonator;
    const auto prepared = resonator.prepare({48'000.0, 128, 1, 1});
    const auto silent = resonator.processSample(
        {0.0F, {220.0F, 0.2F, 0.0F, 0.0F}, {}}).sample == 0.0F;
    return {
        "M1 tuned-delay extension",
        resonant::TunedDelayResonator::memoryBytes(),
        prepared && silent,
        true,
        true,
        true,
        false, // one tuned-delay mode has no native register redistribution gate
        true,
    };
}

[[nodiscard]] CandidateEvidence probeScatteringWaveguide() {
    ScatteringWaveguidePrototype prototype;
    prototype.reset();
    bool silent = true;
    for (std::size_t index = 0U; index < 2048U; ++index) {
        silent = silent && prototype.process(0.0F, 0.8F) == 0.0F;
    }
    prototype.reset();
    bool returned_energy = false;
    for (std::size_t index = 0U; index < 4096U; ++index) {
        const auto output = prototype.process(index == 0U ? 0.25F : 0.0F, 0.8F);
        returned_energy = returned_energy || std::abs(output) > 1.0e-6F;
    }
    return {
        "bounded scattering-waveguide prototype",
        ScatteringWaveguidePrototype::memoryBytes(),
        silent,
        returned_energy,
        returned_energy,
        false, // this bounded prototype has no continuous tuning control
        false, // credible base, but extra mode/register machinery would be required
        true,
    };
}

[[nodiscard]] CandidateEvidence probeModalInteraction() {
    resonant::BreathPipeModalResonator resonator;
    const auto prepared = resonator.prepare({48'000.0, 128, 1, 1});
    resonant::BreathPipeModalResonator::Control low;
    low.tuning_hz = 220.0F;
    low.pressure = 0.42F;
    low.interaction = 0.85F;
    low.nonlinear_drive = 0.85F;
    low.regeneration = 0.50F;
    low.damping = 0.05F;
    const auto silent = resonator.processSample(0.0F, low).sample == 0.0F;
    for (std::size_t index = 0U; index < 64U; ++index) {
        (void)resonator.processSample(index == 0U ? 0.0004F : 0.0F, low);
    }
    const auto low_overblow = resonator.overblowAmount();
    const auto low_fundamental_radius = resonator.modeRadius(0);
    const auto low_upper_radius = resonator.modeRadius(1);

    auto high = low;
    high.pressure = 0.95F;
    for (std::size_t index = 0U; index < 64U; ++index) {
        (void)resonator.processSample(0.0F, high);
    }
    const auto high_overblow = resonator.overblowAmount();
    const auto high_fundamental_radius = resonator.modeRadius(0);
    const auto high_upper_radius = resonator.modeRadius(1);
    const auto reorganisation = low_overblow < 0.01F && high_overblow > 0.50F &&
                                high_fundamental_radius < low_fundamental_radius &&
                                high_upper_radius > low_upper_radius &&
                                high_upper_radius > high_fundamental_radius;
    return {
        "selected three-mode interacting resonator",
        resonant::BreathPipeModalResonator::memoryBytes(),
        prepared && silent,
        true,
        true,
        true,
        reorganisation,
        true,
    };
}

} // namespace

int main() {
    const auto tuned = probeTunedDelay();
    const auto scattering = probeScatteringWaveguide();
    const auto modal = probeModalInteraction();

    check(tuned.exact_silence && scattering.exact_silence && modal.exact_silence,
          "all topology candidates preserve true silence");
    check(scattering.bidirectional_interaction,
          "scattering prototype demonstrates credible bidirectional topology");
    check(!scattering.continuous_pitch,
          "scattering prototype is not credited with unimplemented pitch control");
    check(modal.continuous_overblow_reorganisation,
          "selected modal candidate demonstrates continuous register reorganisation");
    check(modal.memory_bytes < tuned.memory_bytes,
          "selected modal candidate has lower fixed state cost than M1 delay extension");
    check(modal.score() > tuned.score() && modal.score() > scattering.score(),
          "modal/interacting candidate wins the frozen M3.1 capability score");

    std::cout << "M3.1 topology evidence\n"
              << "  tuned-delay: score=" << tuned.score()
              << " bytes=" << tuned.memory_bytes << '\n'
              << "  scattering: score=" << scattering.score()
              << " bytes=" << scattering.memory_bytes << '\n'
              << "  modal: score=" << modal.score()
              << " bytes=" << modal.memory_bytes << '\n';

    if (failures != 0) {
        std::cerr << failures << " topology-selection test(s) failed\n";
        return 1;
    }
    std::cout << "PASS: M3.1 selected bounded three-mode interacting topology\n";
    return 0;
}
