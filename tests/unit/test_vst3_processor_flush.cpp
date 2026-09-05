#include "hosts/vst3/Processor.hpp"

#include "public.sdk/source/vst/hosting/parameterchanges.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <string_view>

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
    return processor.setupProcessing(setup) == Steinberg::kResultOk;
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
}

void testParametersFlushNoBufferNoParameterChange() {
    resonant::vst3::Processor processor;
    check(prepare(processor), "prepare processor for empty flush");

    auto data = zeroSampleData();
    data.inputParameterChanges = nullptr;
    check(processor.process(data) == Steinberg::kResultOk,
          "Steinberg Parameters Flush 2 (no Buffer, no parameter change)");
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

    bool nonzero = false;
    for (const auto sample : left) {
        if (std::abs(sample) > 0.0F) {
            nonzero = true;
            break;
        }
    }
    check(nonzero,
          "staged pressure reaches core through next-block ParameterChange");
}

} // namespace

int main() {
    testParametersFlushNoBuffer();
    testParametersFlushOnlyNumChannelZero();
    testParametersFlushNoBufferNoParameterChange();
    testFlushStateFeedsNextRealBlock();

    if (failures != 0) {
        std::cerr << failures << " VST3 zero-sample flush test(s) failed\n";
        return 1;
    }

    std::cout << "PASS: M4.5 VST3 zero-sample parameter flush\n";
    return 0;
}
