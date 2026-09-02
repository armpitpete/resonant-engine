#include "resonant/Engine.hpp"
#include "resonant/FirstResonator.hpp"

#include <array>
#include <cstdint>

#include <emscripten/emscripten.h>

namespace {

constexpr std::uint32_t kRenderQuantum = 128;

resonant::Engine<resonant::FirstResonatorVoice> g_engine{
    resonant::FirstResonatorVoice{resonant::kDefaultSeed}};
std::array<resonant::Sample, kRenderQuantum> g_input{};
std::array<resonant::Sample, kRenderQuantum> g_output{};

void dispatch(resonant::EventType type, resonant::ParameterId target,
              resonant::Sample value) noexcept {
    g_engine.model().handleEvent({0, type, target, resonant::kNoNoteId, value, 0.0F});
}

} // namespace

extern "C" {

EMSCRIPTEN_KEEPALIVE int re_prepare(double sample_rate) noexcept {
    return g_engine.prepare({sample_rate, kRenderQuantum, 1, 1}) ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE void re_reset() noexcept {
    (void)g_engine.reset();
}

EMSCRIPTEN_KEEPALIVE void re_set_parameter(std::uint32_t target,
                                           float value) noexcept {
    dispatch(resonant::EventType::ParameterChange, target, value);
}

EMSCRIPTEN_KEEPALIVE void re_set_pitch(float hz) noexcept {
    dispatch(resonant::EventType::Pitch, 0, hz);
}

EMSCRIPTEN_KEEPALIVE void re_trigger(float amount) noexcept {
    dispatch(resonant::EventType::Trigger, 0, amount);
}

EMSCRIPTEN_KEEPALIVE void re_note_on(float hz, float velocity) noexcept {
    dispatch(resonant::EventType::Pitch, 0, hz);
    dispatch(resonant::EventType::NoteOn, 0, velocity);
}

EMSCRIPTEN_KEEPALIVE int re_process(std::uint32_t frames) noexcept {
    if (frames == 0 || frames > kRenderQuantum) {
        return 0;
    }

    const resonant::Sample* inputs[1]{g_input.data()};
    resonant::Sample* outputs[1]{g_output.data()};
    const resonant::AudioBlockView block{inputs, outputs, 1, 1, frames};
    return g_engine.process(block) == resonant::ProcessStatus::Ok ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE std::uintptr_t re_output_ptr() noexcept {
    return reinterpret_cast<std::uintptr_t>(g_output.data());
}

} // extern "C"
