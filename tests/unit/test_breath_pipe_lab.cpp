#include "resonant/BreathPipe.hpp"
#include "resonant_lab/BreathPipeLabEngine.hpp"

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

void processBlocks(resonant_lab::BreathPipeLabEngine& lab,
                   std::uint32_t count) {
    std::array<resonant::Sample, 128> output{};
    for (std::uint32_t index = 0; index < count; ++index) {
        check(lab.process(output), "Breath Pipe Lab process remains healthy");
        check(std::all_of(output.begin(), output.end(),
                          [](resonant::Sample sample) { return std::isfinite(sample); }),
              "Breath Pipe Lab emits only finite samples");
    }
}

void testVoiceCountsAndStress() {
    resonant_lab::BreathPipeLabEngine lab;
    check(lab.prepare(48'000.0, 128), "Breath Pipe Lab prepares");
    check(lab.setParameter(resonant::BreathPipeVoice::kPressure, 0.46F),
          "Lab pressure parameter set");
    check(lab.setParameter(resonant::BreathPipeVoice::kDamping, 0.0F),
          "Lab damping parameter set");

    check(lab.noteOn(60, 0.8F), "one voice note on");
    processBlocks(lab, 24U);
    check(lab.telemetry().active_voices == 1U, "one voice telemetry");

    for (const auto note : {64U, 67U, 72U}) {
        check(lab.noteOn(note, 0.8F), "four-voice note on");
    }
    processBlocks(lab, 24U);
    check(lab.telemetry().active_voices == 4U, "four musical voices active");
    check(!lab.telemetry().protected_state, "four musical voices remain unprotected");

    for (const auto note : {76U, 79U, 83U, 84U}) {
        check(lab.noteOn(note, 0.8F), "eight-voice stress note on");
    }
    processBlocks(lab, 32U);
    check(lab.telemetry().active_voices == 8U, "eight-voice stress reaches bounded capacity");
    check(lab.telemetry().max_active_voices == 8U, "eight-voice maximum recorded");
    check(!lab.telemetry().protected_state, "eight-voice stress remains finite");

    check(lab.noteOn(55, 0.7F), "ninth note triggers deterministic voice steal");
    processBlocks(lab, 8U);
    check(lab.telemetry().voice_steals >= 1U, "voice stealing is observable");
    check(lab.telemetry().active_voices == 8U, "voice count remains bounded after steal");

    lab.panic();
    check(lab.telemetry().active_voices == 0U, "panic clears all Breath Pipe voices");
    check(!lab.telemetry().protected_state, "panic leaves recoverable Lab state");
}

void testExternalProbeAndOverblowTelemetry() {
    resonant_lab::BreathPipeLabEngine lab;
    check(lab.prepare(48'000.0, 128), "external Lab prepares");
    check(lab.setParameter(resonant::BreathPipeVoice::kPressure, 0.0F),
          "external test pressure zero");
    check(lab.setParameter(resonant::BreathPipeVoice::kExternalAmount, 1.0F),
          "external amount full");
    check(lab.setParameter(resonant_lab::BreathPipeLabEngine::kExternalProbeLevel, 0.8F),
          "external probe enabled");
    check(lab.noteOn(60, 0.0F), "external test voice allocated without pressure");
    processBlocks(lab, 160U);
    check(lab.telemetry().core_output_rms > 1.0e-7F,
          "external Host audio creates model output with pressure zero");

    lab.reset();
    check(lab.setParameter(resonant::BreathPipeVoice::kPressure, 0.95F),
          "overblow pressure set");
    check(lab.setParameter(resonant::BreathPipeVoice::kInteraction, 0.85F),
          "overblow interaction set");
    check(lab.setParameter(resonant::BreathPipeVoice::kNonlinearDrive, 0.85F),
          "overblow nonlinear drive set");
    check(lab.setParameter(resonant::BreathPipeVoice::kRegeneration, 0.50F),
          "overblow regeneration set");
    check(lab.noteOn(57, 1.0F), "overblow voice on");
    processBlocks(lab, 160U);
    check(lab.telemetry().overblow_amount > 0.50F,
          "Lab exposes overblow state continuously");
    check(lab.telemetry().mode_energy[1] > 0.0F,
          "Lab exposes upper-mode energy during overblow");
}

} // namespace

int main() {
    testVoiceCountsAndStress();
    testExternalProbeAndOverblowTelemetry();
    if (failures != 0) {
        std::cerr << failures << " Breath Pipe Lab test(s) failed\n";
        return 1;
    }
    std::cout << "PASS: M3 Breath Pipe Lab polyphony/external evidence\n";
    return 0;
}
