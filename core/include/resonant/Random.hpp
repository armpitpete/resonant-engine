#pragma once

#include <cstdint>

namespace resonant {

using Seed = std::uint64_t;
inline constexpr Seed kDefaultSeed = 0x7265736f6e616e74ULL; // "resonant"

[[nodiscard]] constexpr Seed splitMix64(Seed value) noexcept {
    value += 0x9e3779b97f4a7c15ULL;
    value = (value ^ (value >> 30U)) * 0xbf58476d1ce4e5b9ULL;
    value = (value ^ (value >> 27U)) * 0x94d049bb133111ebULL;
    return value ^ (value >> 31U);
}

[[nodiscard]] constexpr Seed deriveVoiceSeed(Seed engine_seed,
                                             std::uint32_t voice_index) noexcept {
    return splitMix64(engine_seed ^ (0xd1b54a32d192ed03ULL *
                                     static_cast<Seed>(voice_index + 1U)));
}

class Pcg32 {
public:
    constexpr Pcg32() noexcept { seed(kDefaultSeed); }
    explicit constexpr Pcg32(Seed value) noexcept { seed(value); }

    constexpr void seed(Seed value) noexcept {
        state_ = 0U;
        increment_ = (splitMix64(value) << 1U) | 1U;
        (void)nextUInt();
        state_ += splitMix64(value ^ 0x853c49e6748fea9bULL);
        (void)nextUInt();
        initial_seed_ = value;
    }

    constexpr void reset() noexcept { seed(initial_seed_); }

    [[nodiscard]] constexpr std::uint32_t nextUInt() noexcept {
        const std::uint64_t old_state = state_;
        state_ = old_state * 6364136223846793005ULL + increment_;
        const auto xorshifted = static_cast<std::uint32_t>(
            ((old_state >> 18U) ^ old_state) >> 27U);
        const auto rotation = static_cast<std::uint32_t>(old_state >> 59U);
        return (xorshifted >> rotation) |
               (xorshifted << ((0U - rotation) & 31U));
    }

    [[nodiscard]] float nextUnitFloat() noexcept {
        return static_cast<float>(nextUInt() >> 8U) * (1.0F / 16'777'216.0F);
    }

    [[nodiscard]] float nextSignedFloat() noexcept {
        return nextUnitFloat() * 2.0F - 1.0F;
    }

    [[nodiscard]] constexpr Seed initialSeed() const noexcept { return initial_seed_; }

private:
    std::uint64_t state_{0};
    std::uint64_t increment_{1};
    Seed initial_seed_{kDefaultSeed};
};

} // namespace resonant
