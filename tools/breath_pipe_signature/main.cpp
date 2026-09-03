#include "resonant/BreathPipe.hpp"
#include "resonant_lab/BreathPipeLabEngine.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <iostream>

int main() {
    resonant_lab::BreathPipeLabEngine lab;
    if (!lab.prepare(48'000.0, 128)) {
        return 2;
    }
    const std::array<std::pair<resonant::ParameterId, float>, 9> parameters{{
        {resonant::BreathPipeVoice::kPressure, 0.62F},
        {resonant::BreathPipeVoice::kTurbulence, 0.31F},
        {resonant::BreathPipeVoice::kInteraction, 0.73F},
        {resonant::BreathPipeVoice::kDamping, 0.08F},
        {resonant::BreathPipeVoice::kRegeneration, 0.42F},
        {resonant::BreathPipeVoice::kFeedbackColor, 0.37F},
        {resonant::BreathPipeVoice::kNonlinearDrive, 0.55F},
        {resonant::BreathPipeVoice::kExternalAmount, 0.0F},
        {resonant::BreathPipeVoice::kTimbre, 0.44F},
    }};
    for (const auto& [id, value] : parameters) {
        if (!lab.setParameter(id, value)) {
            return 3;
        }
    }
    if (!lab.noteOn(60, 0.82F)) {
        return 4;
    }

    std::array<float, 128> output{};
    double sum = 0.0;
    double sum_squares = 0.0;
    float peak = 0.0F;
    for (int block = 0; block < 320; ++block) {
        if (block == 96 && !lab.noteOn(67, 0.68F)) {
            return 5;
        }
        if (block == 176 && !lab.setParameter(resonant::BreathPipeVoice::kPressure, 0.92F)) {
            return 6;
        }
        if (block == 224 && !lab.noteOff(60)) {
            return 7;
        }
        if (block == 272 && !lab.noteOff(67)) {
            return 8;
        }
        if (!lab.process(output)) {
            return 9;
        }
        for (const auto sample : output) {
            sum += static_cast<double>(sample);
            sum_squares += static_cast<double>(sample) * static_cast<double>(sample);
            peak = std::max(peak, std::abs(sample));
        }
    }

    const auto& telemetry = lab.telemetry();
    std::cout << std::setprecision(17)
              << "{\"sum\":" << sum
              << ",\"sumSquares\":" << sum_squares
              << ",\"peak\":" << peak
              << ",\"maxVoices\":" << telemetry.max_active_voices
              << ",\"steals\":" << telemetry.voice_steals
              << ",\"overblow\":" << telemetry.overblow_amount
              << "}\n";
    return 0;
}
