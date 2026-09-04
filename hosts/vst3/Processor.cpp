#include "hosts/vst3/Processor.hpp"

#include "hosts/vst3/PluginIds.hpp"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/vstspeaker.h"

#include <algorithm>
#include <cstdint>

namespace resonant::vst3 {

Processor::Processor() {
    setControllerClass(kControllerUid);
}

Steinberg::tresult PLUGIN_API Processor::initialize(Steinberg::FUnknown* context) {
    const auto result = AudioEffect::initialize(context);
    if (result != Steinberg::kResultOk) {
        return result;
    }

    addAudioOutput(STR16("Stereo Out"), Steinberg::Vst::SpeakerArr::kStereo);
    addEventInput(STR16("Event In"), 1);
    return Steinberg::kResultOk;
}

Steinberg::tresult PLUGIN_API Processor::setupProcessing(
    Steinberg::Vst::ProcessSetup& setup) {
    if (setup.symbolicSampleSize != Steinberg::Vst::kSample32 ||
        setup.maxSamplesPerBlock <= 0 ||
        setup.maxSamplesPerBlock > static_cast<Steinberg::int32>(kMaxBlockSize)) {
        return Steinberg::kResultFalse;
    }

    const auto result = AudioEffect::setupProcessing(setup);
    if (result != Steinberg::kResultOk) {
        return result;
    }

    if (!adapter_.prepare(setup.sampleRate,
                          static_cast<std::uint32_t>(setup.maxSamplesPerBlock),
                          0U,
                          2U)) {
        return Steinberg::kResultFalse;
    }
    return Steinberg::kResultOk;
}

Steinberg::tresult PLUGIN_API Processor::setActive(Steinberg::TBool state) {
    if (adapter_.prepared() && !adapter_.reset()) {
        return Steinberg::kResultFalse;
    }
    return AudioEffect::setActive(state);
}

Steinberg::tresult PLUGIN_API Processor::canProcessSampleSize(
    Steinberg::int32 symbolic_sample_size) {
    return symbolic_sample_size == Steinberg::Vst::kSample32
               ? Steinberg::kResultTrue
               : Steinberg::kResultFalse;
}

Steinberg::tresult PLUGIN_API Processor::process(Steinberg::Vst::ProcessData& data) {
    if (!adapter_.prepared() ||
        data.symbolicSampleSize != Steinberg::Vst::kSample32 ||
        data.numSamples < 0 ||
        data.numSamples > static_cast<Steinberg::int32>(adapter_.spec().max_block_size)) {
        return Steinberg::kResultFalse;
    }

    if (data.numSamples == 0) {
        return Steinberg::kResultOk;
    }

    if (data.numOutputs < 1 || data.outputs == nullptr) {
        return Steinberg::kResultFalse;
    }

    auto& output_bus = data.outputs[0];
    if (output_bus.numChannels <= 0 || output_bus.numChannels > 2 ||
        output_bus.channelBuffers32 == nullptr) {
        return Steinberg::kResultFalse;
    }

    auto** output_buffers = output_bus.channelBuffers32;
    for (Steinberg::int32 channel = 0; channel < output_bus.numChannels; ++channel) {
        if (output_buffers[channel] == nullptr) {
            for (Steinberg::int32 clear_channel = 0;
                 clear_channel < output_bus.numChannels; ++clear_channel) {
                if (output_buffers[clear_channel] != nullptr) {
                    std::fill_n(output_buffers[clear_channel], data.numSamples, 0.0F);
                }
            }
            return Steinberg::kResultFalse;
        }
    }

    const auto status = adapter_.process(
        nullptr,
        0U,
        output_buffers,
        static_cast<std::uint32_t>(output_bus.numChannels),
        static_cast<std::uint32_t>(data.numSamples));

    if (status != ProcessStatus::Ok) {
        std::fill_n(output_buffers[0], data.numSamples, 0.0F);
        if (output_bus.numChannels > 1) {
            std::fill_n(output_buffers[1], data.numSamples, 0.0F);
        }
        output_bus.silenceFlags = ~Steinberg::Vst::SpeakerArrangement{0};
        return Steinberg::kResultFalse;
    }

    output_bus.silenceFlags = 0;
    return Steinberg::kResultOk;
}

} // namespace resonant::vst3
