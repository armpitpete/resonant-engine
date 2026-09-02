#include "resonant/BreathPipe.hpp"
#include "resonant/Engine.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <span>
#include <string_view>

namespace {

int failures = 0;

void check(bool condition, std::string_view name) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << name << '\n';
    }
}

[[nodiscard]] double midiToHz(std::uint32_t note) {
    return 440.0 * std::pow(2.0, (static_cast<double>(note) - 69.0) / 12.0);
}

[[nodiscard]] double rms(std::span<const resonant::Sample> samples) {
    double sum = 0.0;
    for (const auto sample : samples) {
        const auto value = static_cast<double>(sample);
        sum += value * value;
    }
    return samples.empty() ? 0.0 : std::sqrt(sum / static_cast<double>(samples.size()));
}

void testOperatingEnvelopeAndSilence() {
    static_assert(resonant::EngineModel<resonant::BreathPipeVoice>);
    static_assert(resonant::BreathPipeModalResonator::memoryBytes() == 60U);

    constexpr std::array<double, 3> sample_rates{{44'100.0, 48'000.0, 96'000.0}};
    constexpr std::array<std::uint32_t, 7> block_sizes{{32U, 64U, 128U, 256U, 512U,
                                                        1024U, 4096U}};
    for (const auto sample_rate : sample_rates) {
        for (const auto block_size : block_sizes) {
            resonant::Engine<resonant::BreathPipeVoice> engine;
            check(engine.prepare({sample_rate, block_size, 1, 1}),
                  "Breath Pipe prepares across M3 operating envelope");
        }
    }

    resonant::Engine<resonant::BreathPipeVoice> engine;
    check(engine.prepare({48'000.0, 128, 1, 1}), "Breath Pipe silence prepare");
    std::array<resonant::Sample, 128> input{};
    std::array<resonant::Sample, 128> output{};
    const resonant::Sample* inputs[1]{input.data()};
    resonant::Sample* outputs[1]{output.data()};
    resonant::AudioBlockView block{inputs, outputs, 1, 1, 128};
    for (std::uint32_t index = 0; index < 32U; ++index) {
        check(engine.process(block) == resonant::ProcessStatus::Ok,
              "zero-energy Breath Pipe processes");
        check(std::all_of(output.begin(), output.end(),
                          [](resonant::Sample sample) { return sample == 0.0F; }),
              "zero-energy Breath Pipe is exactly silent");
    }
}

void testContinuousExcitationExternalAndInteraction() {
    resonant::Engine<resonant::BreathPipeVoice> engine{
        resonant::BreathPipeVoice{777}};
    check(engine.prepare({48'000.0, 128, 1, 1}), "continuous Breath Pipe prepare");

    std::array<resonant::Sample, 128> input{};
    std::array<resonant::Sample, 128> output{};
    const resonant::Sample* inputs[1]{input.data()};
    resonant::Sample* outputs[1]{output.data()};
    resonant::AudioBlockView block{inputs, outputs, 1, 1, 128};

    resonant::FixedEventBuffer<8> events;
    check(events.push({0, resonant::EventType::Pitch, 0, 0, 220.0F, 0.0F}),
          "pitch event accepted");
    check(events.push({0, resonant::EventType::Pressure, 0, 0, 0.42F, 0.0F}),
          "pressure event accepted");
    check(events.push({0, resonant::EventType::ParameterChange,
                       resonant::BreathPipeVoice::kTurbulence, 0, 0.12F, 0.0F}),
          "turbulence event accepted");
    check(events.push({0, resonant::EventType::ParameterChange,
                       resonant::BreathPipeVoice::kDamping, 0, 0.0F, 0.0F}),
          "damping event accepted");

    double energy = 0.0;
    for (std::uint32_t block_index = 0; block_index < 240U; ++block_index) {
        output.fill(0.0F);
        const auto scheduled = block_index == 0U ? events.span()
                                                  : std::span<const resonant::Event>{};
        check(engine.process(block, scheduled) == resonant::ProcessStatus::Ok,
              "continuous Breath Pipe remains finite");
        if (block_index > 80U) {
            const auto value = rms(output);
            energy += value * value;
        }
    }
    check(energy > 1.0e-7, "pressure/turbulence supplies sustained resonant energy");

    check(engine.reset(), "reset before external excitation");
    resonant::FixedEventBuffer<4> external_events;
    check(external_events.push({0, resonant::EventType::Pitch, 0, 0, 220.0F, 0.0F}),
          "external pitch event accepted");
    check(external_events.push({0, resonant::EventType::ParameterChange,
                                resonant::BreathPipeVoice::kExternalAmount,
                                0, 1.0F, 0.0F}),
          "external amount event accepted");
    double external_energy = 0.0;
    for (std::uint32_t block_index = 0; block_index < 160U; ++block_index) {
        for (std::size_t frame = 0; frame < input.size(); ++frame) {
            const auto phase = static_cast<double>(block_index * 128U +
                                                   static_cast<std::uint32_t>(frame));
            input[frame] = static_cast<float>(0.18 * std::sin(6.283185307179586 *
                                                               173.0 * phase / 48'000.0));
        }
        const auto scheduled = block_index == 0U ? external_events.span()
                                                  : std::span<const resonant::Event>{};
        check(engine.process(block, scheduled) == resonant::ProcessStatus::Ok,
              "external excitation path remains finite");
        if (block_index > 40U) {
            external_energy += rms(output);
        }
    }
    check(external_energy > 0.01, "external audio genuinely excites Breath Pipe model");

    auto renderInteractionSignature = [](float interaction) {
        resonant::Engine<resonant::BreathPipeVoice> local{resonant::BreathPipeVoice{999}};
        if (!local.prepare({48'000.0, 128, 1, 1})) {
            return 0.0;
        }
        std::array<resonant::Sample, 128> in{};
        std::array<resonant::Sample, 128> out{};
        const resonant::Sample* in_ptr[1]{in.data()};
        resonant::Sample* out_ptr[1]{out.data()};
        resonant::AudioBlockView local_block{in_ptr, out_ptr, 1, 1, 128};
        resonant::FixedEventBuffer<8> local_events;
        (void)local_events.push({0, resonant::EventType::Pitch, 0, 0, 329.6276F, 0.0F});
        (void)local_events.push({0, resonant::EventType::Pressure, 0, 0, 0.70F, 0.0F});
        (void)local_events.push({0, resonant::EventType::ParameterChange,
                                 resonant::BreathPipeVoice::kTurbulence, 0, 0.18F, 0.0F});
        (void)local_events.push({0, resonant::EventType::ParameterChange,
                                 resonant::BreathPipeVoice::kInteraction, 0, interaction, 0.0F});
        double signature = 0.0;
        for (std::uint32_t block_index = 0; block_index < 180U; ++block_index) {
            const auto scheduled = block_index == 0U ? local_events.span()
                                                      : std::span<const resonant::Event>{};
            if (local.process(local_block, scheduled) != resonant::ProcessStatus::Ok) {
                return 0.0;
            }
            for (std::size_t frame = 0; frame < out.size(); frame += 17U) {
                signature += static_cast<double>(out[frame]) *
                             static_cast<double>((frame + 1U) * (block_index + 1U));
            }
        }
        return signature;
    };
    const auto interaction_off = renderInteractionSignature(0.0F);
    const auto interaction_on = renderInteractionSignature(1.0F);
    check(std::abs(interaction_off - interaction_on) > 1.0e-4,
          "returned resonator state materially changes excitation behaviour");
}

void testOverblowAndRegeneration() {
    resonant::BreathPipeModalResonator resonator;
    check(resonator.prepare({48'000.0, 128, 1, 1}), "modal resonator prepare");

    const resonant::BreathPipeModalResonator::Control low{
        220.0F, 0.05F, 0.42F, 0.85F, 0.50F, 0.70F, 0.85F, 0.25F};
    const resonant::BreathPipeModalResonator::Control high{
        220.0F, 0.05F, 0.95F, 0.85F, 0.50F, 0.70F, 0.85F, 0.25F};

    std::array<double, 3> low_energy{};
    std::array<double, 3> high_energy{};
    for (std::uint32_t frame = 0; frame < 48'000U; ++frame) {
        const auto excitation = frame == 0U ? 0.00038F : 0.000002F *
            static_cast<float>((frame % 19U) - 9U);
        (void)resonator.processSample(excitation, low);
        if (frame > 12'000U) {
            for (std::size_t mode = 0; mode < 3U; ++mode) {
                const auto sample = static_cast<double>(resonator.modeSample(mode));
                low_energy[mode] += sample * sample;
            }
        }
    }
    const auto low_overblow = resonator.overblowAmount();
    resonator.reset();
    for (std::uint32_t frame = 0; frame < 48'000U; ++frame) {
        const auto excitation = frame == 0U ? 0.00038F : 0.000002F *
            static_cast<float>((frame % 23U) - 11U);
        (void)resonator.processSample(excitation, high);
        if (frame > 12'000U) {
            for (std::size_t mode = 0; mode < 3U; ++mode) {
                const auto sample = static_cast<double>(resonator.modeSample(mode));
                high_energy[mode] += sample * sample;
            }
        }
    }
    const auto high_overblow = resonator.overblowAmount();
    check(low_overblow < 0.01F && high_overblow > 0.50F,
          "pressure continuously opens the overblow operating region");
    check(high_energy[1] / std::max(1.0e-18, high_energy[0]) >
              low_energy[1] / std::max(1.0e-18, low_energy[0]),
          "overblow redistributes energy toward the upper resonant mode");
    check(resonator.modeRadius(1) > resonator.modeRadius(0),
          "high-pressure topology favours upper mode without switching model");

    resonator.reset();
    resonant::BreathPipeModalResonator::Control self_sustain{
        220.0F, 0.0F, 0.0F, 0.8F, 1.5F, 0.5F, 0.75F, 0.25F};
    double sustain_tail = 0.0;
    for (std::uint32_t frame = 0; frame < 96'000U; ++frame) {
        const auto excitation = frame == 0U ? 0.0005F : 0.0F;
        const auto output = resonator.processSample(excitation, self_sustain);
        if (frame >= 72'000U) {
            sustain_tail += std::abs(static_cast<double>(output.sample));
        }
    }
    check(sustain_tail > 1.0e-4, "active regeneration can sustain seeded resonance");

    auto recovery = self_sustain;
    recovery.regeneration = 0.0F;
    recovery.damping = 0.85F;
    double recovery_tail = 0.0;
    for (std::uint32_t frame = 0; frame < 48'000U; ++frame) {
        const auto output = resonator.processSample(0.0F, recovery);
        if (frame >= 36'000U) {
            recovery_tail += std::abs(static_cast<double>(output.sample));
        }
    }
    check(recovery_tail < sustain_tail * 0.20,
          "reducing regeneration and increasing loss recovers from self sustain");
}

[[nodiscard]] double estimateModeFrequency(double sample_rate, double target_hz) {
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
    std::array<double, 32> crossings{};
    std::size_t crossing_count = 0U;
    const auto frames = static_cast<std::uint32_t>(sample_rate * 2.5);
    for (std::uint32_t frame = 0; frame < frames && crossing_count < crossings.size(); ++frame) {
        const auto excitation = frame == 0U ? 1.0e-5F : 0.0F;
        (void)resonator.processSample(excitation, control);
        const auto current = static_cast<double>(resonator.modeSample(0));
        if (frame > static_cast<std::uint32_t>(sample_rate * 0.20) &&
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

void testPitchEnvelope() {
    constexpr std::array<std::uint32_t, 5> notes{{36U, 48U, 60U, 72U, 84U}};
    constexpr std::array<double, 3> sample_rates{{44'100.0, 48'000.0, 96'000.0}};
    for (const auto sample_rate : sample_rates) {
        for (const auto note : notes) {
            const auto target = midiToHz(note);
            const auto measured = estimateModeFrequency(sample_rate, target);
            check(measured > 0.0, "C2-C6 modal pitch measurement exists");
            if (measured > 0.0) {
                const auto cents = 1'200.0 * std::log2(measured / target);
                check(std::abs(cents) <= 15.0,
                      "C2-C6 canonical stable-mode tuning stays within 15 cents");
            }
        }
    }
}

void testExtremeFiniteBehaviour() {
    resonant::Engine<resonant::BreathPipeVoice> engine{resonant::BreathPipeVoice{4242}};
    check(engine.prepare({96'000.0, 1024, 1, 1}), "extreme Breath Pipe prepare");
    std::array<resonant::Sample, 1024> input{};
    std::array<resonant::Sample, 1024> output{};
    const resonant::Sample* inputs[1]{input.data()};
    resonant::Sample* outputs[1]{output.data()};
    resonant::AudioBlockView block{inputs, outputs, 1, 1, 1024};
    for (std::uint32_t block_index = 0; block_index < 300U; ++block_index) {
        resonant::FixedEventBuffer<16> events;
        if (block_index % 7U == 0U) {
            (void)events.push({0, resonant::EventType::Pitch, 0, 0,
                               block_index % 14U == 0U ? 65.4064F : 1'046.5023F, 0.0F});
            (void)events.push({0, resonant::EventType::Pressure, 0, 0,
                               block_index % 2U == 0U ? 1.0F : 0.05F, 0.0F});
            (void)events.push({0, resonant::EventType::ParameterChange,
                               resonant::BreathPipeVoice::kRegeneration, 0,
                               block_index % 3U == 0U ? 1.5F : 0.0F, 0.0F});
            (void)events.push({0, resonant::EventType::ParameterChange,
                               resonant::BreathPipeVoice::kNonlinearDrive, 0,
                               block_index % 2U == 0U ? 1.0F : 0.0F, 0.0F});
        }
        for (std::size_t frame = 0; frame < input.size(); ++frame) {
            input[frame] = ((frame + block_index) % 11U == 0U) ? 0.75F : -0.15F;
        }
        const auto status = engine.process(block, events.span());
        check(status == resonant::ProcessStatus::Ok,
              "extreme finite M3 operating sequence remains processable");
        check(std::all_of(output.begin(), output.end(),
                          [](resonant::Sample sample) { return std::isfinite(sample); }),
              "extreme sequence never emits NaN/Infinity");
    }
}

} // namespace

int main() {
    testOperatingEnvelopeAndSilence();
    testContinuousExcitationExternalAndInteraction();
    testOverblowAndRegeneration();
    testPitchEnvelope();
    testExtremeFiniteBehaviour();
    if (failures != 0) {
        std::cerr << failures << " Breath Pipe test(s) failed\n";
        return 1;
    }
    std::cout << "PASS: M3 Breath Pipe operating envelope and behaviour\n";
    return 0;
}
