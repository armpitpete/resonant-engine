#include "resonant/Engine.hpp"
#include "resonant/FirstResonator.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string_view>
#include <vector>

namespace {

int failures = 0;

void check(bool condition, std::string_view name) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << name << '\n';
    }
}

std::vector<resonant::Sample> renderContinuous(resonant::Seed seed,
                                               double sample_rate,
                                               std::uint32_t block_size,
                                               std::uint32_t total_frames) {
    resonant::Engine<resonant::FirstResonatorVoice> engine{
        resonant::FirstResonatorVoice{seed}};
    if (!engine.prepare({sample_rate, block_size, 1, 1})) {
        return {};
    }

    std::vector<resonant::Sample> result;
    result.reserve(total_frames);
    std::vector<resonant::Sample> input(block_size, 0.0F);
    std::vector<resonant::Sample> output(block_size, 0.0F);
    bool first = true;
    std::uint32_t rendered = 0;
    while (rendered < total_frames) {
        const auto frames = std::min(block_size, total_frames - rendered);
        const resonant::Sample* inputs[1]{input.data()};
        resonant::Sample* outputs[1]{output.data()};
        resonant::AudioBlockView block{inputs, outputs, 1, 1, frames};
        resonant::FixedEventBuffer<8> events;
        if (first) {
            if (!events.push({0, resonant::EventType::Pitch, 0, 0, 330.0F, 0.0F}) ||
                !events.push({0, resonant::EventType::Pressure, 0, 0, 0.42F, 0.0F}) ||
                !events.push({0, resonant::EventType::ParameterChange,
                              resonant::FirstResonatorVoice::kTurbulence, 0, 0.83F, 0.0F}) ||
                !events.push({0, resonant::EventType::ParameterChange,
                              resonant::FirstResonatorVoice::kFeedback, 0, 0.28F, 0.0F}) ||
                !events.push({0, resonant::EventType::ParameterChange,
                              resonant::FirstResonatorVoice::kNonlinearity, 0, 0.35F, 0.0F})) {
                return {};
            }
            first = false;
        }
        if (engine.process(block, events.span()) != resonant::ProcessStatus::Ok) {
            return {};
        }
        result.insert(result.end(), output.begin(), output.begin() + frames);
        rendered += frames;
    }
    return result;
}

void testContractsAndPrepare() {
    static_assert(resonant::Exciter<resonant::ContinuousNoiseExciter>);
    static_assert(resonant::Resonator<resonant::TunedDelayResonator>);
    static_assert(resonant::EngineModel<resonant::FirstResonatorVoice>);

    constexpr std::array<double, 5> sample_rates{
        44'100.0, 48'000.0, 96'000.0, 192'000.0, 384'000.0};
    for (const auto sample_rate : sample_rates) {
        resonant::TunedDelayResonator resonator;
        check(resonator.prepare({sample_rate, 64, 1, 1}),
              "first resonator prepares across sample rates");
        check(resonator.minimumTuningHz() >= 24.0F,
              "first resonator minimum tuning is bounded");
        check(resonator.maximumTuningHz() > 1'000.0F,
              "first resonator retains useful high tuning range");
    }
    check(resonant::TunedDelayResonator::memoryBytes() == 65'536U,
          "first resonator delay memory is fixed and known");
}

void testSilenceTriggerAndExternalExcitation() {
    resonant::Engine<resonant::FirstResonatorVoice> engine;
    check(engine.prepare({48'000.0, 64, 1, 1}), "first voice prepare");

    std::array<resonant::Sample, 64> input{};
    std::array<resonant::Sample, 64> output{};
    const resonant::Sample* inputs[1]{input.data()};
    resonant::Sample* outputs[1]{output.data()};
    resonant::AudioBlockView block{inputs, outputs, 1, 1, 64};
    check(engine.process(block) == resonant::ProcessStatus::Ok,
          "first voice silent process succeeds");
    check(std::all_of(output.begin(), output.end(),
                      [](resonant::Sample sample) { return sample == 0.0F; }),
          "first voice remains silent without energy");

    resonant::FixedEventBuffer<4> trigger_events;
    check(trigger_events.push({0, resonant::EventType::Trigger, 0, 0, 0.9F, 0.0F}),
          "trigger event accepted");
    bool heard_trigger = false;
    for (std::uint32_t block_index = 0; block_index < 8U; ++block_index) {
        output.fill(0.0F);
        const auto events = block_index == 0U ? trigger_events.span()
                                              : std::span<const resonant::Event>{};
        check(engine.process(block, events) == resonant::ProcessStatus::Ok,
              "trigger ringing process succeeds");
        for (const auto sample : output) {
            heard_trigger = heard_trigger || std::abs(sample) > 1.0e-6F;
        }
    }
    check(heard_trigger, "transient trigger produces delayed resonant output");

    check(engine.reset(), "reset before external excitation");
    input.fill(0.0F);
    input[0] = 0.8F;
    bool heard_external = false;
    for (std::uint32_t block_index = 0; block_index < 8U; ++block_index) {
        if (block_index != 0U) {
            input.fill(0.0F);
        }
        output.fill(0.0F);
        check(engine.process(block) == resonant::ProcessStatus::Ok,
              "external excitation process succeeds");
        for (const auto sample : output) {
            heard_external = heard_external || std::abs(sample) > 1.0e-6F;
        }
    }
    check(heard_external, "arbitrary external audio excites resonator");
}

void testContinuousExcitationWithoutNoteOn() {
    resonant::Engine<resonant::FirstResonatorVoice> engine{
        resonant::FirstResonatorVoice{777}};
    check(engine.prepare({48'000.0, 64, 1, 1}), "continuous excitation prepare");
    std::array<resonant::Sample, 64> input{};
    std::array<resonant::Sample, 64> output{};
    const resonant::Sample* inputs[1]{input.data()};
    resonant::Sample* outputs[1]{output.data()};
    resonant::AudioBlockView block{inputs, outputs, 1, 1, 64};

    resonant::FixedEventBuffer<4> events;
    check(events.push({0, resonant::EventType::Pressure, 0, 0, 0.5F, 0.0F}),
          "continuous pressure event accepted");
    check(events.push({0, resonant::EventType::ParameterChange,
                       resonant::FirstResonatorVoice::kTurbulence, 0, 0.9F, 0.0F}),
          "continuous turbulence event accepted");

    double energy = 0.0;
    for (std::uint32_t block_index = 0; block_index < 40U; ++block_index) {
        const auto block_events = block_index == 0U ? events.span()
                                                    : std::span<const resonant::Event>{};
        check(engine.process(block, block_events) == resonant::ProcessStatus::Ok,
              "continuous excitation process succeeds");
        for (const auto sample : output) {
            energy += static_cast<double>(sample) * static_cast<double>(sample);
        }
    }
    check(energy > 1.0e-5, "continuous excitation produces sound without NoteOn");
    check(engine.model().diagnostics().output_rms > 0.0,
          "real resonator feeds energy diagnostics");
}

void testFractionalTuningControl() {
    resonant::TunedDelayResonator resonator;
    check(resonator.prepare({48'000.0, 64, 1, 1}), "fractional tuning prepare");
    const auto output = resonator.processSample(
        {1.0F, {440.0F, 0.0F, 0.0F, 0.0F}, {}});
    (void)output;
    check(std::abs(resonator.delaySamples() - (48'000.0 / 440.0)) < 1.0e-6,
          "440 Hz maps to fractional delay without integer quantisation");

    resonant::TunedDelayResonator high_rate;
    check(high_rate.prepare({96'000.0, 64, 1, 1}), "high-rate tuning prepare");
    (void)high_rate.processSample({0.0F, {440.0F, 0.0F, 0.0F, 0.0F}, {}});
    check(std::abs(high_rate.delaySamples() - (96'000.0 / 440.0)) < 1.0e-6,
          "fractional tuning is sample-rate aware");
}

void testDeterminismAndReset() {
    const auto a = renderContinuous(777, 48'000.0, 64, 4'096);
    const auto b = renderContinuous(777, 48'000.0, 64, 4'096);
    const auto c = renderContinuous(778, 48'000.0, 64, 4'096);
    check(!a.empty() && a == b, "same seed and events give exact repeat render");
    check(a.size() == c.size() && a != c, "different seed changes stochastic render");

    resonant::Engine<resonant::FirstResonatorVoice> engine{
        resonant::FirstResonatorVoice{1234}};
    check(engine.prepare({48'000.0, 64, 1, 1}), "reset determinism prepare");
    std::array<resonant::Sample, 64> input{};
    std::array<resonant::Sample, 64> output_a{};
    std::array<resonant::Sample, 64> output_b{};
    const resonant::Sample* inputs[1]{input.data()};
    resonant::Sample* outputs_a[1]{output_a.data()};
    resonant::Sample* outputs_b[1]{output_b.data()};
    const std::array<resonant::Event, 1> events{{
        {0, resonant::EventType::Pressure, 0, 0, 0.7F, 0.0F},
    }};
    resonant::AudioBlockView block_a{inputs, outputs_a, 1, 1, 64};
    resonant::AudioBlockView block_b{inputs, outputs_b, 1, 1, 64};
    check(engine.process(block_a, events) == resonant::ProcessStatus::Ok,
          "first deterministic block processes");
    check(engine.reset(), "deterministic reset succeeds");
    check(engine.process(block_b, events) == resonant::ProcessStatus::Ok,
          "second deterministic block processes");
    check(output_a == output_b, "reset restores deterministic exciter/resonator state");
}

void testPassiveAndRegenerativeStates() {
    auto late_energy = [](resonant::Sample feedback,
                          resonant::Sample nonlinearity) {
        resonant::TunedDelayResonator resonator;
        if (!resonator.prepare({48'000.0, 64, 1, 1})) {
            return -1.0;
        }
        double late = 0.0;
        constexpr std::uint32_t total = 48'000U;
        constexpr std::uint32_t late_start = 44'000U;
        for (std::uint32_t frame = 0; frame < total; ++frame) {
            const auto excitation = frame == 0U ? 0.8F : 0.0F;
            const auto output = resonator.processSample(
                {excitation, {220.0F, 0.30F, feedback, nonlinearity}, {}});
            if (!std::isfinite(output.sample)) {
                return -1.0;
            }
            if (frame >= late_start) {
                late += static_cast<double>(output.sample) *
                        static_cast<double>(output.sample);
            }
        }
        return late;
    };

    const auto passive = late_energy(0.0F, 0.0F);
    const auto regenerative = late_energy(0.60F, 0.55F);
    check(passive >= 0.0 && regenerative >= 0.0,
          "passive/regenerative renders remain finite");
    check(regenerative > passive + 1.0e-8,
          "regenerative feedback retains more late energy than passive decay");
}

void testAggressiveFiniteOperation() {
    resonant::TunedDelayResonator resonator;
    check(resonator.prepare({48'000.0, 64, 1, 1}), "aggressive resonator prepare");
    bool finite = true;
    resonant::Sample peak = 0.0F;
    for (std::uint32_t frame = 0; frame < 100'000U; ++frame) {
        const auto excitation = frame < 4'000U ? 0.35F : 0.0F;
        const auto output = resonator.processSample(
            {excitation, {110.0F, 0.05F, 1.5F, 1.0F}, {}});
        finite = finite && std::isfinite(output.sample);
        peak = std::max(peak, std::abs(output.sample));
    }
    check(finite && !resonator.numericalFailure(),
          "aggressive nonlinear feedback stays numerically finite");
    check(peak > 0.01F && peak <= 2.0F,
          "aggressive state is audible but emergency-bounded");
}

void testBlockAndSampleRateVariation() {
    constexpr std::array<double, 4> sample_rates{44'100.0, 48'000.0, 96'000.0, 192'000.0};
    constexpr std::array<std::uint32_t, 5> block_sizes{1U, 7U, 32U, 63U, 128U};
    for (const auto sample_rate : sample_rates) {
        for (const auto block_size : block_sizes) {
            const auto rendered = renderContinuous(42, sample_rate, block_size, 512U);
            check(rendered.size() == 512U,
                  "first resonator renders across block/sample-rate matrix");
            check(std::all_of(rendered.begin(), rendered.end(),
                              [](resonant::Sample sample) { return std::isfinite(sample); }),
                  "matrix render remains finite");
        }
    }
}

} // namespace

int main() {
    testContractsAndPrepare();
    testSilenceTriggerAndExternalExcitation();
    testContinuousExcitationWithoutNoteOn();
    testFractionalTuningControl();
    testDeterminismAndReset();
    testPassiveAndRegenerativeStates();
    testAggressiveFiniteOperation();
    testBlockAndSampleRateVariation();

    if (failures != 0) {
        std::cerr << failures << " first-resonator assertion(s) failed\n";
        return 1;
    }
    std::cout << "PASS: Resonant Engine M1 first-resonator suite\n";
    return 0;
}
