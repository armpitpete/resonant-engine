#pragma once

#include "resonant/BreathPipe.hpp"
#include "resonant/Engine.hpp"
#include "resonant/Parameter.hpp"
#include "resonant/Random.hpp"
#include "resonant/Types.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>

namespace resonant_lab {

enum class BreathPipeStabilityState : std::uint32_t {
    Quiet = 0,
    Active = 1,
    HighEnergy = 2,
    SelfOscillating = 3,
    NearLimit = 4,
    Unstable = 5,
    Protected = 6,
};

struct BreathPipeTelemetry {
    float resonator_energy{0.0F};
    float core_output_rms{0.0F};
    float core_peak{0.0F};
    float overblow_amount{0.0F};
    std::array<float, resonant::BreathPipeModalResonator::kModeCount> mode_energy{};
    BreathPipeStabilityState stability{BreathPipeStabilityState::Quiet};
    std::uint32_t active_voices{0};
    std::uint32_t held_voices{0};
    std::uint32_t max_active_voices{0};
    std::uint32_t maximum_polyphony{0};
    std::uint32_t voice_steals{0};
    bool protected_state{false};
};

// M3 extends the M2 Lab without replacing or rewriting LabEngine. This adapter
// uses the same bounded eight-voice test-host policy but instantiates the M3
// BreathPipeVoice and provides a deterministic Host-side external-audio probe.
class BreathPipeLabEngine {
public:
    static constexpr std::uint32_t kMaximumPolyphony = 8;
    static constexpr std::uint32_t kMaximumBlockSize = 128;
    static constexpr resonant::ParameterId kExternalProbeLevel = 901;

    inline static constexpr std::array<resonant::ParameterSpec, 11> kParameterSpecs{{
        {resonant::BreathPipeVoice::kPitchHz, "Pitch", "Hz", 48.0F, 2'000.0F,
         220.0F, resonant::ParameterKind::Continuous, resonant::SmoothingMode::OnePole,
         0.003, true},
        {resonant::BreathPipeVoice::kPressure, "Pressure", "", 0.0F, 1.0F,
         0.0F, resonant::ParameterKind::Continuous, resonant::SmoothingMode::OnePole,
         0.008, true},
        {resonant::BreathPipeVoice::kTurbulence, "Turbulence", "", 0.0F, 1.0F,
         0.25F, resonant::ParameterKind::Continuous, resonant::SmoothingMode::OnePole,
         0.012, true},
        {resonant::BreathPipeVoice::kInteraction, "Interaction", "", 0.0F, 1.0F,
         0.55F, resonant::ParameterKind::Continuous, resonant::SmoothingMode::OnePole,
         0.010, true},
        {resonant::BreathPipeVoice::kDamping, "Damping", "", 0.0F, 1.0F,
         0.12F, resonant::ParameterKind::Continuous, resonant::SmoothingMode::OnePole,
         0.012, true},
        {resonant::BreathPipeVoice::kRegeneration, "Regeneration", "", 0.0F, 1.5F,
         0.18F, resonant::ParameterKind::Continuous, resonant::SmoothingMode::OnePole,
         0.012, true},
        {resonant::BreathPipeVoice::kFeedbackColor, "Feedback colour", "", 0.0F, 1.0F,
         0.20F, resonant::ParameterKind::Continuous, resonant::SmoothingMode::OnePole,
         0.012, true},
        {resonant::BreathPipeVoice::kNonlinearDrive, "Nonlinear drive", "", 0.0F, 1.0F,
         0.30F, resonant::ParameterKind::Continuous, resonant::SmoothingMode::OnePole,
         0.010, true},
        {resonant::BreathPipeVoice::kExternalAmount, "External excitation", "", 0.0F, 1.0F,
         0.50F, resonant::ParameterKind::Continuous, resonant::SmoothingMode::OnePole,
         0.010, true},
        {resonant::BreathPipeVoice::kTimbre, "Timbre", "", 0.0F, 1.0F,
         0.25F, resonant::ParameterKind::Continuous, resonant::SmoothingMode::OnePole,
         0.012, true},
        {kExternalProbeLevel, "Lab external-audio probe", "", 0.0F, 1.0F,
         0.0F, resonant::ParameterKind::Internal, resonant::SmoothingMode::None,
         0.0, true},
    }};

    BreathPipeLabEngine() noexcept
        : voices_{{
              VoiceSlot{resonant::deriveVoiceSeed(resonant::kDefaultSeed, 0)},
              VoiceSlot{resonant::deriveVoiceSeed(resonant::kDefaultSeed, 1)},
              VoiceSlot{resonant::deriveVoiceSeed(resonant::kDefaultSeed, 2)},
              VoiceSlot{resonant::deriveVoiceSeed(resonant::kDefaultSeed, 3)},
              VoiceSlot{resonant::deriveVoiceSeed(resonant::kDefaultSeed, 4)},
              VoiceSlot{resonant::deriveVoiceSeed(resonant::kDefaultSeed, 5)},
              VoiceSlot{resonant::deriveVoiceSeed(resonant::kDefaultSeed, 6)},
              VoiceSlot{resonant::deriveVoiceSeed(resonant::kDefaultSeed, 7)},
          }} {
        restoreParameterDefaults();
        telemetry_.maximum_polyphony = kMaximumPolyphony;
    }

    [[nodiscard]] bool prepare(double sample_rate,
                               std::uint32_t maximum_block_size = kMaximumBlockSize) noexcept {
        if (!std::isfinite(sample_rate) || maximum_block_size == 0U ||
            maximum_block_size > kMaximumBlockSize) {
            prepared_ = false;
            return false;
        }
        for (auto& voice : voices_) {
            if (!voice.engine.prepare({sample_rate, maximum_block_size, 1, 1})) {
                prepared_ = false;
                return false;
            }
        }
        sample_rate_ = sample_rate;
        maximum_block_size_ = maximum_block_size;
        prepared_ = true;
        reset();
        return true;
    }

    void reset() noexcept {
        restoreParameterDefaults();
        clearVoices();
        external_probe_rng_.seed(resonant::deriveVoiceSeed(resonant::kDefaultSeed, 91));
        age_counter_ = 0;
        voice_steals_ = 0;
        max_active_voices_ = 0;
        protected_state_ = false;
        telemetry_ = {};
        telemetry_.maximum_polyphony = kMaximumPolyphony;
    }

    void panic() noexcept {
        clearVoices();
        protected_state_ = false;
        telemetry_ = {};
        telemetry_.maximum_polyphony = kMaximumPolyphony;
        telemetry_.max_active_voices = max_active_voices_;
        telemetry_.voice_steals = voice_steals_;
    }

    [[nodiscard]] bool setParameter(resonant::ParameterId id, float value) noexcept {
        const auto index = parameterIndex(id);
        if (index == kInvalidIndex) {
            return false;
        }
        const auto clamped = kParameterSpecs[index].clamp(value);
        parameter_values_[index] = clamped;
        if (id == kExternalProbeLevel) {
            return true;
        }
        for (auto& voice : voices_) {
            if (!voice.engaged) {
                continue;
            }
            if (id == resonant::BreathPipeVoice::kPressure && !voice.held) {
                continue;
            }
            dispatchParameter(voice, id, clamped);
        }
        return true;
    }

    [[nodiscard]] float parameterValue(resonant::ParameterId id) const noexcept {
        const auto index = parameterIndex(id);
        return index == kInvalidIndex ? 0.0F : parameter_values_[index];
    }

    [[nodiscard]] bool noteOn(std::uint32_t midi_note, float velocity) noexcept {
        if (!prepared_ || midi_note > 127U) {
            return false;
        }
        auto& voice = selectVoice();
        if (voice.engaged) {
            ++voice_steals_;
        }
        (void)voice.engine.reset();
        voice.engaged = true;
        voice.held = true;
        voice.note = midi_note;
        voice.age = ++age_counter_;
        voice.quiet_blocks = 0U;

        applyStoredParameters(voice);
        dispatch(voice, resonant::EventType::Pitch, 0,
                 static_cast<float>(midiToHz(midi_note)));
        const auto base_pressure = parameterValue(resonant::BreathPipeVoice::kPressure);
        const auto velocity_pressure = base_pressure *
                                       (0.25F + 0.75F * std::clamp(velocity, 0.0F, 1.0F));
        dispatch(voice, resonant::EventType::Pressure, 0, velocity_pressure);
        dispatch(voice, resonant::EventType::NoteOn, 0,
                 std::clamp(velocity, 0.0F, 1.0F));
        updateVoiceCounts();
        return true;
    }

    [[nodiscard]] bool noteOff(std::uint32_t midi_note) noexcept {
        VoiceSlot* selected = nullptr;
        for (auto& voice : voices_) {
            if (voice.engaged && voice.held && voice.note == midi_note &&
                (selected == nullptr || voice.age > selected->age)) {
                selected = &voice;
            }
        }
        if (selected == nullptr) {
            return false;
        }
        dispatch(*selected, resonant::EventType::Pressure, 0, 0.0F);
        dispatch(*selected, resonant::EventType::NoteOff, 0, 0.0F);
        selected->held = false;
        selected->quiet_blocks = 0U;
        updateVoiceCounts();
        return true;
    }

    [[nodiscard]] bool process(std::span<resonant::Sample> output) noexcept {
        if (!prepared_ || output.empty() || output.size() > maximum_block_size_) {
            std::fill(output.begin(), output.end(), 0.0F);
            return false;
        }
        std::fill(output.begin(), output.end(), 0.0F);
        const auto external_level = parameterValue(kExternalProbeLevel);
        for (std::size_t frame = 0; frame < output.size(); ++frame) {
            // Deterministic Host-side broadband source. This is deliberately not
            // synthesis inside BreathPipeVoice; it exercises Engine input audio.
            external_buffer_[frame] = external_level * 0.20F *
                                      external_probe_rng_.nextSignedFloat();
        }

        std::array<float, kMaximumBlockSize> raw_mix{};
        std::uint32_t processed_voices = 0U;
        double resonator_energy_sum = 0.0;
        double output_rms_square_sum = 0.0;
        std::array<double, resonant::BreathPipeModalResonator::kModeCount> mode_energy_sum{};
        float core_peak = 0.0F;
        float overblow_max = 0.0F;
        BreathPipeStabilityState worst_state = BreathPipeStabilityState::Quiet;
        bool failed = false;

        for (std::size_t voice_index = 0; voice_index < voices_.size(); ++voice_index) {
            auto& voice = voices_[voice_index];
            if (!voice.engaged) {
                continue;
            }
            ++processed_voices;
            auto& buffer = voice_buffers_[voice_index];
            std::fill_n(buffer.begin(), output.size(), 0.0F);
            const resonant::Sample* inputs[1]{external_buffer_.data()};
            resonant::Sample* outputs[1]{buffer.data()};
            const resonant::AudioBlockView block{inputs, outputs, 1, 1,
                                                  static_cast<std::uint32_t>(output.size())};
            const auto status = voice.engine.process(block);
            if (status != resonant::ProcessStatus::Ok) {
                failed = true;
            }

            for (std::size_t frame = 0; frame < output.size(); ++frame) {
                raw_mix[frame] += resonant::finiteOrZero(buffer[frame]);
            }

            const auto& diagnostics = voice.engine.model().diagnostics();
            resonator_energy_sum += diagnostics.resonator_rms * diagnostics.resonator_rms;
            output_rms_square_sum += diagnostics.output_rms * diagnostics.output_rms;
            core_peak = std::max(core_peak, diagnostics.peak);
            worst_state = std::max(worst_state, classify(diagnostics));
            const auto& resonator = voice.engine.model().resonator();
            overblow_max = std::max(overblow_max, resonator.overblowAmount());
            for (std::size_t mode = 0; mode < mode_energy_sum.size(); ++mode) {
                const auto sample = static_cast<double>(resonator.modeSample(mode));
                mode_energy_sum[mode] += sample * sample;
            }

            if (!voice.held) {
                if (diagnostics.state == resonant::EnergyState::Silent &&
                    diagnostics.output_rms < 1.0e-7) {
                    ++voice.quiet_blocks;
                    if (voice.quiet_blocks >= 16U) {
                        voice.engaged = false;
                        voice.quiet_blocks = 0U;
                        (void)voice.engine.reset();
                    }
                } else {
                    voice.quiet_blocks = 0U;
                }
            }
        }

        float raw_peak = 0.0F;
        for (const auto sample : std::span<const float>{raw_mix.data(), output.size()}) {
            raw_peak = std::max(raw_peak, std::abs(sample));
        }
        core_peak = std::max(core_peak, raw_peak);
        if (raw_peak > 4.0F && worst_state < BreathPipeStabilityState::NearLimit) {
            worst_state = BreathPipeStabilityState::NearLimit;
        }
        if (failed) {
            protected_state_ = true;
        }

        if (!protected_state_) {
            const auto mix_gain = processed_voices == 0U
                                      ? 0.0F
                                      : 0.60F / std::sqrt(static_cast<float>(processed_voices));
            for (std::size_t frame = 0; frame < output.size(); ++frame) {
                output[frame] = std::clamp(raw_mix[frame] * mix_gain, -1.0F, 1.0F);
            }
        }

        updateVoiceCounts();
        telemetry_.resonator_energy = processed_voices == 0U
                                          ? 0.0F
                                          : static_cast<float>(resonator_energy_sum /
                                                               static_cast<double>(processed_voices));
        telemetry_.core_output_rms = processed_voices == 0U
                                         ? 0.0F
                                         : static_cast<float>(std::sqrt(
                                               output_rms_square_sum /
                                               static_cast<double>(processed_voices)));
        telemetry_.core_peak = core_peak;
        telemetry_.overblow_amount = overblow_max;
        for (std::size_t mode = 0; mode < mode_energy_sum.size(); ++mode) {
            telemetry_.mode_energy[mode] = processed_voices == 0U
                                                ? 0.0F
                                                : static_cast<float>(mode_energy_sum[mode] /
                                                                     static_cast<double>(processed_voices));
        }
        telemetry_.stability = protected_state_ ? BreathPipeStabilityState::Protected : worst_state;
        telemetry_.max_active_voices = max_active_voices_;
        telemetry_.maximum_polyphony = kMaximumPolyphony;
        telemetry_.voice_steals = voice_steals_;
        telemetry_.protected_state = protected_state_;
        return !protected_state_;
    }

    [[nodiscard]] const BreathPipeTelemetry& telemetry() const noexcept { return telemetry_; }
    [[nodiscard]] bool prepared() const noexcept { return prepared_; }
    [[nodiscard]] double sampleRate() const noexcept { return sample_rate_; }

    [[nodiscard]] static constexpr std::span<const resonant::ParameterSpec>
    parameterSpecs() noexcept {
        return kParameterSpecs;
    }

    [[nodiscard]] static double midiToHz(std::uint32_t midi_note) noexcept {
        return 440.0 * std::pow(2.0, (static_cast<double>(midi_note) - 69.0) / 12.0);
    }

private:
    struct VoiceSlot {
        explicit VoiceSlot(resonant::Seed seed) noexcept
            : engine{resonant::BreathPipeVoice{seed}} {}

        resonant::Engine<resonant::BreathPipeVoice> engine;
        std::uint64_t age{0};
        std::uint32_t note{0};
        std::uint32_t quiet_blocks{0};
        bool engaged{false};
        bool held{false};
    };

    static constexpr std::size_t kInvalidIndex = std::numeric_limits<std::size_t>::max();

    [[nodiscard]] static std::size_t parameterIndex(resonant::ParameterId id) noexcept {
        for (std::size_t index = 0; index < kParameterSpecs.size(); ++index) {
            if (kParameterSpecs[index].id == id) {
                return index;
            }
        }
        return kInvalidIndex;
    }

    void restoreParameterDefaults() noexcept {
        for (std::size_t index = 0; index < kParameterSpecs.size(); ++index) {
            parameter_values_[index] = kParameterSpecs[index].default_value;
        }
    }

    void clearVoices() noexcept {
        for (auto& voice : voices_) {
            if (prepared_) {
                (void)voice.engine.reset();
            }
            voice.age = 0;
            voice.note = 0;
            voice.quiet_blocks = 0;
            voice.engaged = false;
            voice.held = false;
        }
    }

    [[nodiscard]] VoiceSlot& selectVoice() noexcept {
        for (auto& voice : voices_) {
            if (!voice.engaged) {
                return voice;
            }
        }
        VoiceSlot* candidate = nullptr;
        for (auto& voice : voices_) {
            if (!voice.held && (candidate == nullptr || voice.age < candidate->age)) {
                candidate = &voice;
            }
        }
        if (candidate != nullptr) {
            return *candidate;
        }
        candidate = &voices_.front();
        for (auto& voice : voices_) {
            if (voice.age < candidate->age) {
                candidate = &voice;
            }
        }
        return *candidate;
    }

    void applyStoredParameters(VoiceSlot& voice) noexcept {
        for (std::size_t index = 0; index < kParameterSpecs.size(); ++index) {
            const auto id = kParameterSpecs[index].id;
            if (id == resonant::BreathPipeVoice::kPitchHz || id == kExternalProbeLevel) {
                continue;
            }
            dispatchParameter(voice, id, parameter_values_[index]);
        }
    }

    static void dispatch(VoiceSlot& voice, resonant::EventType type,
                         resonant::ParameterId target, float value) noexcept {
        voice.engine.model().handleEvent(
            {0, type, target, resonant::kNoNoteId, value, 0.0F});
    }

    static void dispatchParameter(VoiceSlot& voice, resonant::ParameterId id,
                                  float value) noexcept {
        dispatch(voice, resonant::EventType::ParameterChange, id, value);
    }

    [[nodiscard]] static BreathPipeStabilityState
    classify(const resonant::EnergyDiagnostics& diagnostics) noexcept {
        if (diagnostics.nan_detected || diagnostics.infinity_detected ||
            diagnostics.state == resonant::EnergyState::NumericalRunaway ||
            diagnostics.state == resonant::EnergyState::Unstable) {
            return BreathPipeStabilityState::Unstable;
        }
        if (diagnostics.peak >= 1.35F || diagnostics.resonator_rms >= 1.2) {
            return BreathPipeStabilityState::NearLimit;
        }
        if (diagnostics.state == resonant::EnergyState::SelfSustaining) {
            return BreathPipeStabilityState::SelfOscillating;
        }
        if (diagnostics.output_rms >= 0.25 || diagnostics.resonator_rms >= 0.35) {
            return BreathPipeStabilityState::HighEnergy;
        }
        if (diagnostics.state == resonant::EnergyState::Silent) {
            return BreathPipeStabilityState::Quiet;
        }
        return BreathPipeStabilityState::Active;
    }

    void updateVoiceCounts() noexcept {
        std::uint32_t active = 0U;
        std::uint32_t held = 0U;
        for (const auto& voice : voices_) {
            if (voice.engaged) {
                ++active;
            }
            if (voice.engaged && voice.held) {
                ++held;
            }
        }
        max_active_voices_ = std::max(max_active_voices_, active);
        telemetry_.active_voices = active;
        telemetry_.held_voices = held;
        telemetry_.max_active_voices = max_active_voices_;
        telemetry_.voice_steals = voice_steals_;
        telemetry_.maximum_polyphony = kMaximumPolyphony;
    }

    std::array<VoiceSlot, kMaximumPolyphony> voices_;
    std::array<std::array<resonant::Sample, kMaximumBlockSize>, kMaximumPolyphony>
        voice_buffers_{};
    std::array<resonant::Sample, kMaximumBlockSize> external_buffer_{};
    std::array<float, kParameterSpecs.size()> parameter_values_{};
    resonant::Pcg32 external_probe_rng_{};
    BreathPipeTelemetry telemetry_{};
    std::uint64_t age_counter_{0};
    std::uint32_t voice_steals_{0};
    std::uint32_t max_active_voices_{0};
    std::uint32_t maximum_block_size_{kMaximumBlockSize};
    double sample_rate_{48'000.0};
    bool prepared_{false};
    bool protected_state_{false};
};

} // namespace resonant_lab
