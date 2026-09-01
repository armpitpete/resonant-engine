#pragma once

#include "resonant/Energy.hpp"
#include "resonant/Event.hpp"
#include "resonant/Random.hpp"
#include "resonant/Types.hpp"

#include <concepts>
#include <cstdint>
#include <span>

namespace resonant {

struct ExciterInput {
    Sample external_audio{0.0F};
    Sample pressure{0.0F};
    Sample turbulence{0.0F};
    Sample trigger{0.0F};
    Sample returned_resonator{0.0F};
};

struct ExciterOutput {
    Sample sample{0.0F};
    Sample energy{0.0F};
};

struct ResonatorControl {
    Sample tuning_hz{440.0F};
    Sample damping{0.5F};
    Sample feedback{0.0F};
    Sample nonlinearity{0.0F};
};

struct CouplingPort {
    std::uint32_t id{0};
    Sample value{0.0F};
};

struct ResonatorInput {
    Sample excitation{0.0F};
    ResonatorControl control{};
    std::span<const CouplingPort> coupling_inputs{};
};

struct ResonatorOutput {
    Sample sample{0.0F};
    Sample feedback_tap{0.0F};
    Sample energy{0.0F};
};

template <typename T>
concept Exciter = requires(T exciter, const ProcessSpec& spec,
                           const ExciterInput& input, const Event& event,
                           Seed seed) {
    { exciter.prepare(spec, seed) } noexcept -> std::same_as<bool>;
    { exciter.reset() } noexcept;
    { exciter.handleEvent(event) } noexcept;
    { exciter.processSample(input) } noexcept -> std::same_as<ExciterOutput>;
};

template <typename T>
concept Resonator = requires(T resonator, const ProcessSpec& spec,
                             const ResonatorInput& input, const Event& event) {
    { resonator.prepare(spec) } noexcept -> std::same_as<bool>;
    { resonator.reset() } noexcept;
    { resonator.handleEvent(event) } noexcept;
    { resonator.processSample(input) } noexcept -> std::same_as<ResonatorOutput>;
};

} // namespace resonant
