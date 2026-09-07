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
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <new>
#include <span>
#include <string_view>

namespace {

std::atomic<bool> g_track_allocations{false};
std::atomic<std::size_t> g_allocation_count{0U};
int failures = 0;

constexpr Steinberg::int32 kFrames = 128;
constexpr double kSampleRate = 48'000.0;
constexpr double kDeadlineNs =
    1'000'000'000.0 * static_cast<double>(kFrames) / kSampleRate;
constexpr double kOneInstanceBudgetPercent = 25.0;
constexpr double kFourInstanceBudgetPercent = 70.0;
constexpr double kEightInstanceBudgetPercent = 100.0;

void check(bool condition, std::string_view name) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << name << '\n';
    }
}

bool prepare(resonant::vst3::Processor& processor,
             Steinberg::int32 max_frames = kFrames) {
    Steinberg::Vst::ProcessSetup setup{};
    setup.processMode = Steinberg::Vst::kRealtime;
    setup.symbolicSampleSize = Steinberg::Vst::kSample32;
    setup.maxSamplesPerBlock = max_frames;
    setup.sampleRate = kSampleRate;
    return processor.setupProcessing(setup) == Steinberg::kResultOk &&
           processor.setActive(true) == Steinberg::kResultOk;
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

class ChunkedStream final : public Steinberg::IBStream {
public:
    void load(std::span<const std::byte> bytes) noexcept {
        size_ = std::min(bytes.size(), storage_.size());
        std::copy_n(bytes.begin(), size_, storage_.begin());
        position_ = 0U;
    }

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

    Steinberg::tresult PLUGIN_API read(
        void* buffer,
        Steinberg::int32 num_bytes,
        Steinberg::int32* num_bytes_read) override {
        if (num_bytes_read != nullptr) {
            *num_bytes_read = 0;
        }
        if (buffer == nullptr || num_bytes < 0 || position_ >= size_) {
            return Steinberg::kResultFalse;
        }
        const auto available = size_ - position_;
        const auto requested = static_cast<std::size_t>(num_bytes);
        const auto count = std::min(available, requested);
        if (count == 0U) {
            return Steinberg::kResultFalse;
        }
        std::memcpy(buffer, storage_.data() + position_, count);
        position_ += count;
        if (num_bytes_read != nullptr) {
            *num_bytes_read = static_cast<Steinberg::int32>(count);
        }
        return Steinberg::kResultOk;
    }

    Steinberg::tresult PLUGIN_API write(
        void*,
        Steinberg::int32,
        Steinberg::int32*) override {
        return Steinberg::kNotImplemented;
    }

    Steinberg::tresult PLUGIN_API seek(
        Steinberg::int64 pos,
        Steinberg::int32 mode,
        Steinberg::int64* result) override {
        Steinberg::int64 base = 0;
        switch (mode) {
        case Steinberg::IBStream::kIBSeekSet:
            base = 0;
            break;
        case Steinberg::IBStream::kIBSeekCur:
            base = static_cast<Steinberg::int64>(position_);
            break;
        case Steinberg::IBStream::kIBSeekEnd:
            base = static_cast<Steinberg::int64>(size_);
            break;
        default:
            return Steinberg::kResultFalse;
        }
        const auto next = base + pos;
        if (next < 0 || next > static_cast<Steinberg::int64>(size_)) {
            return Steinberg::kResultFalse;
        }
        position_ = static_cast<std::size_t>(next);
        if (result != nullptr) {
            *result = next;
        }
        return Steinberg::kResultOk;
    }

    Steinberg::tresult PLUGIN_API tell(Steinberg::int64* pos) override {
        if (pos == nullptr) {
            return Steinberg::kResultFalse;
        }
        *pos = static_cast<Steinberg::int64>(position_);
        return Steinberg::kResultOk;
    }

private:
    std::array<std::byte, 256U> storage_{};
    std::size_t size_{0U};
    std::size_t position_{0U};
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

    BlockFixture() noexcept : BlockFixture(false) {}

    explicit BlockFixture(bool with_input) noexcept {
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

template <class Fn>
Steinberg::tresult trackedProcess(Fn&& fn, std::string_view allocation_name) {
    g_allocation_count.store(0U, std::memory_order_relaxed);
    g_track_allocations.store(true, std::memory_order_release);
    const auto status = fn();
    g_track_allocations.store(false, std::memory_order_release);
    check(g_allocation_count.load(std::memory_order_acquire) == 0U,
          allocation_name);
    return status;
}

void addParameterChange(
    Steinberg::Vst::ParameterChanges& changes,
    Steinberg::Vst::ParamID id,
    Steinberg::int32 sample_offset,
    Steinberg::Vst::ParamValue normalized) {
    Steinberg::int32 queue_index = 0;
    auto* queue = changes.addParameterData(id, queue_index);
    Steinberg::int32 point_index = 0;
    check(queue != nullptr &&
              queue->addPoint(sample_offset, normalized, point_index) ==
                  Steinberg::kResultOk,
          "prepare parameter change before allocation tracking");
}

void testAllocationFreeProcessPaths() {
    {
        resonant::vst3::Processor processor;
        check(prepare(processor), "prepare ordinary allocation proof");
        BlockFixture block{true};
        block.input_left.fill(0.05F);
        block.input_right.fill(-0.03F);
        const auto note = noteOnEvent();
        SingleEventList events{note};
        Steinberg::Vst::ParameterChanges changes{1};
        addParameterChange(
            changes, resonant::BreathPipeVoice::kTurbulence, 64, 0.35);
        block.data.inputEvents = &events;
        block.data.inputParameterChanges = &changes;
        check(trackedProcess(
                  [&] { return processor.process(block.data); },
                  "ordinary VST3 process path performs zero dynamic allocations") ==
                  Steinberg::kResultOk,
              "ordinary tracked VST3 process call succeeds");
    }

    {
        resonant::vst3::Processor processor;
        check(prepare(processor), "prepare zero-sample flush allocation proof");
        Steinberg::Vst::ParameterChanges changes{1};
        addParameterChange(
            changes, resonant::BreathPipeVoice::kPressure, 0, 0.4);
        Steinberg::Vst::ProcessData flush{};
        flush.processMode = Steinberg::Vst::kRealtime;
        flush.symbolicSampleSize = Steinberg::Vst::kSample32;
        flush.numSamples = 0;
        flush.inputParameterChanges = &changes;
        check(trackedProcess(
                  [&] { return processor.process(flush); },
                  "zero-sample parameter flush performs zero dynamic allocations") ==
                  Steinberg::kResultOk,
              "tracked zero-sample flush succeeds");
    }

    {
        resonant::vst3::Processor processor;
        check(prepare(processor), "prepare state-handoff allocation proof");
        auto state = resonant::defaultBreathPipeState();
        state.seed = 0x123456789abcdef0ULL;
        state.parameters[1].value = 0.61F;
        std::array<std::byte, resonant::BreathPipeStateCodec::kEncodedSize> bytes{};
        check(resonant::BreathPipeStateCodec::encode(state, bytes),
              "encode queued portable state before allocation tracking");
        ChunkedStream stream{};
        stream.load(bytes);
        check(processor.setState(&stream) == Steinberg::kResultOk,
              "queue portable state before allocation tracking");
        BlockFixture block{};
        check(trackedProcess(
                  [&] { return processor.process(block.data); },
                  "queued state audio-boundary apply performs zero dynamic allocations") ==
                  Steinberg::kResultOk,
              "tracked queued-state process succeeds");
    }

    {
        resonant::vst3::Processor processor;
        check(prepare(processor), "prepare event-overflow allocation proof");
        BlockFixture block{};
        block.clearOutput(0.5F);
        CountOnlyEventList events{
            static_cast<Steinberg::int32>(resonant::kMaxEventsPerBlock + 1U)};
        block.data.inputEvents = &events;
        check(trackedProcess(
                  [&] { return processor.process(block.data); },
                  "fail-closed event overflow performs zero dynamic allocations") ==
                  Steinberg::kResultFalse,
              "tracked event overflow fails closed");
        check(allZero(block), "tracked event overflow clears output");
    }

    {
        resonant::vst3::Processor processor;
        check(prepare(processor), "prepare automation-overflow allocation proof");
        BlockFixture block{};
        block.clearOutput(0.5F);
        CountOnlyParameterChanges changes{
            static_cast<Steinberg::int32>(resonant::kMaxEventsPerBlock + 1U)};
        block.data.inputParameterChanges = &changes;
        check(trackedProcess(
                  [&] { return processor.process(block.data); },
                  "fail-closed automation overflow performs zero dynamic allocations") ==
                  Steinberg::kResultFalse,
              "tracked automation overflow fails closed");
        check(allZero(block), "tracked automation overflow clears output");
    }

    {
        constexpr Steinberg::int32 kOversizedFrames = 5'000;
        resonant::vst3::Processor processor;
        check(prepare(processor, kOversizedFrames),
              "prepare oversized-block allocation proof");

        std::array<float, static_cast<std::size_t>(kOversizedFrames)> input_left{};
        std::array<float, static_cast<std::size_t>(kOversizedFrames)> input_right{};
        std::array<float, static_cast<std::size_t>(kOversizedFrames)> output_left{};
        std::array<float, static_cast<std::size_t>(kOversizedFrames)> output_right{};
        input_left.fill(0.02F);
        input_right.fill(-0.01F);

        std::array<float*, 2U> inputs{{input_left.data(), input_right.data()}};
        std::array<float*, 2U> outputs{{output_left.data(), output_right.data()}};
        Steinberg::Vst::AudioBusBuffers input_bus{};
        input_bus.numChannels = 2;
        input_bus.channelBuffers32 = inputs.data();
        Steinberg::Vst::AudioBusBuffers output_bus{};
        output_bus.numChannels = 2;
        output_bus.channelBuffers32 = outputs.data();

        Steinberg::Vst::ProcessData data{};
        data.processMode = Steinberg::Vst::kRealtime;
        data.symbolicSampleSize = Steinberg::Vst::kSample32;
        data.numSamples = kOversizedFrames;
        data.numInputs = 1;
        data.numOutputs = 1;
        data.inputs = &input_bus;
        data.outputs = &output_bus;

        check(trackedProcess(
                  [&] { return processor.process(data); },
                  "oversized host-block chunking performs zero dynamic allocations") ==
                  Steinberg::kResultOk,
              "tracked oversized host block succeeds");
    }
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
    constexpr std::size_t kWarmupBlocks = 64U;
    constexpr std::size_t kMeasuredBlocks = 512U;
    constexpr std::size_t kRepeats = 7U;

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

double realtimeLoadPercent(double elapsed_ns) {
    return 100.0 * elapsed_ns / kDeadlineNs;
}

void recordCpuScaling() {
    const auto one = benchmarkInstances<1U>();
    const auto four = benchmarkInstances<4U>();
    const auto eight = benchmarkInstances<8U>();

    check(one > 0.0 && four > 0.0 && eight > 0.0 &&
              std::isfinite(one) && std::isfinite(four) && std::isfinite(eight),
          "native wrapper CPU scaling measurements are finite");

    if (one <= 0.0 || four <= 0.0 || eight <= 0.0) {
        return;
    }

    const auto one_load = realtimeLoadPercent(one);
    const auto four_load = realtimeLoadPercent(four);
    const auto eight_load = realtimeLoadPercent(eight);

    check(one_load < kOneInstanceBudgetPercent,
          "one VST3 instance stays under frozen 25% realtime budget");
    check(four_load < kFourInstanceBudgetPercent,
          "four VST3 instances stay under frozen 70% realtime budget");
    check(eight_load < kEightInstanceBudgetPercent,
          "eight-instance stress stays inside the realtime deadline");

    std::cout << "M4.8 CPU_DEADLINE sample_rate=" << kSampleRate
              << " frames=" << kFrames
              << " deadline_ns=" << kDeadlineNs << '\n';
    std::cout << "M4.8 CPU_SCALE instances=1 ns_per_128f_host_block="
              << one << " realtime_load_percent=" << one_load
              << " budget_percent=" << kOneInstanceBudgetPercent << '\n';
    std::cout << "M4.8 CPU_SCALE instances=4 ns_per_128f_host_block="
              << four << " realtime_load_percent=" << four_load
              << " budget_percent=" << kFourInstanceBudgetPercent << '\n';
    std::cout << "M4.8 CPU_SCALE instances=8 ns_per_128f_host_block="
              << eight << " realtime_load_percent=" << eight_load
              << " budget_percent=" << kEightInstanceBudgetPercent << '\n';
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
void* operator new(std::size_t size, const std::nothrow_t&) noexcept {
    try {
        return allocate(size);
    } catch (...) {
        return nullptr;
    }
}
void* operator new[](std::size_t size, const std::nothrow_t&) noexcept {
    try {
        return allocate(size);
    } catch (...) {
        return nullptr;
    }
}
void* operator new(std::size_t size, std::align_val_t alignment) {
    return allocateAligned(size, static_cast<std::size_t>(alignment));
}
void* operator new[](std::size_t size, std::align_val_t alignment) {
    return allocateAligned(size, static_cast<std::size_t>(alignment));
}
void* operator new(std::size_t size,
                   std::align_val_t alignment,
                   const std::nothrow_t&) noexcept {
    try {
        return allocateAligned(size, static_cast<std::size_t>(alignment));
    } catch (...) {
        return nullptr;
    }
}
void* operator new[](std::size_t size,
                     std::align_val_t alignment,
                     const std::nothrow_t&) noexcept {
    try {
        return allocateAligned(size, static_cast<std::size_t>(alignment));
    } catch (...) {
        return nullptr;
    }
}

void operator delete(void* pointer) noexcept { std::free(pointer); }
void operator delete(void* pointer, std::size_t) noexcept { std::free(pointer); }
void operator delete[](void* pointer) noexcept { std::free(pointer); }
void operator delete[](void* pointer, std::size_t) noexcept { std::free(pointer); }
void operator delete(void* pointer, const std::nothrow_t&) noexcept {
    std::free(pointer);
}
void operator delete[](void* pointer, const std::nothrow_t&) noexcept {
    std::free(pointer);
}
void operator delete(void* pointer, std::align_val_t) noexcept { std::free(pointer); }
void operator delete(void* pointer, std::size_t, std::align_val_t) noexcept {
    std::free(pointer);
}
void operator delete[](void* pointer, std::align_val_t) noexcept { std::free(pointer); }
void operator delete[](void* pointer, std::size_t, std::align_val_t) noexcept {
    std::free(pointer);
}
void operator delete(void* pointer,
                     std::align_val_t,
                     const std::nothrow_t&) noexcept {
    std::free(pointer);
}
void operator delete[](void* pointer,
                       std::align_val_t,
                       const std::nothrow_t&) noexcept {
    std::free(pointer);
}

int main() {
    testAllocationFreeProcessPaths();
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
