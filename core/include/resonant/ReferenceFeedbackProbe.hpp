#pragma once

#include "resonant/Energy.hpp"
#include "resonant/Event.hpp"
#include "resonant/Feedback.hpp"
#include "resonant/Parameter.hpp"
#include "resonant/Types.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <span>

namespace resonant {

class ReferenceFeedbackProbe {
public:
    static constexpr ParameterId kExcitation = 1;
    static constexpr ParameterId kResonance = 2;
    static constexpr ParameterId kFeedback = 3;
    static constexpr ParameterId kFeedbackFilter = 4;
    static constexpr ParameterId kNonlinearity = 5;

    [[nodiscard]] bool prepare(const ProcessSpec& spec) noexcept {
        if (!spec.valid()) {
            return false;
        }
        sample_rate_ = spec.sample_rate;
        energy_.prepare(256, 4.0F);
        return true;
    }

    void reset() noexcept {
        y1_ = 0.0F;
        y2_ = 0.0F;
        excitation_ = 0.0F;
        resonance_ = 0.92F;
        feedback_ = 0.0F;
        feedback_filter_ = 0.2F;
        nonlinearity_ = 0.0F;
        feedback_stage_.reset();
        energy_.reset();
    }

    void handleEvent(const Event& event) noexcept {
        const auto v = finiteOrZero(event.value);
        switch (event.type) {
        case EventType::Pressure:
            excitation_ = std::clamp(v, 0.0F, 1.0F);
            break;
        case EventType::Pitch:
            resonance_ = std::clamp(0.6F + v * 0.39F, 0.0F, 0.9995F);
            break;
        case EventType::ParameterChange:
            switch (event.target) {
            case kExcitation: excitation_ = std::clamp(v, 0.0F, 1.0F); break;
            case kResonance: resonance_ = std::clamp(v, 0.0F, 0.9995F); break;
            case kFeedback: feedback_ = std::clamp(v, 0.0F, 1.5F); break;
            case kFeedbackFilter: feedback_filter_ = std::clamp(v, 0.0F, 1.0F); break;
            case kNonlinearity: nonlinearity_ = std::clamp(v, 0.0F, 1.0F); break;
            default: break;
            }
            break;
        case EventType::Trigger:
            y1_ += std::clamp(v, -1.0F, 1.0F);
            break;
        case EventType::NoteOn:
            excitation_ = std::clamp(v, 0.0F, 1.0F);
            break;
        case EventType::NoteOff:
            excitation_ = 0.0F;
            break;
        case EventType::PerNoteExpression:
            break;
        }
    }

    [[nodiscard]] bool processSample(std::span<const Sample> input,
                                     std::span<Sample> output) noexcept {
        const Sample external = input.empty() ? 0.0F : finiteOrZero(input[0]);
        const auto feedback_result = feedback_stage_.process(
            y1_, FeedbackControl{feedback_, 0.0F, feedback_filter_,
                                 nonlinearity_, 1.0F, 1.0F});
        const Sample drive = external + excitation_ + feedback_result.returned;
        const Sample next = drive + resonance_ * (1.85F * y1_ - 0.88F * y2_);
        y2_ = y1_;
        y1_ = next / (1.0F + std::abs(next));
        const Sample sample = EnergyMonitor::contain(y1_, 1.0F);

        energy_.observe(drive, y1_, sample, feedback_result.returned);
        if (energy_.diagnostics().nan_detected || energy_.diagnostics().infinity_detected) {
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

private:
    double sample_rate_{48'000.0};
    Sample y1_{0.0F};
    Sample y2_{0.0F};
    Sample excitation_{0.0F};
    Sample resonance_{0.92F};
    Sample feedback_{0.0F};
    Sample feedback_filter_{0.2F};
    Sample nonlinearity_{0.0F};
    FeedbackStage feedback_stage_{};
    EnergyMonitor energy_{};
};

} // namespace resonant
