#pragma once

#include "resonant/Types.hpp"

#include <algorithm>
#include <cmath>

namespace resonant {

struct FeedbackControl {
    Sample gain{0.0F};
    Sample loss{0.0F};
    Sample filter{1.0F};
    Sample nonlinearity{0.0F};
    Sample polarity{1.0F};
    Sample envelope{1.0F};
};

struct FeedbackResult {
    Sample returned{0.0F};
    Sample filtered{0.0F};
    Sample energy{0.0F};
};

class FeedbackStage {
public:
    void reset() noexcept { filtered_state_ = 0.0F; }

    [[nodiscard]] FeedbackResult process(Sample source,
                                         FeedbackControl control) noexcept {
        const auto gain = clampFinite(control.gain, 0.0F, 4.0F);
        const auto loss = clampFinite(control.loss, 0.0F, 1.0F);
        const auto filter = clampFinite(control.filter, 0.0F, 1.0F, 1.0F);
        const auto amount = clampFinite(control.nonlinearity, 0.0F, 1.0F);
        const auto polarity = control.polarity < 0.0F ? -1.0F : 1.0F;
        const auto envelope = clampFinite(control.envelope, 0.0F, 1.0F, 1.0F);

        const auto finite_source = finiteOrZero(source);
        filtered_state_ += filter * (finite_source - filtered_state_);
        const auto linear = filtered_state_ * gain * (1.0F - loss) * polarity * envelope;
        const auto nonlinear = softSaturate(linear);
        const auto returned = linear + amount * (nonlinear - linear);
        return {returned, filtered_state_, returned * returned};
    }

private:
    [[nodiscard]] static Sample softSaturate(Sample value) noexcept {
        return value / (1.0F + std::abs(value));
    }

    Sample filtered_state_{0.0F};
};

} // namespace resonant
