#pragma once

#include "resonant/Energy.hpp"
#include "resonant/Event.hpp"
#include "resonant/Interfaces.hpp"
#include "resonant/Parameter.hpp"
#include "resonant/Random.hpp"
#include "resonant/Types.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <span>

namespace resonant {

// M1 reference exciter. This is deliberately generic: deterministic noise,
// arbitrary external audio, transient excitation and a bounded returned-state
// interaction. It is not a reed/jet model and does not define Breath Pipe.
class ContinuousNoiseExciter {
public:
    [[nodiscard]] bool prepare(const ProcessSpec& spec, Seed seed) noexcept {
        if (!spec.valid()) {
            return false;
        }
        seed_ = seed;
        rng_.seed(seed_);
        noise_state_ = 0.0F;
        return true;
    }

    void reset() noexcept {
        rng_.seed(seed_);
        noise_state_ = 0.0F;
    }

    void handleEvent(const Event&) noexcept {}

    [[nodiscard]] ExciterOutput processSample(const ExciterInput& input) noexcept {
        const auto external = finiteOrZero(input.external_audio);
        const auto pressure = clampFinite(input.pressure, 0.0F, 1.0F);
        const auto turbulence = clampFinite(input.turbulence, 0.0F, 1.0F);
        const auto trigger = clampFinite(input.trigger, -1.0F, 1.0F);
        const auto returned = clampFinite(input.returned_resonator, -4.0F, 4.0F);

        const auto white = rng_.nextSignedFloat();
        noise_state_ += 0.22F * (white - noise_state_);
        const auto air = pressure *
                         ((1.0F - turbulence) * 0.06F * noise_state_ +
                          turbulence * 0.24F * white);
        const auto sample = external + trigger + air + 0.10F * returned;
        return {sample, sample * sample};
    }

private:
    Seed seed_{kDefaultSeed};
    Pcg32 rng_{};
    Sample noise_state_{0.0F};
};

// M1's first real resonator: a fixed-capacity tuned delay with linear
// fractional-delay interpolation, frequency-dependent loss, controllable
// regeneration and optional bounded nonlinearity in the loop.
//
// It is intentionally one concrete Resonator implementation, not the generic
// Resonator contract and not a claim that all future models are delay based.
class TunedDelayResonator {
public:
    static constexpr std::size_t kDelayCapacity = 16'384;

    [[nodiscard]] bool prepare(const ProcessSpec& spec) noexcept {
        if (!spec.valid()) {
            return false;
        }
        sample_rate_ = spec.sample_rate;
        minimum_tuning_hz_ = std::max(
            24.0F,
            static_cast<Sample>(sample_rate_ /
                                static_cast<double>(kDelayCapacity - 2U)));
        maximum_tuning_hz_ = std::min(
            8'000.0F, static_cast<Sample>(sample_rate_ * 0.45));
        if (!(minimum_tuning_hz_ < maximum_tuning_hz_)) {
            return false;
        }
        reset();
        return true;
    }

    void reset() noexcept {
        delay_.fill(0.0F);
        write_index_ = 0U;
        damping_state_ = 0.0F;
        last_delay_samples_ = 0.0;
        last_loop_gain_ = 0.0F;
        numerical_failure_ = false;
    }

    void handleEvent(const Event&) noexcept {}

    [[nodiscard]] ResonatorOutput processSample(const ResonatorInput& input) noexcept {
        const auto excitation = finiteOrZero(input.excitation);
        const auto tuning_hz = clampTuning(input.control.tuning_hz);
        const auto damping = clampFinite(input.control.damping, 0.0F, 1.0F, 0.25F);
        const auto feedback = clampFinite(input.control.feedback, 0.0F, 1.5F);
        const auto nonlinearity = clampFinite(input.control.nonlinearity, 0.0F, 1.0F);

        auto delay_samples = sample_rate_ / static_cast<double>(tuning_hz);
        delay_samples = std::clamp(
            delay_samples, 2.0, static_cast<double>(kDelayCapacity - 2U));
        last_delay_samples_ = delay_samples;

        const auto resonated = readFractional(delay_samples);
        if (!std::isfinite(resonated)) {
            numerical_failure_ = true;
            return {};
        }

        // damping=0 leaves the loop broadband; increasing damping lowers the
        // one-pole bandwidth and increases passive loss. This deliberately
        // exposes the phase/tuning interaction rather than hiding it in Host code.
        const auto damping_coefficient = 1.0F - 0.92F * damping;
        damping_state_ += damping_coefficient * (resonated - damping_state_);

        const auto passive_gain = 0.9995F - 0.08F * damping;
        const auto active_gain = 1.0F + 0.08F * feedback;
        last_loop_gain_ = passive_gain * active_gain;

        const auto linear_loop = damping_state_ * last_loop_gain_;
        const auto saturated_loop = softSaturate(linear_loop);
        const auto returned = linear_loop +
                              nonlinearity * (saturated_loop - linear_loop);
        auto next = excitation + returned;

        if (!std::isfinite(next)) {
            numerical_failure_ = true;
            next = 0.0F;
        } else {
            // Emergency finite-state containment is separate from the musical
            // nonlinearity above. Aggressive finite operation remains legal.
            next = EnergyMonitor::contain(next, 4.0F);
        }

        delay_[write_index_] = next;
        write_index_ = (write_index_ + 1U) % kDelayCapacity;

        const auto output = EnergyMonitor::contain(resonated, 2.0F);
        return {output, damping_state_, output * output};
    }

    [[nodiscard]] Sample clampTuning(Sample tuning_hz) const noexcept {
        return clampFinite(tuning_hz, minimum_tuning_hz_, maximum_tuning_hz_,
                           std::clamp(220.0F, minimum_tuning_hz_, maximum_tuning_hz_));
    }

    [[nodiscard]] Sample minimumTuningHz() const noexcept { return minimum_tuning_hz_; }
    [[nodiscard]] Sample maximumTuningHz() const noexcept { return maximum_tuning_hz_; }
    [[nodiscard]] double delaySamples() const noexcept { return last_delay_samples_; }
    [[nodiscard]] Sample loopGain() const noexcept { return last_loop_gain_; }
    [[nodiscard]] bool numericalFailure() const noexcept { return numerical_failure_; }
    [[nodiscard]] static constexpr std::size_t memoryBytes() noexcept {
        return sizeof(Sample) * kDelayCapacity;
    }

private:
    [[nodiscard]] Sample readFractional(double delay_samples) const noexcept {
        auto read_position = static_cast<double>(write_index_) - delay_samples;
        if (read_position < 0.0) {
            read_position += static_cast<double>(kDelayCapacity);
        }
        const auto index0 = static_cast<std::size_t>(std::floor(read_position));
        const auto index1 = (index0 + 1U) % kDelayCapacity;
        const auto fraction = static_cast<Sample>(
            read_position - static_cast<double>(index0));
        return delay_[index0] + fraction * (delay_[index1] - delay_[index0]);
    }

    [[nodiscard]] static Sample softSaturate(Sample value) noexcept {
        return value / (1.0F + std::abs(value));
    }

    std::array<Sample, kDelayCapacity> delay_{};
    double sample_rate_{48'000.0};
    double last_delay_samples_{0.0};
    std::size_t write_index_{0U};
    Sample minimum_tuning_hz_{24.0F};
    Sample maximum_tuning_hz_{8'000.0F};
    Sample damping_state_{0.0F};
    Sample last_loop_gain_{0.0F};
    bool numerical_failure_{false};
};

class FirstResonatorVoice {
public:
    static constexpr ParameterId kTuningHz = 101;
    static constexpr ParameterId kDamping = 102;
    static constexpr ParameterId kFeedback = 103;
    static constexpr ParameterId kNonlinearity = 104;
    static constexpr ParameterId kExcitation = 105;
    static constexpr ParameterId kTurbulence = 106;
    static constexpr ParameterId kInteraction = 107;

    explicit FirstResonatorVoice(Seed seed = kDefaultSeed) noexcept : seed_(seed) {}

    [[nodiscard]] bool prepare(const ProcessSpec& spec) noexcept {
        if (!spec.valid() || !exciter_.prepare(spec, seed_) ||
            !resonator_.prepare(spec)) {
            return false;
        }

        tuning_.prepare(spec.sample_rate, SmoothingMode::OnePole, 0.002);
        damping_.prepare(spec.sample_rate, SmoothingMode::OnePole, 0.010);
        feedback_.prepare(spec.sample_rate, SmoothingMode::OnePole, 0.010);
        nonlinearity_.prepare(spec.sample_rate, SmoothingMode::OnePole, 0.010);
        excitation_.prepare(spec.sample_rate, SmoothingMode::OnePole, 0.005);
        turbulence_.prepare(spec.sample_rate, SmoothingMode::OnePole, 0.010);
        interaction_.prepare(spec.sample_rate, SmoothingMode::OnePole, 0.010);
        energy_.prepare(512, 3.5F);
        reset();
        return true;
    }

    void reset() noexcept {
        exciter_.reset();
        resonator_.reset();
        tuning_.reset(resonator_.clampTuning(220.0F));
        damping_.reset(0.30F);
        feedback_.reset(0.0F);
        nonlinearity_.reset(0.20F);
        excitation_.reset(0.0F);
        turbulence_.reset(0.65F);
        interaction_.reset(0.0F);
        pending_trigger_ = 0.0F;
        last_returned_ = 0.0F;
        energy_.reset();
    }

    void handleEvent(const Event& event) noexcept {
        const auto value = finiteOrZero(event.value);
        switch (event.type) {
        case EventType::Pitch:
            tuning_.setTarget(resonator_.clampTuning(value));
            break;
        case EventType::Pressure:
            excitation_.setTarget(clampFinite(value, 0.0F, 1.0F));
            break;
        case EventType::ParameterChange:
            handleParameter(event.target, value);
            break;
        case EventType::Trigger:
        case EventType::NoteOn:
            pending_trigger_ = std::clamp(pending_trigger_ + value, -1.0F, 1.0F);
            break;
        case EventType::NoteOff:
        case EventType::PerNoteExpression:
            break;
        }
    }

    [[nodiscard]] bool processSample(std::span<const Sample> input,
                                     std::span<Sample> output) noexcept {
        Sample external = 0.0F;
        if (!input.empty()) {
            for (const auto sample : input) {
                external += finiteOrZero(sample);
            }
            external /= static_cast<Sample>(input.size());
        }

        const auto tuning_hz = resonator_.clampTuning(tuning_.next());
        const auto damping = clampFinite(damping_.next(), 0.0F, 1.0F, 0.30F);
        const auto feedback = clampFinite(feedback_.next(), 0.0F, 1.5F);
        const auto nonlinearity = clampFinite(nonlinearity_.next(), 0.0F, 1.0F, 0.20F);
        const auto excitation = clampFinite(excitation_.next(), 0.0F, 1.0F);
        const auto turbulence = clampFinite(turbulence_.next(), 0.0F, 1.0F, 0.65F);
        const auto interaction = clampFinite(interaction_.next(), 0.0F, 1.0F);

        const auto trigger = pending_trigger_;
        pending_trigger_ = 0.0F;

        const auto exciter_output = exciter_.processSample(
            {external, excitation, turbulence, trigger,
             last_returned_ * interaction});
        const auto resonator_output = resonator_.processSample(
            {exciter_output.sample,
             {tuning_hz, damping, feedback, nonlinearity}, {}});
        last_returned_ = resonator_output.feedback_tap;

        const auto sample = EnergyMonitor::contain(0.75F * resonator_output.sample, 1.5F);
        energy_.observe(exciter_output.sample, resonator_output.sample, sample,
                        last_returned_ * feedback);

        const auto& diagnostics = energy_.diagnostics();
        if (resonator_.numericalFailure() || diagnostics.nan_detected ||
            diagnostics.infinity_detected) {
            for (auto& channel : output) {
                channel = 0.0F;
            }
            return false;
        }

        for (auto& channel : output) {
            channel = sample;
        }
        return true;
    }

    [[nodiscard]] const EnergyDiagnostics& diagnostics() const noexcept {
        return energy_.diagnostics();
    }
    [[nodiscard]] const TunedDelayResonator& resonator() const noexcept {
        return resonator_;
    }
    [[nodiscard]] Sample currentTuningHz() const noexcept { return tuning_.current(); }
    [[nodiscard]] Sample currentExcitation() const noexcept { return excitation_.current(); }
    [[nodiscard]] Seed seed() const noexcept { return seed_; }

private:
    void handleParameter(ParameterId target, Sample value) noexcept {
        switch (target) {
        case kTuningHz:
            tuning_.setTarget(resonator_.clampTuning(value));
            break;
        case kDamping:
            damping_.setTarget(clampFinite(value, 0.0F, 1.0F, 0.30F));
            break;
        case kFeedback:
            feedback_.setTarget(clampFinite(value, 0.0F, 1.5F));
            break;
        case kNonlinearity:
            nonlinearity_.setTarget(clampFinite(value, 0.0F, 1.0F, 0.20F));
            break;
        case kExcitation:
            excitation_.setTarget(clampFinite(value, 0.0F, 1.0F));
            break;
        case kTurbulence:
            turbulence_.setTarget(clampFinite(value, 0.0F, 1.0F, 0.65F));
            break;
        case kInteraction:
            interaction_.setTarget(clampFinite(value, 0.0F, 1.0F));
            break;
        default:
            break;
        }
    }

    Seed seed_{kDefaultSeed};
    ContinuousNoiseExciter exciter_{};
    TunedDelayResonator resonator_{};
    ParameterSmoother tuning_{};
    ParameterSmoother damping_{};
    ParameterSmoother feedback_{};
    ParameterSmoother nonlinearity_{};
    ParameterSmoother excitation_{};
    ParameterSmoother turbulence_{};
    ParameterSmoother interaction_{};
    EnergyMonitor energy_{};
    Sample pending_trigger_{0.0F};
    Sample last_returned_{0.0F};
};

static_assert(Exciter<ContinuousNoiseExciter>);
static_assert(Resonator<TunedDelayResonator>);

} // namespace resonant
