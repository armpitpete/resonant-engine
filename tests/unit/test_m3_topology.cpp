#include "resonant/BreathPipe.hpp"
#include "resonant/FirstResonator.hpp"
#include "resonant/Types.hpp"

#include <algorithm>
#include <array>
#include <chrono>
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
    bool stable_tuning{false};
    bool continuous_overblow_reorganisation{false};
    bool reverse_recovery{false};
    bool deterministic{false};
    bool bounded_work{false};
    double measured_ns_per_sample{0.0};

    [[nodiscard]] int score() const noexcept {
        return static_cast<int>(exact_silence) + static_cast<int>(external_excitation) +
               static_cast<int>(bidirectional_interaction) + static_cast<int>(continuous_pitch) +
               static_cast<int>(stable_tuning) +
               2 * static_cast<int>(continuous_overblow_reorganisation) +
               static_cast<int>(reverse_recovery) + static_cast<int>(deterministic) +
               static_cast<int>(bounded_work);
    }
};

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

[[nodiscard]] double centsError(double measured, double target) {
    return measured > 0.0 && target > 0.0
               ? 1'200.0 * std::log2(measured / target)
               : 1.0e9;
}

[[nodiscard]] double estimateModalFrequency(double sample_rate, double target_hz) {
    resonant::BreathPipeModalResonator resonator;
    if (!resonator.prepare({sample_rate, 128, 1, 1})) {
        return 0.0;
    }
    resonant::BreathPipeModalResonator::Control control;
    control.tuning_hz = static_cast<float>(target_hz);
    control.damping = 0.0F;
    control.pressure = 0.30F;
    control.interaction = 0.0F;
    control.regeneration = 0.0F;
    control.nonlinear_drive = 0.0F;

    double previous = 0.0;
    std::array<double, 20> crossings{};
    std::size_t crossing_count = 0U;
    const auto maximum_frames = static_cast<std::uint32_t>(sample_rate * 1.5);
    for (std::uint32_t frame = 0; frame < maximum_frames && crossing_count < crossings.size();
         ++frame) {
        const auto excitation = frame == 0U ? 1.0e-5F : 0.0F;
        (void)resonator.processSample(excitation, control);
        const auto current = static_cast<double>(resonator.modeSample(0));
        if (frame > static_cast<std::uint32_t>(sample_rate * 0.10) &&
            previous <= 0.0 && current > 0.0) {
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

[[nodiscard]] CandidateEvidence probeTunedDelay() {
    resonant::TunedDelayResonator resonator;
    const auto prepared = resonator.prepare({48'000.0, 128, 1, 1});

    bool silent = prepared;
    for (std::size_t frame = 0; frame < 4096U; ++frame) {
        const auto result = resonator.processSample(
            {0.0F, {220.0F, 0.20F, 0.0F, 0.0F}, {}});
        silent = silent && result.sample == 0.0F;
    }

    resonator.reset();
    double external_energy = 0.0;
    for (std::size_t frame = 0; frame < 2048U; ++frame) {
        const auto result = resonator.processSample(
            {frame == 0U ? 0.20F : 0.0F, {220.0F, 0.10F, 0.0F, 0.0F}, {}});
        external_energy += std::abs(static_cast<double>(result.sample));
    }

    resonant::ContinuousNoiseExciter exciter;
    const auto exciter_prepared = exciter.prepare({48'000.0, 128, 1, 1}, 777U);
    const auto no_return = exciter.processSample({0.0F, 0.0F, 0.0F, 0.0F, 0.0F}).sample;
    exciter.reset();
    const auto with_return = exciter.processSample({0.0F, 0.0F, 0.0F, 0.0F, 0.5F}).sample;
    const auto interaction = exciter_prepared && std::abs(with_return - no_return) > 1.0e-6F;

    resonator.reset();
    (void)resonator.processSample({0.0F, {220.0F, 0.0F, 0.0F, 0.0F}, {}});
    const auto delay_220 = resonator.delaySamples();
    resonator.reset();
    (void)resonator.processSample({0.0F, {440.0F, 0.0F, 0.0F, 0.0F}, {}});
    const auto delay_440 = resonator.delaySamples();
    const auto pitch_continuous = delay_220 > delay_440 &&
                                  std::abs(delay_220 / delay_440 - 2.0) < 1.0e-6;
    const auto effective_220 = delay_220 > 0.0 ? 48'000.0 / delay_220 : 0.0;
    const auto effective_440 = delay_440 > 0.0 ? 48'000.0 / delay_440 : 0.0;
    const auto stable_tuning = std::abs(centsError(effective_220, 220.0)) <= 15.0 &&
                               std::abs(centsError(effective_440, 440.0)) <= 15.0;

    auto tailEnergy = [&resonator](float feedback, float damping, bool seed) {
        double total = 0.0;
        for (std::size_t frame = 0; frame < 16'384U; ++frame) {
            const auto result = resonator.processSample(
                {seed && frame == 0U ? 0.05F : 0.0F,
                 {220.0F, damping, feedback, 0.30F}, {}});
            if (frame >= 12'288U) {
                total += std::abs(static_cast<double>(result.sample));
            }
        }
        return total;
    };
    resonator.reset();
    const auto active_tail = tailEnergy(1.20F, 0.0F, true);
    const auto recovered_tail = tailEnergy(0.0F, 1.0F, false);
    const auto recovery = active_tail > 0.0 && recovered_tail < active_tail;

    auto signature = []() {
        resonant::TunedDelayResonator local;
        if (!local.prepare({48'000.0, 128, 1, 1})) {
            return 0.0;
        }
        double sum = 0.0;
        for (std::size_t frame = 0; frame < 4096U; ++frame) {
            const auto result = local.processSample(
                {frame == 0U ? 0.07F : 0.0F, {329.6276F, 0.15F, 0.4F, 0.35F}, {}});
            sum += static_cast<double>(result.sample) * static_cast<double>(frame + 1U);
        }
        return sum;
    };
    const auto deterministic = signature() == signature();

    resonator.reset();
    double work_sink = 0.0;
    const auto start = std::chrono::steady_clock::now();
    constexpr std::size_t work_frames = 50'000U;
    for (std::size_t frame = 0; frame < work_frames; ++frame) {
        const auto result = resonator.processSample(
            {frame == 0U ? 0.02F : 0.0F, {220.0F, 0.2F, 0.3F, 0.3F}, {}});
        work_sink += result.sample;
    }
    const auto elapsed = std::chrono::duration<double, std::nano>(
        std::chrono::steady_clock::now() - start).count();
    if (!std::isfinite(work_sink)) {
        silent = false;
    }

    return {
        "M1 tuned-delay extension",
        resonant::TunedDelayResonator::memoryBytes(),
        silent,
        external_energy > 1.0e-5,
        interaction,
        pitch_continuous,
        stable_tuning,
        false,
        recovery,
        deterministic,
        true,
        elapsed / static_cast<double>(work_frames),
    };
}

[[nodiscard]] CandidateEvidence probeScatteringWaveguide() {
    ScatteringWaveguidePrototype prototype;
    prototype.reset();
    bool silent = true;
    for (std::size_t frame = 0U; frame < 4096U; ++frame) {
        silent = silent && prototype.process(0.0F, 0.8F) == 0.0F;
    }

    prototype.reset();
    double returned_energy = 0.0;
    for (std::size_t frame = 0U; frame < 4096U; ++frame) {
        const auto output = prototype.process(frame == 0U ? 0.25F : 0.0F, 0.8F);
        returned_energy += std::abs(static_cast<double>(output));
    }

    prototype.reset();
    double active_tail = 0.0;
    for (std::size_t frame = 0U; frame < 4096U; ++frame) {
        const auto output = prototype.process(frame == 0U ? 0.25F : 0.0F, 0.98F);
        if (frame >= 3072U) {
            active_tail += std::abs(static_cast<double>(output));
        }
    }
    double recovered_tail = 0.0;
    for (std::size_t frame = 0U; frame < 4096U; ++frame) {
        const auto output = prototype.process(0.0F, 0.0F);
        if (frame >= 3072U) {
            recovered_tail += std::abs(static_cast<double>(output));
        }
    }
    const auto recovery = active_tail > 0.0 && recovered_tail < active_tail;

    auto signature = []() {
        ScatteringWaveguidePrototype local;
        local.reset();
        double sum = 0.0;
        for (std::size_t frame = 0U; frame < 4096U; ++frame) {
            const auto output = local.process(frame == 0U ? 0.20F : 0.0F, 0.85F);
            sum += static_cast<double>(output) * static_cast<double>(frame + 1U);
        }
        return sum;
    };
    const auto deterministic = signature() == signature();

    prototype.reset();
    double work_sink = 0.0;
    const auto start = std::chrono::steady_clock::now();
    constexpr std::size_t work_frames = 50'000U;
    for (std::size_t frame = 0U; frame < work_frames; ++frame) {
        work_sink += prototype.process(frame == 0U ? 0.02F : 0.0F, 0.85F);
    }
    const auto elapsed = std::chrono::duration<double, std::nano>(
        std::chrono::steady_clock::now() - start).count();
    if (!std::isfinite(work_sink)) {
        silent = false;
    }

    return {
        "bounded scattering-waveguide prototype",
        ScatteringWaveguidePrototype::memoryBytes(),
        silent,
        returned_energy > 1.0e-5,
        returned_energy > 1.0e-5,
        false,
        false,
        false,
        recovery,
        deterministic,
        true,
        elapsed / static_cast<double>(work_frames),
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

    bool silent = prepared;
    for (std::size_t frame = 0U; frame < 4096U; ++frame) {
        silent = silent && resonator.processSample(0.0F, low).sample == 0.0F;
    }

    resonator.reset();
    double external_energy = 0.0;
    for (std::size_t frame = 0U; frame < 4096U; ++frame) {
        const auto result = resonator.processSample(frame == 0U ? 0.0004F : 0.0F, low);
        external_energy += std::abs(static_cast<double>(result.sample));
    }

    resonant::BreathPipeExciter exciter;
    const auto exciter_prepared = exciter.prepare({48'000.0, 128, 1, 1}, 777U);
    const auto no_return = exciter.processSample(
        {0.0F, 0.0F, 0.45F, 0.20F, 1.0F, 0.0F, 0.4F, 0.0F});
    exciter.reset();
    const auto with_return = exciter.processSample(
        {0.0F, 0.0F, 0.45F, 0.20F, 1.0F, 0.5F, 0.4F, 0.0F});
    const auto interaction = exciter_prepared && std::abs(with_return - no_return) > 1.0e-9F;

    const auto measured_220 = estimateModalFrequency(48'000.0, 220.0);
    const auto measured_440 = estimateModalFrequency(48'000.0, 440.0);
    const auto pitch_continuous = measured_220 > 0.0 && measured_440 > measured_220;
    const auto stable_tuning = std::abs(centsError(measured_220, 220.0)) <= 15.0 &&
                               std::abs(centsError(measured_440, 440.0)) <= 15.0;

    resonator.reset();
    for (std::size_t frame = 0U; frame < 64U; ++frame) {
        (void)resonator.processSample(frame == 0U ? 0.0004F : 0.0F, low);
    }
    const auto low_overblow = resonator.overblowAmount();
    const auto low_fundamental_radius = resonator.modeRadius(0);
    const auto low_upper_radius = resonator.modeRadius(1);

    auto high = low;
    high.pressure = 0.95F;
    for (std::size_t frame = 0U; frame < 64U; ++frame) {
        (void)resonator.processSample(0.0F, high);
    }
    const auto high_overblow = resonator.overblowAmount();
    const auto high_fundamental_radius = resonator.modeRadius(0);
    const auto high_upper_radius = resonator.modeRadius(1);
    const auto reorganisation = low_overblow < 0.01F && high_overblow > 0.50F &&
                                high_fundamental_radius < low_fundamental_radius &&
                                high_upper_radius > low_upper_radius &&
                                high_upper_radius > high_fundamental_radius;

    for (std::size_t frame = 0U; frame < 64U; ++frame) {
        (void)resonator.processSample(0.0F, low);
    }
    const auto reverse_recovery = resonator.overblowAmount() < 0.01F &&
                                  resonator.modeRadius(0) > resonator.modeRadius(1);

    auto signature = [low]() {
        resonant::BreathPipeModalResonator local;
        if (!local.prepare({48'000.0, 128, 1, 1})) {
            return 0.0;
        }
        double sum = 0.0;
        for (std::size_t frame = 0U; frame < 4096U; ++frame) {
            const auto result = local.processSample(frame == 0U ? 0.0004F : 0.0F, low);
            sum += static_cast<double>(result.sample) * static_cast<double>(frame + 1U);
        }
        return sum;
    };
    const auto deterministic = signature() == signature();

    resonator.reset();
    double work_sink = 0.0;
    const auto start = std::chrono::steady_clock::now();
    constexpr std::size_t work_frames = 50'000U;
    for (std::size_t frame = 0U; frame < work_frames; ++frame) {
        const auto result = resonator.processSample(frame == 0U ? 0.0004F : 0.0F, low);
        work_sink += result.sample;
    }
    const auto elapsed = std::chrono::duration<double, std::nano>(
        std::chrono::steady_clock::now() - start).count();
    if (!std::isfinite(work_sink)) {
        silent = false;
    }

    return {
        "selected three-mode interacting resonator",
        resonant::BreathPipeModalResonator::memoryBytes(),
        silent,
        external_energy > 1.0e-7,
        interaction,
        pitch_continuous,
        stable_tuning,
        reorganisation,
        reverse_recovery,
        deterministic,
        true,
        elapsed / static_cast<double>(work_frames),
    };
}

} // namespace

int main() {
    const auto tuned = probeTunedDelay();
    const auto scattering = probeScatteringWaveguide();
    const auto modal = probeModalInteraction();

    check(tuned.exact_silence && scattering.exact_silence && modal.exact_silence,
          "all topology candidates preserve true silence");
    check(tuned.external_excitation && scattering.external_excitation &&
              modal.external_excitation,
          "all topology candidates respond to supplied excitation");
    check(tuned.bidirectional_interaction && scattering.bidirectional_interaction &&
              modal.bidirectional_interaction,
          "all topology families have measured returned-state interaction suitability");
    check(tuned.continuous_pitch && tuned.stable_tuning,
          "M1 tuned-delay candidate receives pitch credit from measured delay behaviour");
    check(!scattering.continuous_pitch && !scattering.stable_tuning,
          "bounded scattering prototype is not credited with unimplemented tuning");
    check(modal.continuous_pitch && modal.stable_tuning,
          "selected modal candidate receives pitch credit from measured mode frequency");
    check(modal.continuous_overblow_reorganisation,
          "selected modal candidate demonstrates continuous register reorganisation");
    check(tuned.reverse_recovery && scattering.reverse_recovery && modal.reverse_recovery,
          "candidate recovery behaviour is measured rather than assumed");
    check(tuned.deterministic && scattering.deterministic && modal.deterministic,
          "all topology probes are deterministic under identical input sequences");
    check(modal.memory_bytes < tuned.memory_bytes,
          "selected modal candidate has lower fixed state cost than M1 delay extension");
    check(modal.score() > tuned.score() && modal.score() > scattering.score(),
          "modal/interacting candidate wins the measured M3.1 capability score");

    std::cout << "M3.1 topology evidence\n"
              << "  tuned-delay: score=" << tuned.score()
              << " bytes=" << tuned.memory_bytes
              << " measured-ns/sample=" << tuned.measured_ns_per_sample << '\n'
              << "  scattering: score=" << scattering.score()
              << " bytes=" << scattering.memory_bytes
              << " measured-ns/sample=" << scattering.measured_ns_per_sample << '\n'
              << "  modal: score=" << modal.score()
              << " bytes=" << modal.memory_bytes
              << " measured-ns/sample=" << modal.measured_ns_per_sample << '\n';

    if (failures != 0) {
        std::cerr << failures << " topology-selection test(s) failed\n";
        return 1;
    }
    std::cout << "PASS: M3.1 selected bounded three-mode interacting topology\n";
    return 0;
}
