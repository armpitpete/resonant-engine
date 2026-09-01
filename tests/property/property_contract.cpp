#include "resonant/Parameter.hpp"
#include "resonant/Random.hpp"

#include <cmath>
#include <iostream>

int main() {
    constexpr resonant::ParameterSpec spec{7, "test", "u", -2.0F, 3.0F, 0.0F};
    resonant::Pcg32 rng{0x4d302d70726f70ULL};
    for (int i = 0; i < 20'000; ++i) {
        const float native = rng.nextSignedFloat() * 100.0F;
        const float normalized = spec.normalize(native);
        if (!(normalized >= 0.0F && normalized <= 1.0F && std::isfinite(normalized))) {
            std::cerr << "normalized parameter escaped range\n";
            return 1;
        }
        const float round_trip = spec.denormalize(normalized);
        if (!(round_trip >= spec.minimum && round_trip <= spec.maximum && std::isfinite(round_trip))) {
            std::cerr << "denormalized parameter escaped range\n";
            return 1;
        }
    }

    const auto voice0_seed = resonant::deriveVoiceSeed(777, 0);
    const auto voice1_seed = resonant::deriveVoiceSeed(777, 1);
    resonant::Pcg32 voice0_a{voice0_seed};
    resonant::Pcg32 voice0_b{voice0_seed};
    resonant::Pcg32 voice1{voice1_seed};
    bool independent_voice_differs = false;
    for (int i = 0; i < 1024; ++i) {
        const auto a = voice0_a.nextUInt();
        const auto b = voice0_b.nextUInt();
        const auto c = voice1.nextUInt();
        if (a != b) {
            std::cerr << "derived voice seed is not deterministic\n";
            return 1;
        }
        independent_voice_differs = independent_voice_differs || (a != c);
    }
    if (!independent_voice_differs) {
        std::cerr << "independent voice seed did not produce an independent sequence\n";
        return 1;
    }

    std::cout << "PASS: M0 property contracts\n";
    return 0;
}
