#include "resonant/Engine.hpp"
#include "resonant/FirstResonator.hpp"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <new>

namespace {
std::atomic<bool> g_track_allocations{false};
std::atomic<std::size_t> g_allocation_count{0};
}

void* operator new(std::size_t size) {
    if (g_track_allocations.load(std::memory_order_relaxed)) {
        g_allocation_count.fetch_add(1, std::memory_order_relaxed);
    }
    if (void* pointer = std::malloc(size)) {
        return pointer;
    }
    throw std::bad_alloc{};
}

void operator delete(void* pointer) noexcept { std::free(pointer); }
void operator delete(void* pointer, std::size_t) noexcept { std::free(pointer); }

int main() {
    resonant::Engine<resonant::FirstResonatorVoice> engine{
        resonant::FirstResonatorVoice{777}};
    if (!engine.prepare({48'000.0, 64, 1, 1})) {
        std::cerr << "first resonator allocation probe failed to prepare\n";
        return 1;
    }

    std::array<resonant::Sample, 64> input{};
    std::array<resonant::Sample, 64> output{};
    const resonant::Sample* inputs[1]{input.data()};
    resonant::Sample* outputs[1]{output.data()};
    resonant::AudioBlockView block{inputs, outputs, 1, 1, 64};
    const std::array<resonant::Event, 2> events{{
        {0, resonant::EventType::Pressure, 0, 0, 0.4F, 0.0F},
        {0, resonant::EventType::ParameterChange,
         resonant::FirstResonatorVoice::kFeedback, 0, 0.35F, 0.0F},
    }};

    g_allocation_count.store(0, std::memory_order_relaxed);
    g_track_allocations.store(true, std::memory_order_relaxed);
    const auto status = engine.process(block, events);
    g_track_allocations.store(false, std::memory_order_relaxed);

    if (status != resonant::ProcessStatus::Ok) {
        std::cerr << "first resonator allocation probe process failed\n";
        return 1;
    }
    if (g_allocation_count.load(std::memory_order_relaxed) != 0U) {
        std::cerr << "first resonator allocated in Engine::process\n";
        return 1;
    }

    std::cout << "PASS: first resonator process path allocates nothing\n";
    return 0;
}
