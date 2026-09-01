#include "resonant/Engine.hpp"
#include "resonant/Parameter.hpp"
#include "resonant/Random.hpp"
#include "resonant/ReferenceFeedbackProbe.hpp"

#include <array>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <new>
#include <string_view>

namespace {
std::atomic<bool> g_track_allocations{false};
std::atomic<std::size_t> g_allocation_count{0};
}

void* operator new(std::size_t size) {
    if (g_track_allocations.load(std::memory_order_relaxed)) {
        g_allocation_count.fetch_add(1, std::memory_order_relaxed);
    }
    if (void* p = std::malloc(size)) {
        return p;
    }
    throw std::bad_alloc{};
}

void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }

namespace {

int failures = 0;

void check(bool condition, std::string_view name) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << name << '\n';
    }
}

resonant::AudioBlockView makeMonoBlock(std::array<resonant::Sample, 64>& input,
                                       std::array<resonant::Sample, 64>& output,
                                       std::uint32_t frames) {
    static const resonant::Sample* inputs[1];
    static resonant::Sample* outputs[1];
    inputs[0] = input.data();
    outputs[0] = output.data();
    return {inputs, outputs, 1, 1, frames};
}

void testConstructionPrepareSilenceReset() {
    resonant::Engine<resonant::ReferenceFeedbackProbe> engine;
    std::array<resonant::Sample, 64> input{};
    std::array<resonant::Sample, 64> output{};
    auto block = makeMonoBlock(input, output, 64);

    check(engine.process(block) == resonant::ProcessStatus::NotPrepared,
          "processing before prepare is inert");
    check(engine.prepare({48'000.0, 64, 1, 1}), "prepare valid spec");
    check(engine.process(block) == resonant::ProcessStatus::Ok, "silence process succeeds");
    for (auto sample : output) {
        check(sample == 0.0F, "silence remains silent");
    }
    check(engine.reset(), "reset after prepare succeeds");
    check(!resonant::Engine<resonant::ReferenceFeedbackProbe>{}.reset(),
          "reset before prepare rejected");
}

void testSampleAccurateEventsAndOrdering() {
    resonant::Engine<resonant::ReferenceFeedbackProbe> engine;
    check(engine.prepare({48'000.0, 64, 1, 1}), "event test prepare");

    std::array<resonant::Sample, 64> input{};
    std::array<resonant::Sample, 64> output{};
    auto block = makeMonoBlock(input, output, 16);

    resonant::FixedEventBuffer<8> events;
    check(events.push({8, resonant::EventType::ParameterChange,
                       resonant::ReferenceFeedbackProbe::kExcitation, 0, 0.25F, 0.0F}),
          "event inserted");
    check(events.push({8, resonant::EventType::ParameterChange,
                       resonant::ReferenceFeedbackProbe::kFeedback, 0, 0.1F, 0.0F}),
          "simultaneous event inserted");
    check(engine.process(block, events.span()) == resonant::ProcessStatus::Ok,
          "ordered events process");
    for (std::size_t i = 0; i < 8; ++i) {
        check(output[i] == 0.0F, "event has no effect before exact sample");
    }
    check(std::abs(output[8]) > 0.0F, "event applies at exact sample");

    const std::array<resonant::Event, 2> bad_order{{
        {9, resonant::EventType::Trigger, 0, 0, 1.0F, 0.0F},
        {3, resonant::EventType::Trigger, 0, 0, 1.0F, 0.0F},
    }};
    check(engine.process(block, bad_order) == resonant::ProcessStatus::InvalidEventOrder,
          "malformed event order rejected");
}

void testEventOverflowAndFinalSample() {
    resonant::FixedEventBuffer<2> events;
    check(events.push({3, resonant::EventType::Trigger, 0, 0, 1.0F, 0.0F}), "event 1");
    check(events.push({0, resonant::EventType::Trigger, 0, 0, 1.0F, 0.0F}), "event 2");
    check(!events.push({1, resonant::EventType::Trigger, 0, 0, 1.0F, 0.0F}), "overflow rejected");
    check(events.overflowed(), "overflow reported");
    check(events.span()[0].sample_offset == 0, "fixed event buffer orders deterministically");

    resonant::Engine<resonant::ReferenceFeedbackProbe> engine;
    check(engine.prepare({48'000.0, 64, 1, 1}), "final sample prepare");
    std::array<resonant::Sample, 64> input{};
    std::array<resonant::Sample, 64> output{};
    auto block = makeMonoBlock(input, output, 4);
    const std::array<resonant::Event, 1> final_event{{
        {3, resonant::EventType::ParameterChange,
         resonant::ReferenceFeedbackProbe::kExcitation, 0, 0.2F, 0.0F},
    }};
    check(engine.process(block, final_event) == resonant::ProcessStatus::Ok,
          "final sample event accepted");
    check(output[0] == 0.0F && output[1] == 0.0F && output[2] == 0.0F,
          "final event deferred until final sample");
    check(output[3] != 0.0F, "final event affects final sample");
}

void testParameterContract() {
    constexpr resonant::ParameterSpec spec{
        42, "pressure", "normalized", 0.0F, 2.0F, 0.25F,
        resonant::ParameterKind::Continuous,
        resonant::SmoothingMode::Linear, 0.001, true};
    check(spec.valid(), "parameter spec valid");
    check(spec.clamp(-1.0F) == 0.0F, "parameter clamps low");
    check(spec.clamp(3.0F) == 2.0F, "parameter clamps high");
    check(std::abs(spec.normalize(1.0F) - 0.5F) < 1.0e-6F, "parameter normalizes");
    check(std::abs(spec.denormalize(0.5F) - 1.0F) < 1.0e-6F, "parameter denormalizes");
    check(spec.clamp(std::numeric_limits<float>::quiet_NaN()) == spec.default_value,
          "invalid parameter falls back safely");

    resonant::ParameterSmoother smoother;
    smoother.reset(0.0F);
    smoother.prepare(1'000.0, resonant::SmoothingMode::Linear, 0.004);
    smoother.setTarget(1.0F);
    check(std::abs(smoother.next() - 0.25F) < 1.0e-6F, "linear smoothing sample 1");
    (void)smoother.next();
    (void)smoother.next();
    check(std::abs(smoother.next() - 1.0F) < 1.0e-6F, "linear smoothing reaches target");
    smoother.reset(0.4F);
    check(smoother.current() == 0.4F && smoother.target() == 0.4F,
          "smoothing reset deterministic");
}

void testDeterministicRandomness() {
    resonant::Pcg32 a{12345};
    resonant::Pcg32 b{12345};
    resonant::Pcg32 c{12346};
    bool different = false;
    for (int i = 0; i < 128; ++i) {
        const auto av = a.nextUInt();
        const auto bv = b.nextUInt();
        const auto cv = c.nextUInt();
        check(av == bv, "identical seed identical RNG sequence");
        different = different || (av != cv);
    }
    check(different, "different seed differs");
    check(resonant::deriveVoiceSeed(99, 0) != resonant::deriveVoiceSeed(99, 1),
          "voice seeds are independently derived");
    a.reset();
    b.reset();
    check(a.nextUInt() == b.nextUInt(), "rng reset returns to initial sequence");
}

void testBlockAndSampleRateVariation() {
    constexpr std::array<std::uint32_t, 6> block_sizes{1, 2, 7, 16, 31, 64};
    constexpr std::array<double, 4> sample_rates{8'000.0, 44'100.0, 48'000.0, 192'000.0};
    for (double sr : sample_rates) {
        for (auto frames : block_sizes) {
            resonant::Engine<resonant::ReferenceFeedbackProbe> engine;
            check(engine.prepare({sr, 64, 1, 1}), "variation prepare");
            std::array<resonant::Sample, 64> input{};
            std::array<resonant::Sample, 64> output{};
            auto block = makeMonoBlock(input, output, frames);
            const std::array<resonant::Event, 1> events{{
                {0, resonant::EventType::ParameterChange,
                 resonant::ReferenceFeedbackProbe::kExcitation, 0, 0.15F, 0.0F},
            }};
            check(engine.process(block, events) == resonant::ProcessStatus::Ok,
                  "variation process");
            for (std::uint32_t i = 0; i < frames; ++i) {
                check(std::isfinite(output[i]), "variation output finite");
            }
        }
    }
}

void testStereoAndZeroFrame() {
    resonant::Engine<resonant::ReferenceFeedbackProbe> engine;
    check(engine.prepare({48'000.0, 64, 2, 2}), "stereo prepare");
    std::array<resonant::Sample, 64> left_in{};
    std::array<resonant::Sample, 64> right_in{};
    std::array<resonant::Sample, 64> left_out{};
    std::array<resonant::Sample, 64> right_out{};
    const resonant::Sample* inputs[2]{left_in.data(), right_in.data()};
    resonant::Sample* outputs[2]{left_out.data(), right_out.data()};
    resonant::AudioBlockView zero{inputs, outputs, 2, 2, 0};
    check(engine.process(zero) == resonant::ProcessStatus::Ok, "zero-frame block succeeds");

    resonant::AudioBlockView block{inputs, outputs, 2, 2, 8};
    const std::array<resonant::Event, 1> events{{
        {0, resonant::EventType::ParameterChange,
         resonant::ReferenceFeedbackProbe::kExcitation, 0, 0.2F, 0.0F},
    }};
    check(engine.process(block, events) == resonant::ProcessStatus::Ok, "stereo process");
    for (std::size_t i = 0; i < 8; ++i) {
        check(left_out[i] == right_out[i], "probe writes deterministic stereo");
    }
}

void testNoRealtimeAllocation() {
    resonant::Engine<resonant::ReferenceFeedbackProbe> engine;
    check(engine.prepare({48'000.0, 64, 1, 1}), "allocation prepare");
    std::array<resonant::Sample, 64> input{};
    std::array<resonant::Sample, 64> output{};
    auto block = makeMonoBlock(input, output, 64);
    const std::array<resonant::Event, 1> events{{
        {0, resonant::EventType::ParameterChange,
         resonant::ReferenceFeedbackProbe::kExcitation, 0, 0.2F, 0.0F},
    }};

    g_allocation_count.store(0, std::memory_order_relaxed);
    g_track_allocations.store(true, std::memory_order_relaxed);
    const auto status = engine.process(block, events);
    g_track_allocations.store(false, std::memory_order_relaxed);
    check(status == resonant::ProcessStatus::Ok, "allocation test process succeeds");
    check(g_allocation_count.load(std::memory_order_relaxed) == 0,
          "demonstrated process path allocates nothing");
}

void testRepeatedLifecycle() {
    resonant::Engine<resonant::ReferenceFeedbackProbe> engine;
    for (int i = 0; i < 32; ++i) {
        check(engine.prepare({48'000.0 + (i % 2) * 100.0, 64, 1, 1}),
              "repeated prepare");
        check(engine.reset(), "repeated reset");
    }
    check(!engine.prepare({0.0, 64, 1, 1}), "invalid sample rate rejected");
    check(!engine.prepared(), "invalid reprepare leaves engine unprepared");
}

} // namespace

int main() {
    testConstructionPrepareSilenceReset();
    testSampleAccurateEventsAndOrdering();
    testEventOverflowAndFinalSample();
    testParameterContract();
    testDeterministicRandomness();
    testBlockAndSampleRateVariation();
    testStereoAndZeroFrame();
    testNoRealtimeAllocation();
    testRepeatedLifecycle();

    if (failures != 0) {
        std::cerr << failures << " test assertion(s) failed\n";
        return 1;
    }
    std::cout << "PASS: Resonant Engine M0 unit suite\n";
    return 0;
}
