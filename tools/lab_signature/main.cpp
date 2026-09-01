#include "resonant_lab/LabEngine.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <iostream>

int main() {
    resonant_lab::LabEngine lab;
    if (!lab.prepare(48'000.0, 128)) {
        return 2;
    }
    const std::array<std::pair<resonant::ParameterId, float>, 6> parameters{{
        {resonant::FirstResonatorVoice::kDamping, 0.27F},
        {resonant::FirstResonatorVoice::kFeedback, 0.46F},
        {resonant::FirstResonatorVoice::kNonlinearity, 0.35F},
        {resonant::FirstResonatorVoice::kExcitation, 0.31F},
        {resonant::FirstResonatorVoice::kTurbulence, 0.71F},
        {resonant::FirstResonatorVoice::kInteraction, 0.22F},
    }};
    for (const auto& [id, value] : parameters) {
        if (!lab.setParameter(id, value)) {
            return 3;
        }
    }
    if (!lab.noteOn(60, 0.80F)) {
        return 4;
    }

    std::array<float, 128> output{};
    double sum = 0.0;
    double sum_squares = 0.0;
    float peak = 0.0F;
    for (int block = 0; block < 240; ++block) {
        if (block == 80 && !lab.noteOn(64, 0.65F)) {
            return 5;
        }
        if (block == 160 && !lab.noteOff(60)) {
            return 6;
        }
        if (block == 200 && !lab.noteOff(64)) {
            return 7;
        }
        if (!lab.process(output)) {
            return 8;
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
              << "}\n";
    return 0;
}
