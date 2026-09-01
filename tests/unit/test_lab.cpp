#include "resonant_lab/LabEngine.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
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

void testParameterContract() {
    const auto specs = resonant_lab::LabEngine::parameterSpecs();
    check(specs.size() == 7U, "lab exposes seven canonical M1 parameters");
    for (const auto& spec : specs) {
        check(spec.valid(), "lab parameter metadata is valid");
    }
}

void testPrepareSilenceAndReset() {
    resonant_lab::LabEngine lab;
    check(lab.prepare(48'000.0, 128), "lab prepares at browser quantum");
    check(!lab.prepare(48'000.0, 129), "lab rejects oversized browser quantum");
    check(lab.prepare(48'000.0, 128), "lab re-prepares after rejected spec");

    std::array<float, 128> output{};
    check(lab.process(output), "lab silent process succeeds");
    check(std::all_of(output.begin(), output.end(),
                      [](float sample) { return sample == 0.0F; }),
          "lab is silent before a test note");
    check(lab.telemetry().active_voices == 0U, "lab starts with no active voices");

    check(lab.setParameter(resonant::FirstResonatorVoice::kDamping, 0.8F),
          "lab accepts canonical damping parameter");
    check(std::abs(lab.parameterValue(resonant::FirstResonatorVoice::kDamping) - 0.8F) <
              1.0e-6F,
          "lab stores parameter value");
    lab.reset();
    check(std::abs(lab.parameterValue(resonant::FirstResonatorVoice::kDamping) - 0.30F) <
              1.0e-6F,
          "lab reset restores canonical default");
}

void testNoteAndPolyphony() {
    resonant_lab::LabEngine lab;
    check(lab.prepare(48'000.0), "polyphony lab prepare");
    std::array<float, 128> output{};

    check(lab.noteOn(60, 0.8F), "lab note on accepted");
    bool heard = false;
    for (std::uint32_t block = 0; block < 24U; ++block) {
        check(lab.process(output), "single voice process succeeds");
        for (const auto sample : output) {
            heard = heard || std::abs(sample) > 1.0e-6F;
            check(std::isfinite(sample), "single voice remains finite");
        }
    }
    check(heard, "lab note reaches the real resonator core");
    check(lab.telemetry().active_voices == 1U, "lab reports one active voice");
    check(lab.noteOff(60), "lab note off accepted");

    lab.panic();
    constexpr std::array<std::uint32_t, 8> chord{48, 52, 55, 60, 64, 67, 72, 76};
    for (const auto note : chord) {
        check(lab.noteOn(note, 0.7F), "polyphony note accepted");
    }
    check(lab.telemetry().active_voices == 8U, "lab reaches configured maximum polyphony");
    check(lab.telemetry().max_active_voices == 8U, "lab records maximum active voices");
    check(lab.noteOn(79, 0.7F), "ninth note triggers bounded voice stealing");
    check(lab.telemetry().active_voices == 8U, "voice stealing preserves fixed polyphony");
    check(lab.telemetry().voice_steals == 1U, "voice stealing is observable");

    lab.panic();
    check(lab.telemetry().active_voices == 0U, "panic clears active voices");
    output.fill(1.0F);
    check(lab.process(output), "post-panic process succeeds");
    check(std::all_of(output.begin(), output.end(),
                      [](float sample) { return sample == 0.0F; }),
          "panic restores immediate silence");
}

void testAggressiveFiniteOperation() {
    resonant_lab::LabEngine lab;
    check(lab.prepare(48'000.0), "stress lab prepare");
    check(lab.setParameter(resonant::FirstResonatorVoice::kDamping, 0.0F),
          "stress damping accepted");
    check(lab.setParameter(resonant::FirstResonatorVoice::kFeedback, 1.5F),
          "stress feedback accepted");
    check(lab.setParameter(resonant::FirstResonatorVoice::kNonlinearity, 1.0F),
          "stress nonlinearity accepted");
    check(lab.setParameter(resonant::FirstResonatorVoice::kExcitation, 1.0F),
          "stress excitation accepted");
    check(lab.setParameter(resonant::FirstResonatorVoice::kTurbulence, 1.0F),
          "stress turbulence accepted");
    check(lab.setParameter(resonant::FirstResonatorVoice::kInteraction, 1.0F),
          "stress interaction accepted");
    for (std::uint32_t note = 48; note < 56; ++note) {
        check(lab.noteOn(note, 1.0F), "stress voice accepted");
    }

    std::array<float, 128> output{};
    for (std::uint32_t block = 0; block < 1'000U; ++block) {
        check(lab.process(output), "aggressive lab process remains protected and finite");
        for (const auto sample : output) {
            check(std::isfinite(sample), "aggressive lab output finite");
            check(std::abs(sample) <= 1.0F, "lab safety mix remains bounded");
        }
    }
    check(!lab.telemetry().protected_state, "aggressive finite state is not false-positive protected");
    check(lab.telemetry().maximum_polyphony == 8U, "maximum polyphony telemetry is stable");
}

void testDeterministicReset() {
    resonant_lab::LabEngine lab;
    check(lab.prepare(48'000.0), "determinism lab prepare");
    std::array<float, 128> first{};
    std::array<float, 128> second{};

    check(lab.setParameter(resonant::FirstResonatorVoice::kTurbulence, 0.83F),
          "determinism parameter accepted");
    check(lab.noteOn(57, 0.77F), "determinism first note accepted");
    for (std::uint32_t block = 0; block < 10U; ++block) {
        check(lab.process(first), "determinism first pass process");
    }

    lab.reset();
    check(lab.setParameter(resonant::FirstResonatorVoice::kTurbulence, 0.83F),
          "determinism parameter restored");
    check(lab.noteOn(57, 0.77F), "determinism second note accepted");
    for (std::uint32_t block = 0; block < 10U; ++block) {
        check(lab.process(second), "determinism second pass process");
    }
    check(first == second, "lab reset restores deterministic voice-bank result");
}

} // namespace

int main() {
    testParameterContract();
    testPrepareSilenceAndReset();
    testNoteAndPolyphony();
    testAggressiveFiniteOperation();
    testDeterministicReset();
    if (failures != 0) {
        std::cerr << failures << " lab test(s) failed\n";
        return 1;
    }
    std::cout << "Resonant Engine Lab tests passed\n";
    return 0;
}
