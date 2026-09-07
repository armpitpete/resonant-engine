#include "hosts/vst3/Processor.hpp"

#include "public.sdk/source/vst/hosting/parameterchanges.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <new>
#include <string_view>

namespace {

std::atomic<bool> g_track_allocations{false};
std::atomic<std::size_t> g_allocation_count{0U};
int failures = 0;

void check(bool condition, std::string_view name) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << name << '\n';
    }
}

constexpr Steinberg::int32 kFrames = 128;

bool prepare(resonant::vst3::Processor& processor) {
    Steinberg::Vst::ProcessSetup setup{};
    setup.processMode = Steinberg::Vst::kRealtime;
    setup.symbolicSampleSize = Steinberg::Vst::kSample32;
    setup.maxSamplesPerBlock = kFrames;
    setup.sampleRate = 48'000.0;
    return processor.setupProcessing(setup) == Steinberg::kResultOk;
}

Steinberg::Vst::Event noteOnEvent() {
    Steinberg::Vst::Event event{};
    event.busIndex = 0;
    event.sampleOffset = 0;
    event.type = Steinberg::Vst::Event::kNoteOnEvent;
    event.noteOn.channel = 0;
    event.noteOn.pitch = 60;
    event.noteOn.tuning = 0.0F;
    event.noteOn.velocity = 0.72F;
    event.noteOn.length = 0;
    event.noteOn.noteId = 7;
    return event;
}

class SingleEventList final : public Steinberg::Vst::IEventList {
public:
    explicit SingleEventList(const Steinberg::Vst::Event& event) noexcept
        : event_(event) {}

    Steinberg::tresult PLUGIN_API queryInterface(
        const Steinberg::TUID,
        void** object) override {
        if (object != nullptr) {
            *object = nullptr;
        }
        return Steinberg::kNoInterface;
    }

    Steinberg::uint32 PLUGIN_API addRef() override { return 1U; }
    Steinberg::uint32 PLUGIN_API release() override { return 1U; }
    Steinberg::int32 PLUGIN_API getEventCount() override { return 1; }

    Steinberg::tresult PLUGIN_API getEvent(
        Steinberg::int32 index,
        Steinberg::Vst::Event& event) override {
        if (index != 0) {
            return Steinberg::kResultFalse;
        }
        event = event_;
        return Steinberg::kResultOk;
    }

    Steinberg::tresult PLUGIN_API addEvent(
        Steinberg::Vst::Event&) override {
        return Steinberg::kNotImplemented;
    }

private:
    Steinberg::Vst::Event event_{};
};

class CountOnlyEventList final : public Steinberg::Vst::IEventList {
public:
    explicit CountOnlyEventList(Steinberg::int32 count) noexcept
        : count_(count) {}

    Steinberg::tresult PLUGIN_API queryInterface(
        const Steinberg::TUID,
        void** object) override {
        if (object != nullptr) {
            *object = nullptr;
        }
        return Steinberg::kNoInterface;
    }

    Steinberg::uint32 PLUGIN_API addRef() override { return 1U; }
    Steinberg::uint32 PLUGIN_API release() override { return 1U; }
    Steinberg::int32 PLUGIN_API getEventCount() override { return count_; }

    Steinberg::tresult PLUGIN_API getEvent(
        Steinberg::int32,
        Steinberg::Vst::Event&) override {
        return Steinberg::kResultFalse;
    }

    Steinberg::tresult PLUGIN_API addEvent(
        Steinberg::Vst::Event&) override {
        return Steinberg::kNotImplemented;
    }

private:
    Steinberg::int32 count_{0};
};

class CountOnlyParameterChanges final
    : public Steinberg::Vst::IParameterChanges {
public:
    explicit CountOnlyParameterChanges(Steinberg::int32 count) noexcept
        : count_(count) {}

    Steinberg::tresult PLUGIN_API queryInterface(
        const Steinberg::TUID,
        void** object) override {
        if (object != nullptr) {
            *object = nullptr;
        }
        return Steinberg::kNoInterface;
    }

    Steinberg::uint32 PLUGIN_API addRef() override { return 1U; }
    Steinberg::uint32 PLUGIN_API release() override { return 1U; }

    Steinberg::int32 PLUGIN_API getParameterCount() override {
        return count_;
    }

    Steinberg::Vst::IParamValueQueue* PLUGIN_API getParameterData(
        Steinberg::int32) override {
        return nullptr;
    }

    Steinberg::Vst::IParamValueQueue* PLUGIN_API addParameterData(
        const Steinberg::Vst::ParamID&,
        Steinberg::int32&) override {
        return nullptr;
    }

private:
    Steinberg::int32 count_{0};
};

struct BlockFixture {
    std::array<float, static_cast<std::size_t>(kFrames)> input_left{};
    std::array<float, static_cast<std::size_t>(kFrames)> input_right{};
    std::array<float, static_cast<std::size_t>(kFrames)> output_left{};
    std::array<float, static_cast<std::size_t>(kFrames)> output_right{};
    std::array<float*, 2U> input_channels{{
        input_left.data(),
        input_right.data(),
    }};
    std::array<float*, 2U> output_channels{{
        output_left.data(),
        output_right.data(),
    }};
    Steinberg::Vst::AudioBusBuffers input_bus{};
    Steinberg::Vst::AudioBusBuffers output_bus{};
    Steinberg::Vst::ProcessData data{};

    explicit BlockFixture(bool with_input = false) noexcept {
        input_bus.numChannels = 2;
        input_bus.channelBuffers32 = input_channels.data();
        output_bus.numChannels = 2;
        output_bus.channelBuffers32 = output_channels.data();

        data.processMode = Steinberg::Vst::kRealtime;
        data.symbolicSampleSize = Steinberg::Vst::kSample32;
        data.numSamples = kFrames;
        data.numInputs = with_input ? 1 : 0;
        data.numOutputs = 1;
        data.inputs = with_input ? &input_bus : nullptr;
        data.outputs = &output_bus;
    }

    void clearOutput(float value = 0.0F) noexcept {
        output_left.fill(value);
        output_right.fill(value);
        output_bus.silenceFlags = 0;
    }
};

bool allZero(const BlockFixture& block) {
    const auto zero = [](float sample) { return sample == 0.0F; };
    return std::all_of(block.output_left.begin(), block.output_left.end(), zero) &&
           std::all_of(block.output_right.begin(), block.output_right.end(), zero);
}

void testWrapperProcessAllocatesNothing() {
    resonant::vst3::Processor processor;
    check(prepare(processor), "prepare processor for wrapper allocation proof");

    BlockFixture block{true};
    block.input_left.fill(0.05F);
    block.input_right.fill(-0.03F);

    const auto note = noteOnEvent();
    SingleEventList events{note};

    Steinberg::Vst::ParameterChanges changes{1};
    Steinberg::int32 queue_index = 0;
    auto* queue = changes.addParameterData(
        resonant::BreathPipeVoice::kTurbulence, queue_index);
    Steinberg::int32 point_index = 0;
    check(queue != nullptr &&
              queue->addPoint(64, 0.35, point_index) == Steinberg::kResultOk,
          "prepare fixed automation before allocation tracking");

    block.data.inputEvents = &events;
    block.data.inputParameterChanges = &changes;

    g_allocation_count.store(0U, std::memory_order_relaxed);
    g_track_allocations.store(true, std::memory_order_release);
    const auto status = processor.process(block.data);
    g_track_allocations.store(false, std::memory_order_release);

    check(status == Steinberg::kResultOk,
          "tracked VST3 process call succeeds");
    check(g_allocation_count.load(std::memory_order_acquire) == 0U,
          "VST3 process path performs zero dynamic allocations");
}

void testBoundedHostTranslationFailsClosed() {
    const auto too_many =
        static_cast<Steinberg::int32>(resonant::kMaxEventsPerBlock + 1U);

    resonant::vst3::Processor event_processor;
    check(prepare(event_processor), "prepare event-overflow processor");
    BlockFixture event_block{};
    event_block.clearOutput(0.5F);
    CountOnlyEventList events{too_many};
    event_block.data.inputEvents = &events;
    check(event_processor.process(event_block.data) == Steinberg::kResultFalse,
          "over-capacity host event list fails closed");
    check(allZero(event_block) && event_block.output_bus.silenceFlags != 0,
          "event overflow clears output and reports silence");

    resonant::vst3::Processor parameter_processor;
    check(prepare(parameter_processor), "prepare automation-overflow processor");
    BlockFixture parameter_block{};
    parameter_block.clearOutput(0.5F);
    CountOnlyParameterChanges parameters{too_many};
    parameter_block.data.inputParameterChanges = &parameters;
    check(parameter_processor.process(parameter_block.data) ==
              Steinberg::kResultFalse,
          "over-capacity automation queue list fails closed");
    check(allZero(parameter_block) &&
              parameter_block.output_bus.silenceFlags != 0,
          "automation overflow clears output and reports silence");
}

template <std::size_t Blocks>
bool renderTrajectory(
    resonant::vst3::Processor& processor,
    std::array<std::uint32_t,
               Blocks * static_cast<std::size_t>(kFrames) * 2U>& signature) {
    BlockFixture block{};
    const auto note = noteOnEvent();
    SingleEventList events{note};

    std::size_t cursor = 0U;
    for (std::size_t block_index = 0U; block_index < Blocks; ++block_index) {
        block.clearOutput();
        block.data.inputEvents = block_index == 0U ? &events : nullptr;
        block.data.inputParameterChanges = nullptr;
        if (processor.process(block.data) != Steinberg::kResultOk) {
            return false;
        }
        for (std::size_t frame = 0U;
             frame < static_cast<std::size_t>(kFrames); ++frame) {
            signature[cursor++] =
                std::bit_cast<std::uint32_t>(block.output_left[frame]);
            signature[cursor++] =
                std::bit_cast<std::uint32_t>(block.output_right[frame]);
        }
    }
    return true;
}

void testLifecycleResetIsDeterministic() {
    resonant::vst3::Processor processor;
    check(prepare(processor), "prepare deterministic reset processor");

    constexpr std::size_t kBlocks = 8U;
    std::array<std::uint32_t,
               kBlocks * static_cast<std::size_t>(kFrames) * 2U> first{};
    std::array<std::uint32_t,
               kBlocks * static_cast<std::size_t>(kFrames) * 2U> second{};

    check(renderTrajectory<kBlocks>(processor, first),
          "render pre-reset deterministic trajectory");
    check(processor.setActive(false) == Steinberg::kResultOk &&
              processor.setActive(true) == Steinberg::kResultOk,
          "VST3 deactivate/reactivate reset succeeds");
    check(renderTrajectory<kBlocks>(processor, second),
          "render post-reset deterministic trajectory");
    check(first == second,
          "VST3 lifecycle reset reproduces the exact trajectory");
}

void testFiniteInputProtectionIsObservable() {
    resonant::vst3::Processor processor;
    check(prepare(processor), "prepare non-finite input processor");

    BlockFixture block{true};
    const auto nan = std::numeric_limits<float>::quiet_NaN();
    block.input_left.fill(nan);
    block.input_right.fill(nan);
    block.clearOutput(0.5F);

    check(processor.process(block.data) == Steinberg::kResultOk,
          "non-finite external input is sanitized by the portable core boundary");
    check(allZero(block),
          "sanitized non-finite external input cannot leak non-finite output");
}

template <std::size_t Instances>
double benchmarkInstances() {
    constexpr std::size_t kWarmupBlocks = 48U;
    constexpr std::size_t kMeasuredBlocks = 320U;
    constexpr std::size_t kRepeats = 5U;

    std::array<resonant::vst3::Processor, Instances> processors{};
    std::array<BlockFixture, Instances> blocks{};
    const auto note = noteOnEvent();
    SingleEventList events{note};

    for (std::size_t index = 0U; index < Instances; ++index) {
        if (!prepare(processors[index])) {
            return 0.0;
        }
        blocks[index].data.inputEvents = &events;
        if (processors[index].process(blocks[index].data) !=
            Steinberg::kResultOk) {
            return 0.0;
        }
        blocks[index].data.inputEvents = nullptr;
    }

    for (std::size_t block_index = 0U;
         block_index < kWarmupBlocks; ++block_index) {
        for (std::size_t index = 0U; index < Instances; ++index) {
            if (processors[index].process(blocks[index].data) !=
                Steinberg::kResultOk) {
                return 0.0;
            }
        }
    }

    std::array<double, kRepeats> measurements{};
    for (std::size_t repeat = 0U; repeat < kRepeats; ++repeat) {
        const auto start = std::chrono::steady_clock::now();
        for (std::size_t block_index = 0U;
             block_index < kMeasuredBlocks; ++block_index) {
            for (std::size_t index = 0U; index < Instances; ++index) {
                if (processors[index].process(blocks[index].data) !=
                    Steinberg::kResultOk) {
                    return 0.0;
                }
            }
        }
        const auto stop = std::chrono::steady_clock::now();
        const auto elapsed =
            std::chrono::duration<double, std::nano>(stop - start).count();
        measurements[repeat] =
            elapsed / static_cast<double>(kMeasuredBlocks);
    }

    std::sort(measurements.begin(), measurements.end());
    return measurements[kRepeats / 2U];
}

void recordCpuScaling() {
    const auto one = benchmarkInstances<1U>();
    const auto four = benchmarkInstances<4U>();
    const auto eight = benchmarkInstances<8U>();

    check(one > 0.0 && four > 0.0 && eight > 0.0 &&
              std::isfinite(one) && std::isfinite(four) && std::isfinite(eight),
          "native wrapper CPU scaling measurements are finite");

    if (one > 0.0) {
        std::cout << "M4.8 CPU_SCALE instances=1 ns_per_128f_host_block="
                  << one << " relative=1\n";
        std::cout << "M4.8 CPU_SCALE instances=4 ns_per_128f_host_block="
                  << four << " relative=" << (four / one) << '\n';
        std::cout << "M4.8 CPU_SCALE instances=8 ns_per_128f_host_block="
                  << eight << " relative=" << (eight / one) << '\n';
    }
}

void* allocate(std::size_t size) {
    if (g_track_allocations.load(std::memory_order_acquire)) {
        g_allocation_count.fetch_add(1U, std::memory_order_relaxed);
    }
    if (void* pointer = std::malloc(size)) {
        return pointer;
    }
    throw std::bad_alloc{};
}

void* allocateAligned(std::size_t size, std::size_t alignment) {
    if (g_track_allocations.load(std::memory_order_acquire)) {
        g_allocation_count.fetch_add(1U, std::memory_order_relaxed);
    }
    const auto adjusted =
        ((std::max(size, std::size_t{1U}) + alignment - 1U) / alignment) *
        alignment;
    if (void* pointer = std::aligned_alloc(alignment, adjusted)) {
        return pointer;
    }
    throw std::bad_alloc{};
}

} // namespace

void* operator new(std::size_t size) { return allocate(size); }
void* operator new[](std::size_t size) { return allocate(size); }
void* operator new(std::size_t size, std::align_val_t alignment) {
    return allocateAligned(size, static_cast<std::size_t>(alignment));
}
void* operator new[](std::size_t size, std::align_val_t alignment) {
    return allocateAligned(size, static_cast<std::size_t>(alignment));
}

void operator delete(void* pointer) noexcept { std::free(pointer); }
void operator delete(void* pointer, std::size_t) noexcept { std::free(pointer); }
void operator delete[](void* pointer) noexcept { std::free(pointer); }
void operator delete[](void* pointer, std::size_t) noexcept { std::free(pointer); }
void operator delete(void* pointer, std::align_val_t) noexcept { std::free(pointer); }
void operator delete(void* pointer, std::size_t, std::align_val_t) noexcept {
    std::free(pointer);
}
void operator delete[](void* pointer, std::align_val_t) noexcept { std::free(pointer); }
void operator delete[](void* pointer, std::size_t, std::align_val_t) noexcept {
    std::free(pointer);
}

int main() {
    testWrapperProcessAllocatesNothing();
    testBoundedHostTranslationFailsClosed();
    testLifecycleResetIsDeterministic();
    testFiniteInputProtectionIsObservable();
    recordCpuScaling();

    if (failures != 0) {
        std::cerr << failures << " M4.8 VST3 realtime/boundedness test(s) failed\n";
        return 1;
    }

    std::cout << "PASS: M4.8 VST3 realtime and boundedness proof\n";
    return 0;
}
