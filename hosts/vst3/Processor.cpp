#include "hosts/vst3/Processor.hpp"

#include "hosts/vst3/PluginIds.hpp"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/vstspeaker.h"

#include <algorithm>
#include <array>
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
        setup.maxSamplesPerBlock <= 0) {
        return Steinberg::kResultFalse;
    }

    const auto result = AudioEffect::setupProcessing(setup);
    if (result != Steinberg::kResultOk) {
        return result;
    }

    const auto core_block_size = static_cast<std::uint32_t>(
        std::min(setup.maxSamplesPerBlock,
                 static_cast<Steinberg::int32>(kMaxBlockSize)));
    if (!adapter_.prepare(setup.sampleRate,
                          core_block_size,
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
        data.numSamples < 0) {
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

    const auto total_frames = static_cast<std::uint32_t>(data.numSamples);
    const auto output_channels = static_cast<std::uint32_t>(output_bus.numChannels);
    std::array<Sample*, kMaxChannels> chunk_outputs{};
    std::uint32_t processed = 0U;
    while (processed < total_frames) {
        const auto chunk_frames =
            std::min(total_frames - processed, adapter_.spec().max_block_size);
        for (std::uint32_t channel = 0U; channel < output_channels; ++channel) {
            chunk_outputs[channel] = output_buffers[channel] + processed;
        }

        const auto status = adapter_.process(
            nullptr,
            0U,
            chunk_outputs.data(),
            output_channels,
            chunk_frames);

        if (status != ProcessStatus::Ok) {
            for (Steinberg::int32 channel = 0; channel < output_bus.numChannels; ++channel) {
                std::fill_n(output_buffers[channel] + processed,
                            data.numSamples - static_cast<Steinberg::int32>(processed),
                            0.0F);
            }
            output_bus.silenceFlags = ~Steinberg::Vst::SpeakerArrangement{0};
            return Steinberg::kResultFalse;
        }
        processed += chunk_frames;
    }

    output_bus.silenceFlags = 0;
    return Steinberg::kResultOk;
}

} // namespace resonant::vst3
