#include "resonant_lab/BreathPipeLabEngine.hpp"

#include <array>
#include <cstdint>
#include <span>

#include <emscripten/emscripten.h>

EM_JS(double, re_m3_now_ms, (), {
    return Date.now();
});

namespace {

constexpr std::uint32_t kRenderQuantum = resonant_lab::BreathPipeLabEngine::kMaximumBlockSize;
resonant_lab::BreathPipeLabEngine g_lab;
std::array<resonant::Sample, kRenderQuantum> g_output{};
double g_sample_rate = 48'000.0;
double g_cpu_load = 0.0;
double g_cpu_load_smoothed = 0.0;
double g_cpu_load_max = 0.0;

void resetCpu() noexcept {
    g_cpu_load = 0.0;
    g_cpu_load_smoothed = 0.0;
    g_cpu_load_max = 0.0;
}

} // namespace

extern "C" {

EMSCRIPTEN_KEEPALIVE int re_prepare(double sample_rate) noexcept {
    g_sample_rate = sample_rate;
    resetCpu();
    return g_lab.prepare(sample_rate, kRenderQuantum) ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE void re_reset() noexcept {
    g_lab.reset();
    resetCpu();
}

EMSCRIPTEN_KEEPALIVE void re_panic() noexcept { g_lab.panic(); }

EMSCRIPTEN_KEEPALIVE int re_set_parameter(std::uint32_t id, float value) noexcept {
    return g_lab.setParameter(id, value) ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE float re_parameter_value(std::uint32_t id) noexcept {
    return g_lab.parameterValue(id);
}

EMSCRIPTEN_KEEPALIVE int re_note_on(std::uint32_t midi_note, float velocity) noexcept {
    return g_lab.noteOn(midi_note, velocity) ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE int re_note_off(std::uint32_t midi_note) noexcept {
    return g_lab.noteOff(midi_note) ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE int re_process(std::uint32_t frames) noexcept {
    if (frames == 0U || frames > kRenderQuantum) {
        return 0;
    }
    const auto start = re_m3_now_ms();
    const auto ok = g_lab.process(std::span<resonant::Sample>{g_output.data(), frames});
    const auto elapsed_ms = re_m3_now_ms() - start;
    const auto budget_ms = 1'000.0 * static_cast<double>(frames) / g_sample_rate;
    g_cpu_load = budget_ms > 0.0 ? 100.0 * elapsed_ms / budget_ms : 0.0;
    g_cpu_load_smoothed = g_cpu_load_smoothed == 0.0
                              ? g_cpu_load
                              : (0.94 * g_cpu_load_smoothed + 0.06 * g_cpu_load);
    if (g_cpu_load > g_cpu_load_max) {
        g_cpu_load_max = g_cpu_load;
    }
    return ok ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE std::uintptr_t re_output_ptr() noexcept {
    return reinterpret_cast<std::uintptr_t>(g_output.data());
}

EMSCRIPTEN_KEEPALIVE std::uint32_t re_parameter_count() noexcept {
    return static_cast<std::uint32_t>(resonant_lab::BreathPipeLabEngine::parameterSpecs().size());
}

EMSCRIPTEN_KEEPALIVE std::uint32_t re_parameter_id(std::uint32_t index) noexcept {
    const auto specs = resonant_lab::BreathPipeLabEngine::parameterSpecs();
    return index < specs.size() ? specs[index].id : 0U;
}

EMSCRIPTEN_KEEPALIVE float re_parameter_min(std::uint32_t index) noexcept {
    const auto specs = resonant_lab::BreathPipeLabEngine::parameterSpecs();
    return index < specs.size() ? specs[index].minimum : 0.0F;
}

EMSCRIPTEN_KEEPALIVE float re_parameter_max(std::uint32_t index) noexcept {
    const auto specs = resonant_lab::BreathPipeLabEngine::parameterSpecs();
    return index < specs.size() ? specs[index].maximum : 0.0F;
}

EMSCRIPTEN_KEEPALIVE float re_parameter_default(std::uint32_t index) noexcept {
    const auto specs = resonant_lab::BreathPipeLabEngine::parameterSpecs();
    return index < specs.size() ? specs[index].default_value : 0.0F;
}

EMSCRIPTEN_KEEPALIVE float re_resonator_energy() noexcept {
    return g_lab.telemetry().resonator_energy;
}

EMSCRIPTEN_KEEPALIVE float re_core_output_rms() noexcept {
    return g_lab.telemetry().core_output_rms;
}

EMSCRIPTEN_KEEPALIVE float re_core_peak() noexcept {
    return g_lab.telemetry().core_peak;
}

EMSCRIPTEN_KEEPALIVE std::uint32_t re_stability_state() noexcept {
    return static_cast<std::uint32_t>(g_lab.telemetry().stability);
}

EMSCRIPTEN_KEEPALIVE std::uint32_t re_active_voices() noexcept {
    return g_lab.telemetry().active_voices;
}

EMSCRIPTEN_KEEPALIVE std::uint32_t re_held_voices() noexcept {
    return g_lab.telemetry().held_voices;
}

EMSCRIPTEN_KEEPALIVE std::uint32_t re_max_active_voices() noexcept {
    return g_lab.telemetry().max_active_voices;
}

EMSCRIPTEN_KEEPALIVE std::uint32_t re_maximum_polyphony() noexcept {
    return g_lab.telemetry().maximum_polyphony;
}

EMSCRIPTEN_KEEPALIVE std::uint32_t re_voice_steals() noexcept {
    return g_lab.telemetry().voice_steals;
}

EMSCRIPTEN_KEEPALIVE int re_protected_state() noexcept {
    return g_lab.telemetry().protected_state ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE float re_overblow_amount() noexcept {
    return g_lab.telemetry().overblow_amount;
}

EMSCRIPTEN_KEEPALIVE float re_mode_energy(std::uint32_t mode) noexcept {
    const auto& energy = g_lab.telemetry().mode_energy;
    return mode < energy.size() ? energy[mode] : 0.0F;
}

EMSCRIPTEN_KEEPALIVE double re_cpu_load() noexcept { return g_cpu_load; }
EMSCRIPTEN_KEEPALIVE double re_cpu_load_smoothed() noexcept { return g_cpu_load_smoothed; }
EMSCRIPTEN_KEEPALIVE double re_cpu_load_max() noexcept { return g_cpu_load_max; }

} // extern "C"
