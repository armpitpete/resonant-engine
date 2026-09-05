#pragma once

#include "resonant/Engine.hpp"
#include "resonant/Energy.hpp"
#include "resonant/Event.hpp"
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

// Model-specific continuously driven exciter for the M3 Breath Pipe reference
// voice. It intentionally exposes no MIDI or Host concepts. Pressure supplies
// energy; turbulence supplies stochastic broadband excitation; the returned
// resonator state perturbs the jet operating point so excitation is genuinely
// bidirectional rather than a noise source feeding a filter.
class BreathPipeExciter {
public:
    [[nodiscard]] bool prepare(const ProcessSpec& spec, Seed seed) noexcept {
        if (!spec.valid()) {
            return false;
        }
        seed_ = seed;
        rng_.seed(seed_);
        reset();
        return true;
    }

    void reset() noexcept {
        rng_.seed(seed_);
        previous_jet_ = 0.0F;
        jet_highpass_ = 0.0F;
    }

    void setSeed(Seed seed) noexcept {
        seed_ = seed;
        reset();
    }

    struct Input {
        Sample external_audio{0.0F};
        Sample external_amount{0.0F};
        Sample pressure{0.0F};
        Sample turbulence{0.0F};
        Sample interaction{0.0F};
        Sample returned_resonator{0.0F};
        Sample nonlinear_drive{0.0F};
        Sample trigger{0.0F};
    };

    [[nodiscard]] Sample processSample(const Input& input) noexcept {
        const auto pressure = clampFinite(input.pressure, 0.0F, 1.0F);
        const auto turbulence = clampFinite(input.turbulence, 0.0F, 1.0F);
        const auto interaction = clampFinite(input.interaction, 0.0F, 1.0F);
        const auto drive = clampFinite(input.nonlinear_drive, 0.0F, 1.0F);
        const auto returned = clampFinite(input.returned_resonator, -2.0F, 2.0F);
        const auto external = finiteOrZero(input.external_audio);
        const auto external_amount = clampFinite(input.external_amount, 0.0F, 1.0F);
        const auto trigger = clampFinite(input.trigger, -1.0F, 1.0F);

        // The steady pressure term is removed by the high-pass state below. It
        // therefore changes the flow operating point without creating a hidden
        // DC oscillator/source. Returned acoustic state changes the flow before
        // the nonlinearity, which is the key M3 bidirectional interaction.
        const auto jet_input = pressure - 0.75F * interaction * returned +
                               0.20F * external_amount * external;
        const auto jet = std::tanh((1.3F + 6.0F * drive) * jet_input);
        jet_highpass_ = (jet - previous_jet_) + 0.996F * jet_highpass_;
        previous_jet_ = jet;

        const auto white = rng_.nextSignedFloat();
        const auto air = pressure * (0.025F + 0.28F * turbulence) * white;
        const auto external_excitation = 0.20F * external_amount * external;
        const auto excitation = (air + 0.14F * jet_highpass_ + external_excitation +
                                 0.35F * trigger) *
                                0.00038F;
        return finiteOrZero(excitation);
    }

private:
    Seed seed_{kDefaultSeed};
    Pcg32 rng_{};
    Sample previous_jet_{0.0F};
    Sample jet_highpass_{0.0F};
};

// Bounded three-mode resonator selected for M3. The modes are resonant poles,
// not free-running oscillators: from reset they remain exactly silent until the
// exciter or external input supplies energy. Active regeneration changes modal
// loss inside the resonant topology. At high drive, loss is redistributed
// continuously from the fundamental toward upper modes, producing an overblow
// transition without switching models or substituting an oscillator.
class BreathPipeModalResonator {
public:
    static constexpr std::size_t kModeCount = 3U;

    struct Control {
        Sample tuning_hz{220.0F};
        Sample damping{0.12F};
        Sample pressure{0.0F};
        Sample interaction{0.55F};
        Sample regeneration{0.18F};
        Sample feedback_color{0.20F};
        Sample nonlinear_drive{0.30F};
        Sample timbre{0.25F};
    };

    struct Output {
        Sample sample{0.0F};
        Sample feedback_tap{0.0F};
        Sample overblow{0.0F};
    };

    [[nodiscard]] bool prepare(const ProcessSpec& spec) noexcept {
        if (!spec.valid()) {
            return false;
        }
        sample_rate_ = spec.sample_rate;
        minimum_tuning_hz_ = 48.0F;
        maximum_tuning_hz_ = std::min(2'000.0F,
                                      static_cast<Sample>(spec.sample_rate * 0.12));
        if (!(minimum_tuning_hz_ < maximum_tuning_hz_)) {
            return false;
        }
        reset();
        return true;
    }

    void reset() noexcept {
        state_1_.fill(0.0F);
        state_2_.fill(0.0F);
        coefficient_.fill(0.0F);
        radius_squared_.fill(0.0F);
        mode_radii_.fill(0.0F);
        coefficient_counter_ = 0U;
        last_feedback_tap_ = 0.0F;
        last_overblow_ = 0.0F;
        numerical_failure_ = false;
        updateCoefficients({});
    }

    [[nodiscard]] Output processSample(Sample excitation,
                                       const Control& raw_control) noexcept {
        const auto control = sanitize(raw_control);
        // Coefficients are control-rate quantities. Recomputing the three cosines
        // every eighth sample keeps movement effectively continuous while making
        // the declared four-voice WASM budget realistic. The resonant recursion
        // itself remains sample-by-sample.
        if (coefficient_counter_ == 0U) {
            updateCoefficients(control);
        }
        coefficient_counter_ = (coefficient_counter_ + 1U) & 7U;

        constexpr std::array<Sample, kModeCount> input_weights{{1.0F, 0.58F, 0.38F}};
        std::array<Sample, kModeCount> next{};
        const auto state_limit = 5.0F - 2.5F * control.nonlinear_drive;
        for (std::size_t mode = 0; mode < kModeCount; ++mode) {
            auto value = coefficient_[mode] * state_1_[mode] -
                         radius_squared_[mode] * state_2_[mode] +
                         input_weights[mode] * finiteOrZero(excitation);
            if (!std::isfinite(value)) {
                numerical_failure_ = true;
                value = 0.0F;
            }
            // Gentle bounded nonlinearity is inside the modal recursion, not a
            // post-output effect. Zero nonlinear drive leaves the modal recursion
            // exactly linear; bounded containment below still protects state.
            const auto denominator = 1.0F + control.nonlinear_drive * std::abs(value) /
                                              std::max(1.0F, state_limit * 20.0F);
            value /= denominator;
            next[mode] = EnergyMonitor::contain(value, 12.0F);
        }

        state_2_ = state_1_;
        state_1_ = next;

        const auto color = control.feedback_color;
        const std::array<Sample, kModeCount> feedback_weights{{
            1.0F - 0.25F * color,
            0.45F + 0.50F * color,
            0.20F + 0.65F * color,
        }};
        auto feedback = 0.0F;
        for (std::size_t mode = 0; mode < kModeCount; ++mode) {
            feedback += state_1_[mode] * feedback_weights[mode];
        }
        last_feedback_tap_ = EnergyMonitor::contain(feedback * 0.018F, 2.0F);

        const std::array<Sample, kModeCount> output_weights{{
            1.0F,
            0.50F + 0.35F * color,
            0.22F + 0.45F * color,
        }};
        auto mixed = 0.0F;
        for (std::size_t mode = 0; mode < kModeCount; ++mode) {
            mixed += state_1_[mode] * output_weights[mode];
        }
        mixed *= 0.45F;
        const auto shaped = std::tanh(mixed * (1.0F + 1.4F * control.nonlinear_drive));
        const auto sample = EnergyMonitor::contain(0.8F * shaped, 1.5F);
        return {sample, last_feedback_tap_, last_overblow_};
    }

    [[nodiscard]] Sample clampTuning(Sample tuning_hz) const noexcept {
        return clampFinite(tuning_hz, minimum_tuning_hz_, maximum_tuning_hz_,
                           std::clamp(220.0F, minimum_tuning_hz_, maximum_tuning_hz_));
    }

    [[nodiscard]] Sample minimumTuningHz() const noexcept { return minimum_tuning_hz_; }
    [[nodiscard]] Sample maximumTuningHz() const noexcept { return maximum_tuning_hz_; }
    [[nodiscard]] Sample feedbackTap() const noexcept { return last_feedback_tap_; }
    [[nodiscard]] Sample overblowAmount() const noexcept { return last_overblow_; }
    [[nodiscard]] bool numericalFailure() const noexcept { return numerical_failure_; }
    [[nodiscard]] Sample modeSample(std::size_t mode) const noexcept {
        return mode < kModeCount ? state_1_[mode] : 0.0F;
    }
    [[nodiscard]] Sample modeRadius(std::size_t mode) const noexcept {
        return mode < kModeCount ? mode_radii_[mode] : 0.0F;
    }
    [[nodiscard]] static constexpr std::size_t memoryBytes() noexcept {
        return sizeof(Sample) * kModeCount * 5U;
    }

private:
    [[nodiscard]] Control sanitize(Control control) const noexcept {
        control.tuning_hz = clampTuning(control.tuning_hz);
        control.damping = clampFinite(control.damping, 0.0F, 1.0F, 0.12F);
        control.pressure = clampFinite(control.pressure, 0.0F, 1.0F);
        control.interaction = clampFinite(control.interaction, 0.0F, 1.0F, 0.55F);
        control.regeneration = clampFinite(control.regeneration, 0.0F, 1.5F, 0.18F);
        control.feedback_color = clampFinite(control.feedback_color, 0.0F, 1.0F, 0.20F);
        control.nonlinear_drive = clampFinite(control.nonlinear_drive, 0.0F, 1.0F, 0.30F);
        control.timbre = clampFinite(control.timbre, 0.0F, 1.0F, 0.25F);
        return control;
    }

    void updateCoefficients(const Control& raw_control) noexcept {
        const auto control = sanitize(raw_control);
        const auto pressure_distance = std::clamp((control.pressure - 0.50F) / 0.42F,
                                                  0.0F, 1.0F);
        last_overblow_ = pressure_distance * pressure_distance *
                         (0.35F + 0.65F * control.interaction) *
                         (0.45F + 0.55F * control.nonlinear_drive);

        const auto active_boost = 0.00042F * control.regeneration;
        const auto base_radius = 0.99992F - 0.00240F * control.damping + active_boost;
        mode_radii_[0] = std::clamp(base_radius - 0.00320F * last_overblow_,
                                    0.992F, 1.00035F);
        mode_radii_[1] = std::clamp(base_radius - 0.00012F -
                                        0.00018F * control.damping +
                                        0.00028F * last_overblow_,
                                    0.992F, 1.00035F);
        mode_radii_[2] = std::clamp(base_radius - 0.00022F -
                                        0.00028F * control.damping +
                                        0.00020F * last_overblow_,
                                    0.992F, 1.00035F);

        const auto fundamental = static_cast<double>(control.tuning_hz);
        const auto timbre = static_cast<double>(control.timbre);
        std::array<double, kModeCount> frequencies{{
            fundamental,
            2.0 * fundamental * (1.0 + 0.0010 * timbre),
            3.0 * fundamental * (1.0 + 0.0025 * timbre),
        }};
        const auto maximum_frequency = sample_rate_ * 0.45;
        constexpr double two_pi = 6.28318530717958647692;
        for (std::size_t mode = 0; mode < kModeCount; ++mode) {
            frequencies[mode] = std::clamp(frequencies[mode], 20.0, maximum_frequency);
            const auto angle = two_pi * frequencies[mode] / sample_rate_;
            const auto radius = static_cast<double>(mode_radii_[mode]);
            coefficient_[mode] = static_cast<Sample>(2.0 * radius * std::cos(angle));
            radius_squared_[mode] = mode_radii_[mode] * mode_radii_[mode];
        }
    }

    std::array<Sample, kModeCount> state_1_{};
    std::array<Sample, kModeCount> state_2_{};
    std::array<Sample, kModeCount> coefficient_{};
    std::array<Sample, kModeCount> radius_squared_{};
    std::array<Sample, kModeCount> mode_radii_{};
    double sample_rate_{48'000.0};
    Sample minimum_tuning_hz_{48.0F};
    Sample maximum_tuning_hz_{2'000.0F};
    Sample last_feedback_tap_{0.0F};
    Sample last_overblow_{0.0F};
    std::uint32_t coefficient_counter_{0U};
    bool numerical_failure_{false};
};

class BreathPipeVoice {
public:
    static constexpr ParameterId kPitchHz = 201;
    static constexpr ParameterId kPressure = 202;
    static constexpr ParameterId kTurbulence = 203;
    static constexpr ParameterId kInteraction = 204;
    static constexpr ParameterId kDamping = 205;
    static constexpr ParameterId kRegeneration = 206;
    static constexpr ParameterId kFeedbackColor = 207;
    static constexpr ParameterId kNonlinearDrive = 208;
    static constexpr ParameterId kExternalAmount = 209;
    static constexpr ParameterId kTimbre = 210;

    inline static constexpr std::array<ParameterSpec, 10> kParameterSpecs{{
        {kPitchHz, "Pitch", "Hz", 48.0F, 2'000.0F, 220.0F,
         ParameterKind::Continuous, SmoothingMode::OnePole, 0.003, true},
        {kPressure, "Pressure", "", 0.0F, 1.0F, 0.0F,
         ParameterKind::Continuous, SmoothingMode::OnePole, 0.008, true},
        {kTurbulence, "Turbulence", "", 0.0F, 1.0F, 0.25F,
         ParameterKind::Continuous, SmoothingMode::OnePole, 0.012, true},
        {kInteraction, "Interaction", "", 0.0F, 1.0F, 0.55F,
         ParameterKind::Continuous, SmoothingMode::OnePole, 0.010, true},
        {kDamping, "Damping", "", 0.0F, 1.0F, 0.12F,
         ParameterKind::Continuous, SmoothingMode::OnePole, 0.012, true},
        {kRegeneration, "Regeneration", "", 0.0F, 1.5F, 0.18F,
         ParameterKind::Continuous, SmoothingMode::OnePole, 0.012, true},
        {kFeedbackColor, "Feedback colour", "", 0.0F, 1.0F, 0.20F,
         ParameterKind::Continuous, SmoothingMode::OnePole, 0.012, true},
        {kNonlinearDrive, "Nonlinear drive", "", 0.0F, 1.0F, 0.30F,
         ParameterKind::Continuous, SmoothingMode::OnePole, 0.010, true},
        {kExternalAmount, "External excitation", "", 0.0F, 1.0F, 0.50F,
         ParameterKind::Continuous, SmoothingMode::OnePole, 0.010, true},
        {kTimbre, "Timbre", "", 0.0F, 1.0F, 0.25F,
         ParameterKind::Continuous, SmoothingMode::OnePole, 0.012, true},
    }};

    [[nodiscard]] static constexpr std::span<const ParameterSpec>
    parameterSpecs() noexcept {
        return kParameterSpecs;
    }

    [[nodiscard]] static constexpr const ParameterSpec*
    parameterSpec(ParameterId id) noexcept {
        for (const auto& spec : kParameterSpecs) {
            if (spec.id == id) {
                return &spec;
            }
        }
        return nullptr;
    }

    explicit BreathPipeVoice(Seed seed = kDefaultSeed) noexcept : seed_(seed) {}

    [[nodiscard]] bool prepare(const ProcessSpec& spec) noexcept {
        if (!spec.valid() || !exciter_.prepare(spec, seed_) || !resonator_.prepare(spec)) {
            return false;
        }
        pitch_.prepare(spec.sample_rate, SmoothingMode::OnePole, 0.003);
        pressure_.prepare(spec.sample_rate, SmoothingMode::OnePole, 0.008);
        turbulence_.prepare(spec.sample_rate, SmoothingMode::OnePole, 0.012);
        interaction_.prepare(spec.sample_rate, SmoothingMode::OnePole, 0.010);
        damping_.prepare(spec.sample_rate, SmoothingMode::OnePole, 0.012);
        regeneration_.prepare(spec.sample_rate, SmoothingMode::OnePole, 0.012);
        feedback_color_.prepare(spec.sample_rate, SmoothingMode::OnePole, 0.012);
        nonlinear_drive_.prepare(spec.sample_rate, SmoothingMode::OnePole, 0.010);
        external_amount_.prepare(spec.sample_rate, SmoothingMode::OnePole, 0.010);
        timbre_.prepare(spec.sample_rate, SmoothingMode::OnePole, 0.012);
        energy_.prepare(512, 3.5F);
        reset();
        return true;
    }

    void reset() noexcept {
        exciter_.setSeed(seed_);
        resonator_.reset();
        pitch_.reset(resonator_.clampTuning(220.0F));
        pressure_.reset(0.0F);
        turbulence_.reset(0.25F);
        interaction_.reset(0.55F);
        damping_.reset(0.12F);
        regeneration_.reset(0.18F);
        feedback_color_.reset(0.20F);
        nonlinear_drive_.reset(0.30F);
        external_amount_.reset(0.50F);
        timbre_.reset(0.25F);
        for (std::size_t index = 0U; index < kParameterSpecs.size(); ++index) {
            persistent_parameter_values_[index] = kParameterSpecs[index].default_value;
        }
        pending_trigger_ = 0.0F;
        last_returned_ = 0.0F;
        energy_.reset();
    }

    void handleEvent(const Event& event) noexcept {
        const auto value = finiteOrZero(event.value);
        switch (event.type) {
        case EventType::Pitch:
            pitch_.setTarget(resonator_.clampTuning(value));
            break;
        case EventType::Pressure:
            pressure_.setTarget(clampFinite(value, 0.0F, 1.0F));
            break;
        case EventType::ParameterChange:
            handleParameter(event.target, value);
            break;
        case EventType::PerNoteExpression:
            timbre_.setTarget(clampFinite(value, 0.0F, 1.0F, 0.25F));
            break;
        case EventType::Trigger:
        case EventType::NoteOn:
            pending_trigger_ = std::clamp(pending_trigger_ + value, -1.0F, 1.0F);
            break;
        case EventType::NoteOff:
            break;
        }
    }

    [[nodiscard]] bool processSample(std::span<const Sample> input,
                                     std::span<Sample> output) noexcept {
        auto external = 0.0F;
        if (!input.empty()) {
            for (const auto sample : input) {
                external += finiteOrZero(sample);
            }
            external /= static_cast<Sample>(input.size());
        }

        const auto pitch = resonator_.clampTuning(pitch_.next());
        const auto pressure = clampFinite(pressure_.next(), 0.0F, 1.0F);
        const auto turbulence = clampFinite(turbulence_.next(), 0.0F, 1.0F, 0.25F);
        const auto interaction = clampFinite(interaction_.next(), 0.0F, 1.0F, 0.55F);
        const auto damping = clampFinite(damping_.next(), 0.0F, 1.0F, 0.12F);
        const auto regeneration = clampFinite(regeneration_.next(), 0.0F, 1.5F, 0.18F);
        const auto feedback_color = clampFinite(feedback_color_.next(), 0.0F, 1.0F, 0.20F);
        const auto nonlinear_drive = clampFinite(nonlinear_drive_.next(), 0.0F, 1.0F, 0.30F);
        const auto external_amount = clampFinite(external_amount_.next(), 0.0F, 1.0F, 0.50F);
        const auto timbre = clampFinite(timbre_.next(), 0.0F, 1.0F, 0.25F);

        const auto trigger = pending_trigger_;
        pending_trigger_ = 0.0F;
        const auto excitation = exciter_.processSample({
            external,
            external_amount,
            pressure,
            turbulence,
            interaction,
            last_returned_,
            nonlinear_drive,
            trigger,
        });
        const auto resonated = resonator_.processSample(
            excitation,
            {pitch, damping, pressure, interaction, regeneration,
             feedback_color, nonlinear_drive, timbre});
        last_returned_ = resonated.feedback_tap;
        const auto sample = EnergyMonitor::contain(resonated.sample, 1.5F);
        energy_.observe(excitation, resonated.sample, sample,
                        last_returned_ * regeneration);

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
    [[nodiscard]] const BreathPipeModalResonator& resonator() const noexcept {
        return resonator_;
    }
    [[nodiscard]] Sample currentPitchHz() const noexcept { return pitch_.current(); }
    [[nodiscard]] Sample currentPressure() const noexcept { return pressure_.current(); }
    [[nodiscard]] Sample currentRegeneration() const noexcept { return regeneration_.current(); }
    [[nodiscard]] Seed seed() const noexcept { return seed_; }

    [[nodiscard]] bool parameterTarget(ParameterId id, Sample& value) const noexcept {
        for (std::size_t index = 0U; index < kParameterSpecs.size(); ++index) {
            if (kParameterSpecs[index].id == id) {
                value = persistent_parameter_values_[index];
                return true;
            }
        }
        return false;
    }

    // State recall is a non-realtime lifecycle operation. It deliberately
    // clears transient resonator/exciter history, then restores canonical
    // parameter targets as hard state so recall cannot depend on whatever
    // audio the instance processed before the load. Ordinary automation still
    // uses handleParameter() and the existing core-owned smoothers.
    [[nodiscard]] bool restorePersistentState(
        Seed seed,
        std::span<const Sample> values) noexcept {
        if (values.size() != kParameterSpecs.size()) {
            return false;
        }
        for (std::size_t index = 0U; index < values.size(); ++index) {
            const auto& spec = kParameterSpecs[index];
            if (!std::isfinite(values[index]) ||
                values[index] < spec.minimum ||
                values[index] > spec.maximum) {
                return false;
            }
        }

        seed_ = seed;
        reset();
        for (std::size_t index = 0U; index < values.size(); ++index) {
            resetParameterTarget(kParameterSpecs[index].id, values[index]);
        }
        return true;
    }

private:
    void applyPersistentParameter(ParameterId target,
                                  Sample value,
                                  bool hard_reset) noexcept {
        Sample applied = value;
        ParameterSmoother* smoother = nullptr;

        switch (target) {
        case kPitchHz:
            applied = resonator_.clampTuning(value);
            smoother = &pitch_;
            break;
        case kPressure:
            applied = clampFinite(value, 0.0F, 1.0F);
            smoother = &pressure_;
            break;
        case kTurbulence:
            applied = clampFinite(value, 0.0F, 1.0F, 0.25F);
            smoother = &turbulence_;
            break;
        case kInteraction:
            applied = clampFinite(value, 0.0F, 1.0F, 0.55F);
            smoother = &interaction_;
            break;
        case kDamping:
            applied = clampFinite(value, 0.0F, 1.0F, 0.12F);
            smoother = &damping_;
            break;
        case kRegeneration:
            applied = clampFinite(value, 0.0F, 1.5F, 0.18F);
            smoother = &regeneration_;
            break;
        case kFeedbackColor:
            applied = clampFinite(value, 0.0F, 1.0F, 0.20F);
            smoother = &feedback_color_;
            break;
        case kNonlinearDrive:
            applied = clampFinite(value, 0.0F, 1.0F, 0.30F);
            smoother = &nonlinear_drive_;
            break;
        case kExternalAmount:
            applied = clampFinite(value, 0.0F, 1.0F, 0.50F);
            smoother = &external_amount_;
            break;
        case kTimbre:
            applied = clampFinite(value, 0.0F, 1.0F, 0.25F);
            smoother = &timbre_;
            break;
        default:
            return;
        }

        if (hard_reset) {
            smoother->reset(applied);
        } else {
            smoother->setTarget(applied);
        }

        for (std::size_t index = 0U; index < kParameterSpecs.size(); ++index) {
            if (kParameterSpecs[index].id == target) {
                persistent_parameter_values_[index] = applied;
                return;
            }
        }
    }

    void resetParameterTarget(ParameterId target, Sample value) noexcept {
        applyPersistentParameter(target, value, true);
    }

    void handleParameter(ParameterId target, Sample value) noexcept {
        applyPersistentParameter(target, value, false);
    }

    Seed seed_{kDefaultSeed};
    BreathPipeExciter exciter_{};
    BreathPipeModalResonator resonator_{};
    ParameterSmoother pitch_{};
    ParameterSmoother pressure_{};
    ParameterSmoother turbulence_{};
    ParameterSmoother interaction_{};
    ParameterSmoother damping_{};
    ParameterSmoother regeneration_{};
    ParameterSmoother feedback_color_{};
    ParameterSmoother nonlinear_drive_{};
    ParameterSmoother external_amount_{};
    ParameterSmoother timbre_{};
    std::array<Sample, kParameterSpecs.size()> persistent_parameter_values_{};
    EnergyMonitor energy_{};
    Sample pending_trigger_{0.0F};
    Sample last_returned_{0.0F};
};

static_assert(EngineModel<BreathPipeVoice>);

} // namespace resonant
