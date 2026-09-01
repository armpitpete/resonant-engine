#pragma once

#include <algorithm>
#include <cmath>

#include "resonant/Engine.hpp"

namespace resonant {

// Architectural probe only. This is not the Breath Pipe voice and is not a
// finished resonator. It exists to prove that the core can host continuous
// excitation, a stateful resonant element, active feedback, feedback shaping,
// and a bounded nonlinearity inside one real-time sample loop.
class ReferenceFeedbackProbe final {
public:
    struct Input {
        float excitation = 0.0f;
        float resonance = 0.90f;
        float feedback = 0.0f;
        float feedback_filter = 0.5f;
        float nonlinearity = 0.0f;
    };

    struct Output {
        float sample = 0.0f;
        float feedback_tap = 0.0f;
    };

    void prepare(const ProcessSpec& spec) noexcept {
        sample_rate_ = static_cast<float>(spec.sample_rate);
    }

    void reset() noexcept {
        resonator_state_ = 0.0f;
        feedback_state_ = 0.0f;
    }

    Output tick(const Input& input) noexcept {
        const float feedback_amount = clamp(input.feedback, -0.999f, 0.999f);
        const float filter_amount = clamp(input.feedback_filter, 0.0f, 1.0f);
        const float resonance = clamp(input.resonance, 0.0f, 0.9995f);
        const float nonlinear_amount = clamp(input.nonlinearity, 0.0f, 1.0f);

        feedback_state_ += filter_amount * (resonator_state_ - feedback_state_);
        const float returned_energy = feedback_state_ * feedback_amount;
        const float drive = input.excitation + returned_energy;
        const float shaped_drive = crossfade(drive, softClip(drive), nonlinear_amount);

        // Deliberately simple state element: its job in M0 is to exercise the
        // closed-loop architecture, not to claim a pipe or modal model.
        resonator_state_ = shaped_drive + resonance * resonator_state_;
        resonator_state_ = softClip(resonator_state_);

        return {resonator_state_, feedback_state_};
    }

    float sampleRate() const noexcept {
        return sample_rate_;
    }

private:
    static float clamp(float value, float low, float high) noexcept {
        return std::max(low, std::min(value, high));
    }

    static float softClip(float value) noexcept {
        return value / (1.0f + std::fabs(value));
    }

    static float crossfade(float a, float b, float amount) noexcept {
        return a + (b - a) * amount;
    }

    float sample_rate_ = 48000.0f;
    float resonator_state_ = 0.0f;
    float feedback_state_ = 0.0f;
};

} // namespace resonant
