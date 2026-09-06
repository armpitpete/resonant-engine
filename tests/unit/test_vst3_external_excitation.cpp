#include "hosts/vst3/Processor.hpp"

#include "pluginterfaces/vst/ivstcomponent.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
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

bool prepare(resonant::vst3::Processor& processor,
             Steinberg::int32 max_samples = 128) {
    Steinberg::Vst::ProcessSetup setup{};
    setup.processMode = Steinberg::Vst::kRealtime;
    setup.symbolicSampleSize = Steinberg::Vst::kSample32;
    setup.maxSamplesPerBlock = max_samples;
    setup.sampleRate = 48'000.0;
    return processor.setupProcessing(setup) == Steinberg::kResultOk;
}

Steinberg::tresult processStereo(
    resonant::vst3::Processor& processor,
    Steinberg::int32 frames,
    float* input_left,
    float* input_right,
    Steinberg::int32 input_channels,
    bool provide_input_bus,
    float* output_left,
    float* output_right) {
    std::array<float*, 2U> input_buffers{{input_left, input_right}};
    Steinberg::Vst::AudioBusBuffers input_bus{};
    input_bus.numChannels = input_channels;
    input_bus.channelBuffers32 = input_buffers.data();

    std::array<float*, 2U> output_buffers{{output_left, output_right}};
    Steinberg::Vst::AudioBusBuffers output_bus{};
    output_bus.numChannels = 2;
    output_bus.channelBuffers32 = output_buffers.data();

    Steinberg::Vst::ProcessData data{};
    data.processMode = Steinberg::Vst::kRealtime;
    data.symbolicSampleSize = Steinberg::Vst::kSample32;
    data.numSamples = frames;
    data.numInputs = provide_input_bus ? 1 : 0;
    data.numOutputs = 1;
    data.inputs = provide_input_bus ? &input_bus : nullptr;
    data.outputs = &output_bus;
    return processor.process(data);
}

template <std::size_t N>
bool allZero(const std::array<float, N>& samples) {
    return std::all_of(samples.begin(), samples.end(),
                       [](float sample) { return sample == 0.0F; });
}

void testAuxiliaryBusDeclaration() {
    resonant::vst3::Processor processor;
    check(processor.initialize(nullptr) == Steinberg::kResultOk,
          "initialize processor for external bus declaration");
    check(processor.getBusCount(Steinberg::Vst::kAudio,
                                Steinberg::Vst::kInput) == 1,
          "processor declares one audio input bus");

    Steinberg::Vst::BusInfo info{};
    check(processor.getBusInfo(Steinberg::Vst::kAudio,
                               Steinberg::Vst::kInput,
                               0,
                               info) == Steinberg::kResultOk,
          "read external input bus metadata");
    check(info.channelCount == 2,
          "external excitation bus is stereo");
    check(info.busType == Steinberg::Vst::kAux,
          "external excitation bus is auxiliary");
    check((info.flags & Steinberg::Vst::BusInfo::kDefaultActive) == 0,
          "external excitation bus is not default-active");
    (void)processor.terminate();
}

void testExternalAudioUsesCorePathWithoutDryMonitoring() {
    constexpr std::size_t frames = 128U;

    resonant::vst3::Processor silent;
    check(prepare(silent), "prepare no-input processor");
    std::array<float, frames> silent_left{};
    std::array<float, frames> silent_right{};
    check(processStereo(silent,
                        static_cast<Steinberg::int32>(frames),
                        nullptr,
                        nullptr,
                        0,
                        false,
                        silent_left.data(),
                        silent_right.data()) == Steinberg::kResultOk,
          "process without external input");
    check(allZero(silent_left) && allZero(silent_right),
          "disconnected fresh processor remains exactly silent");

    resonant::vst3::Processor excited;
    check(prepare(excited), "prepare externally excited processor");
    std::array<float, frames> input_left{};
    std::array<float, frames> input_right{};
    input_left.fill(1.0F);
    input_right.fill(1.0F);
    std::array<float, frames> left{};
    std::array<float, frames> right{};

    check(processStereo(excited,
                        static_cast<Steinberg::int32>(frames),
                        input_left.data(),
                        input_right.data(),
                        2,
                        true,
                        left.data(),
                        right.data()) == Steinberg::kResultOk,
          "process stereo external excitation");

    double energy = 0.0;
    for (std::size_t frame = 0U; frame < frames; ++frame) {
        energy += std::abs(static_cast<double>(left[frame]));
        check(left[frame] == right[frame],
              "core mono model still duplicates identically to stereo output");
    }
    check(energy > 1.0e-8,
          "external audio excites the existing Breath Pipe core");
    check(!std::equal(input_left.begin(), input_left.end(), left.begin()),
          "external audio is not dry-monitored into the output");
}

void testInactiveAndMalformedInputHandling() {
    constexpr std::size_t frames = 128U;

    resonant::vst3::Processor inactive;
    check(prepare(inactive), "prepare inactive-input processor");
    std::array<float, frames> inactive_left{};
    std::array<float, frames> inactive_right{};
    check(processStereo(inactive,
                        static_cast<Steinberg::int32>(frames),
                        nullptr,
                        nullptr,
                        2,
                        true,
                        inactive_left.data(),
                        inactive_right.data()) == Steinberg::kResultOk,
          "inactive all-null auxiliary bus is accepted");
    check(allZero(inactive_left) && allZero(inactive_right),
          "inactive auxiliary bus behaves as no external input");

    resonant::vst3::Processor partial;
    check(prepare(partial), "prepare partial-null input processor");
    std::array<float, frames> source{};
    source.fill(1.0F);
    std::array<float, frames> partial_left{};
    std::array<float, frames> partial_right{};
    partial_left.fill(0.5F);
    partial_right.fill(0.5F);
    check(processStereo(partial,
                        static_cast<Steinberg::int32>(frames),
                        source.data(),
                        nullptr,
                        2,
                        true,
                        partial_left.data(),
                        partial_right.data()) == Steinberg::kResultFalse,
          "partially-null stereo input fails closed");
    check(allZero(partial_left) && allZero(partial_right),
          "partial-null failure clears host output");

    resonant::vst3::Processor mono;
    check(prepare(mono), "prepare malformed mono input processor");
    std::array<float, frames> mono_left{};
    std::array<float, frames> mono_right{};
    mono_left.fill(0.5F);
    mono_right.fill(0.5F);
    check(processStereo(mono,
                        static_cast<Steinberg::int32>(frames),
                        source.data(),
                        source.data(),
                        1,
                        true,
                        mono_left.data(),
                        mono_right.data()) == Steinberg::kResultFalse,
          "wrong input channel count fails closed");
    check(allZero(mono_left) && allZero(mono_right),
          "wrong-channel failure clears host output");
}

void testOversizedHostBlockRebasesExternalInput() {
    constexpr std::size_t frames = 5'000U;
    constexpr std::size_t second_chunk = resonant::kMaxBlockSize;

    resonant::vst3::Processor processor;
    check(prepare(processor, static_cast<Steinberg::int32>(frames)),
          "prepare processor for oversized external-input block");

    std::array<float, frames> input_left{};
    std::array<float, frames> input_right{};
    for (std::size_t frame = second_chunk; frame < frames; ++frame) {
        input_left[frame] = 1.0F;
        input_right[frame] = 1.0F;
    }

    std::array<float, frames> left{};
    std::array<float, frames> right{};
    check(processStereo(processor,
                        static_cast<Steinberg::int32>(frames),
                        input_left.data(),
                        input_right.data(),
                        2,
                        true,
                        left.data(),
                        right.data()) == Steinberg::kResultOk,
          "process oversized external-input host block");

    const auto first_chunk_silent =
        std::all_of(left.begin(),
                    left.begin() + static_cast<std::ptrdiff_t>(second_chunk),
                    [](float sample) { return sample == 0.0F; });
    check(first_chunk_silent,
          "first core chunk remains silent before external excitation begins");

    double tail_energy = 0.0;
    for (std::size_t frame = second_chunk; frame < frames; ++frame) {
        tail_energy += std::abs(static_cast<double>(left[frame]));
        check(left[frame] == right[frame],
              "oversized external excitation remains stereo-identical");
    }
    check(tail_energy > 1.0e-8,
          "second core chunk reads rebased external input pointers");
}

} // namespace

int main() {
    testAuxiliaryBusDeclaration();
    testExternalAudioUsesCorePathWithoutDryMonitoring();
    testInactiveAndMalformedInputHandling();
    testOversizedHostBlockRebasesExternalInput();

    if (failures != 0) {
        std::cerr << failures << " M4.7 VST3 external-excitation test(s) failed\n";
        return 1;
    }

    std::cout << "PASS: M4.7 VST3 external excitation\n";
    return 0;
}
