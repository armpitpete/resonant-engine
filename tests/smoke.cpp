#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>

#include "resonant/Engine.hpp"
#include "resonant/ReferenceFeedbackProbe.hpp"

int main() {
    using Engine = resonant::Engine<resonant::ReferenceFeedbackProbe>;

    Engine engine;
    const bool prepared = engine.prepare({48000.0, 64});
    assert(prepared);
    assert(engine.model().sampleRate() == 48000.0f);

    std::array<Engine::Input, 256> inputs{};
    std::array<Engine::Output, 256> outputs{};

    for (std::size_t i = 0; i < inputs.size(); ++i) {
        const float phase = static_cast<float>(i) / static_cast<float>(inputs.size() - 1);
        inputs[i].excitation = 0.01f + 0.20f * phase;
        inputs[i].resonance = 0.92f;
        inputs[i].feedback = 0.80f * phase;
        inputs[i].feedback_filter = 0.12f + 0.70f * phase;
        inputs[i].nonlinearity = phase;
    }

    engine.processBlock(inputs.data(), outputs.data(), outputs.size());

    bool produced_energy = false;
    for (const auto& output : outputs) {
        assert(std::isfinite(output.sample));
        assert(std::isfinite(output.feedback_tap));
        assert(std::fabs(output.sample) <= 1.0f);
        if (std::fabs(output.sample) > 0.00001f) {
            produced_energy = true;
        }
    }
    assert(produced_energy);

    engine.reset();
    Engine::Input silent{};
    const auto after_reset = engine.processSample(silent);
    assert(std::fabs(after_reset.sample) < 0.00001f);
    assert(std::fabs(after_reset.feedback_tap) < 0.00001f);

    return 0;
}
