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
    std::cout << "PASS: M0 property contracts\n";
    return 0;
}
