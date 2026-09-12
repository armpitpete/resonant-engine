#include "hosts/vst3/Processor.hpp"

#include "public.sdk/source/vst/hosting/parameterchanges.h"
#include "pluginterfaces/vst/ivstevents.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <string_view>
#include <vector>

namespace {

int failures = 0;

void check(bool condition, std::string_view name) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << name << '\n';
    }
}

bool prepare(resonant::vst3::Processor& processor) {
    Steinberg::Vst::ProcessSetup setup{};
    setup.processMode = Steinberg::Vst::kRealtime;
    setup.symbolicSampleSize = Steinberg::Vst::kSample32;
    setup.maxSamplesPerBlock = 256;
    setup.sampleRate = 48'000.0;
    return processor.setupProcessing(setup) == Steinberg::kResultOk &&
           processor.setActive(true) == Steinberg::kResultOk;
}

Steinberg::Vst::ProcessData zeroSampleData() {
    Steinberg::Vst::ProcessData data{};
    data.processMode = Steinberg::Vst::kRealtime;
    data.symbolicSampleSize = Steinberg::Vst::kSample32;
    data.numSamples = 0;
    data.numInputs = 0;
    data.numOutputs = 0;
    data.inputs = nullptr;
    data.outputs = nullptr;
    return data;
}

void testParametersFlushNoBuffer() {
    resonant::vst3::Processor processor;
    check(prepare(processor), "prepare processor for no-buffer flush");

    Steinberg::Vst::ParameterChanges changes{1};
    Steinberg::int32 queue_index = 0;
    auto* queue = changes.addParameterData(
        resonant::BreathPipeVoice::kPressure, queue_index);
    check(queue != nullptr, "create zero-point parameter queue");

    auto data = zeroSampleData();
    data.inputParameterChanges = &changes;
    check(processor.process(data) == Steinberg::kResultOk,
          "Steinberg Parameters Flush (no Buffer)");
    (void)processor.setActive(false);
}

void testParametersFlushOnlyNumChannelZero() {
    resonant::vst3::Processor processor;
    check(prepare(processor), "prepare processor for zero-channel flush");

    Steinberg::Vst::ParameterChanges changes{1};
    Steinberg::int32 queue_index = 0;
    auto* queue = changes.addParameterData(
        resonant::BreathPipeVoice::kPressure, queue_index);
    check(queue != nullptr, "create zero-channel zero-point parameter queue");

    Steinberg::Vst::AudioBusBuffers output_bus{};
    output_bus.numChannels = 0;

    auto data = zeroSampleData();
    data.outputs = &output_bus;
    data.inputParameterChanges = &changes;
    check(processor.process(data) == Steinberg::kResultOk,
          "Steinberg Parameters Flush 2 (only numChannel==0)");
    (void)processor.setActive(false);
}

void testParametersFlushNoBufferNoParameterChange() {
    resonant::vst3::Processor processor;
    check(prepare(processor), "prepare processor for empty flush");

    auto data = zeroSampleData();
    data.inputParameterChanges = nullptr;
    check(processor.process(data) == Steinberg::kResultOk,
          "Steinberg Parameters Flush 2 (no Buffer, no parameter change)");
    (void)processor.setActive(false);
}

void testFlushStateFeedsNextRealBlock() {
    resonant::vst3::Processor processor;
    check(prepare(processor), "prepare processor for staged-state proof");

    Steinberg::Vst::ParameterChanges changes{1};
    Steinberg::int32 queue_index = 0;
    auto* queue = changes.addParameterData(
        resonant::BreathPipeVoice::kPressure, queue_index);
    check(queue != nullptr, "create pressure queue");

    Steinberg::int32 point_index = 0;
    check(queue != nullptr &&
              queue->addPoint(0, 1.0, point_index) == Steinberg::kResultOk,
          "add zero-sample pressure state point");

    auto flush = zeroSampleData();
    flush.inputParameterChanges = &changes;
    check(processor.process(flush) == Steinberg::kResultOk,
          "accept zero-sample pressure state without audio");

    constexpr Steinberg::int32 frames = 256;
    std::array<float, static_cast<std::size_t>(frames)> left{};
    std::array<float, static_cast<std::size_t>(frames)> right{};
    std::array<float*, 2> channels{{left.data(), right.data()}};

    Steinberg::Vst::AudioBusBuffers output_bus{};
    output_bus.numChannels = 2;
    output_bus.channelBuffers32 = channels.data();

    Steinberg::Vst::ProcessData data{};
    data.processMode = Steinberg::Vst::kRealtime;
    data.symbolicSampleSize = Steinberg::Vst::kSample32;
    data.numSamples = frames;
    data.numInputs = 0;
    data.numOutputs = 1;
    data.outputs = &output_bus;

    check(processor.process(data) == Steinberg::kResultOk,
          "process real block after zero-sample flush");

    const auto nonzero = std::any_of(
        left.begin(), left.end(),
        [](float sample) { return std::abs(sample) > 0.0F; });
    check(nonzero,
          "staged pressure reaches core through next-block ParameterChange");
    (void)processor.setActive(false);
}

template <std::size_t Capacity>
class FixedEventList final : public Steinberg::Vst::IEventList {
public:
    bool push(const Steinberg::Vst::Event& event) noexcept {
        if (size_ >= events_.size()) {
            return false;
        }
        events_[size_++] = event;
        return true;
    }

    Steinberg::tresult PLUGIN_API queryInterface(
        const Steinberg::TUID, void** object) override {
        if (object != nullptr) {
            *object = nullptr;
        }
        return Steinberg::kNoInterface;
    }
    Steinberg::uint32 PLUGIN_API addRef() override { return 1U; }
    Steinberg::uint32 PLUGIN_API release() override { return 1U; }
    Steinberg::int32 PLUGIN_API getEventCount() override {
        return static_cast<Steinberg::int32>(size_);
    }
    Steinberg::tresult PLUGIN_API getEvent(
        Steinberg::int32 index, Steinberg::Vst::Event& event) override {
        if (index < 0 || static_cast<std::size_t>(index) >= size_) {
            return Steinberg::kResultFalse;
        }
        event = events_[static_cast<std::size_t>(index)];
        return Steinberg::kResultOk;
    }
    Steinberg::tresult PLUGIN_API addEvent(
        Steinberg::Vst::Event& event) override {
        return push(event) ? Steinberg::kResultOk : Steinberg::kResultFalse;
    }

private:
    std::array<Steinberg::Vst::Event, Capacity> events_{};
    std::size_t size_{0U};
};

struct RenderResult {
    bool ok{false};
    double rms{0.0};
    double peak{0.0};
    std::vector<float> samples{};
};

bool flushNormalized(resonant::vst3::Processor& processor,
                     resonant::ParameterId id,
                     double normalized) {
    Steinberg::Vst::ParameterChanges changes{1};
    Steinberg::int32 queue_index = 0;
    auto* queue = changes.addParameterData(
        static_cast<Steinberg::Vst::ParamID>(id), queue_index);
    if (queue == nullptr) {
        return false;
    }
    Steinberg::int32 point_index = 0;
    if (queue->addPoint(0, normalized, point_index) != Steinberg::kResultOk) {
        return false;
    }
    auto data = zeroSampleData();
    data.inputParameterChanges = &changes;
    return processor.process(data) == Steinberg::kResultOk;
}

RenderResult renderAfterFlush(resonant::ParameterId varied,
                              double normalized) {
    constexpr Steinberg::int32 kFrames = 256;
    constexpr std::size_t kBlocks = 180U;
    constexpr std::size_t kWarmupBlocks = 20U;

    resonant::vst3::Processor processor;
    RenderResult result;
    if (!prepare(processor)) {
        return result;
    }

    if (!flushNormalized(processor, resonant::BreathPipeVoice::kTurbulence, 0.25) ||
        !flushNormalized(processor, resonant::BreathPipeVoice::kInteraction, 0.55) ||
        !flushNormalized(processor, resonant::BreathPipeVoice::kDamping, 0.12) ||
        !flushNormalized(processor, resonant::BreathPipeVoice::kRegeneration, 0.12) ||
        !flushNormalized(processor, resonant::BreathPipeVoice::kFeedbackColor, 0.20) ||
        !flushNormalized(processor, resonant::BreathPipeVoice::kNonlinearDrive, 0.30) ||
        !flushNormalized(processor, resonant::BreathPipeVoice::kTimbre, 0.25) ||
        !flushNormalized(processor, resonant::BreathPipeVoice::kPressure, 0.65) ||
        !flushNormalized(processor, varied, normalized)) {
        (void)processor.setActive(false);
        return result;
    }

    Steinberg::Vst::Event note{};
    note.busIndex = 0;
    note.sampleOffset = 0;
    note.type = Steinberg::Vst::Event::kNoteOnEvent;
    note.noteOn.channel = 0;
    note.noteOn.pitch = 60;
    note.noteOn.tuning = 0.0F;
    note.noteOn.velocity = 0.65F;
    note.noteOn.length = 0;
    note.noteOn.noteId = 1;
    FixedEventList<1U> note_events;
    if (!note_events.push(note)) {
        (void)processor.setActive(false);
        return result;
    }

    std::array<float, static_cast<std::size_t>(kFrames)> left{};
    std::array<float, static_cast<std::size_t>(kFrames)> right{};
    std::array<float*, 2U> channels{{left.data(), right.data()}};
    Steinberg::Vst::AudioBusBuffers output_bus{};
    output_bus.numChannels = 2;
    output_bus.channelBuffers32 = channels.data();

    result.samples.reserve((kBlocks - kWarmupBlocks) *
                           static_cast<std::size_t>(kFrames));
    double sum_squares = 0.0;

    for (std::size_t block_index = 0U; block_index < kBlocks; ++block_index) {
        left.fill(0.0F);
        right.fill(0.0F);
        Steinberg::Vst::ProcessData data{};
        data.processMode = Steinberg::Vst::kRealtime;
        data.symbolicSampleSize = Steinberg::Vst::kSample32;
        data.numSamples = kFrames;
        data.numInputs = 0;
        data.numOutputs = 1;
        data.outputs = &output_bus;
        data.inputEvents = block_index == 0U ? &note_events : nullptr;
        if (processor.process(data) != Steinberg::kResultOk) {
            (void)processor.setActive(false);
            return result;
        }
        if (block_index < kWarmupBlocks) {
            continue;
        }
        for (const auto sample : left) {
            const auto value = static_cast<double>(sample);
            result.samples.push_back(sample);
            sum_squares += value * value;
            result.peak = std::max(result.peak, std::abs(value));
        }
    }

    (void)processor.setActive(false);
    if (!result.samples.empty()) {
        result.rms = std::sqrt(sum_squares /
                               static_cast<double>(result.samples.size()));
        result.ok = true;
    }
    return result;
}

double normalizedDifference(const RenderResult& a,
                            const RenderResult& b) {
    const auto count = std::min(a.samples.size(), b.samples.size());
    if (!a.ok || !b.ok || count == 0U) {
        return 0.0;
    }
    double sum = 0.0;
    for (std::size_t i = 0U; i < count; ++i) {
        const auto delta = static_cast<double>(a.samples[i]) -
                           static_cast<double>(b.samples[i]);
        sum += delta * delta;
    }
    const auto diff_rms = std::sqrt(sum / static_cast<double>(count));
    return diff_rms / std::max({a.rms, b.rms, 1.0e-12});
}

void proveFlushLeverage(std::string_view name,
                        resonant::ParameterId id,
                        double low,
                        double high,
                        double minimum_difference) {
    const auto low_render = renderAfterFlush(id, low);
    const auto high_render = renderAfterFlush(id, high);
    const auto difference = normalizedDifference(low_render, high_render);
    check(low_render.ok && high_render.ok,
          "flushed sensitivity renders complete");
    check(difference >= minimum_difference,
          "flushed host parameter retains expected control leverage");
    std::cout << "M4.11 FLUSH_LEVERAGE control=" << name
              << " low_rms=" << low_render.rms
              << " high_rms=" << high_render.rms
              << " normalized_diff=" << difference
              << " minimum_diff=" << minimum_difference << '\n';
}

void testFlushControlLeverage() {
    proveFlushLeverage("Pressure", resonant::BreathPipeVoice::kPressure,
                       0.25, 0.85, 0.30);
    proveFlushLeverage("Turbulence", resonant::BreathPipeVoice::kTurbulence,
                       0.05, 0.85, 0.50);
    proveFlushLeverage("Damping", resonant::BreathPipeVoice::kDamping,
                       0.03, 0.65, 0.35);
    proveFlushLeverage("NonlinearDrive",
                       resonant::BreathPipeVoice::kNonlinearDrive,
                       0.05, 0.90, 0.50);
}

} // namespace

int main() {
    testParametersFlushNoBuffer();
    testParametersFlushOnlyNumChannelZero();
    testParametersFlushNoBufferNoParameterChange();
    testFlushStateFeedsNextRealBlock();
    testFlushControlLeverage();

    if (failures != 0) {
        std::cerr << failures << " VST3 zero-sample flush test(s) failed\n";
        return 1;
    }

    std::cout << "PASS: M4.11 VST3 zero-sample parameter flush and leverage\n";
    return 0;
}
