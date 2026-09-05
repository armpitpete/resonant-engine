#include "hosts/vst3/Processor.hpp"

#include "hosts/vst3/ParameterMapping.hpp"
#include "hosts/vst3/PluginIds.hpp"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
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

    event_translator_.reset();
    chunk_events_.clear();
    clearPendingParameters();
    return Steinberg::kResultOk;
}

Steinberg::tresult PLUGIN_API Processor::setActive(Steinberg::TBool state) {
    if (adapter_.prepared() && !adapter_.reset()) {
        return Steinberg::kResultFalse;
    }
    event_translator_.reset();
    chunk_events_.clear();
    return AudioEffect::setActive(state);
}

Steinberg::tresult PLUGIN_API Processor::canProcessSampleSize(
    Steinberg::int32 symbolic_sample_size) {
    return symbolic_sample_size == Steinberg::Vst::kSample32
               ? Steinberg::kResultTrue
               : Steinberg::kResultFalse;
}

bool Processor::translateEvents(Steinberg::Vst::ProcessData& data,
                                std::uint32_t total_frames) noexcept {
    event_translator_.beginBlock(total_frames);
    const auto reject_block = [&]() noexcept {
        event_translator_.abortBlock();
        return false;
    };

    if (data.inputEvents == nullptr) {
        return true;
    }

    const auto event_count = data.inputEvents->getEventCount();
    if (event_count < 0 ||
        event_count > static_cast<Steinberg::int32>(kMaxEventsPerBlock)) {
        return reject_block();
    }

    for (Steinberg::int32 index = 0; index < event_count; ++index) {
        Steinberg::Vst::Event host_event{};
        if (data.inputEvents->getEvent(index, host_event) != Steinberg::kResultOk ||
            host_event.busIndex != 0 ||
            host_event.sampleOffset < 0 ||
            host_event.sampleOffset >= data.numSamples) {
            return reject_block();
        }

        const auto sample_offset =
            static_cast<std::uint32_t>(host_event.sampleOffset);

        switch (host_event.type) {
        case Steinberg::Vst::Event::kNoteOnEvent:
            if (!event_translator_.noteOn(
                    sample_offset,
                    host_event.noteOn.pitch,
                    host_event.noteOn.tuning,
                    host_event.noteOn.velocity,
                    host_event.noteOn.noteId)) {
                return reject_block();
            }
            break;

        case Steinberg::Vst::Event::kNoteOffEvent:
            if (!event_translator_.noteOff(
                    sample_offset,
                    host_event.noteOff.pitch,
                    host_event.noteOff.velocity,
                    host_event.noteOff.noteId)) {
                return reject_block();
            }
            break;

        case Steinberg::Vst::Event::kPolyPressureEvent:
            if (!event_translator_.polyPressure(
                    sample_offset,
                    host_event.polyPressure.pitch,
                    host_event.polyPressure.pressure,
                    host_event.polyPressure.noteId)) {
                return reject_block();
            }
            break;

        case Steinberg::Vst::Event::kNoteExpressionValueEvent:
            if (host_event.noteExpressionValue.typeId ==
                    Steinberg::Vst::kTuningTypeID &&
                !event_translator_.noteExpressionTuning(
                    sample_offset,
                    host_event.noteExpressionValue.value,
                    host_event.noteExpressionValue.noteId)) {
                return reject_block();
            }
            break;

        default:
            break;
        }
    }

    if (!event_translator_.valid()) {
        return reject_block();
    }
    return true;
}

bool Processor::translateParameters(Steinberg::Vst::ProcessData& data,
                                    std::uint32_t total_frames) noexcept {
    if (data.inputParameterChanges == nullptr) {
        return true;
    }

    const auto reject_block = [&]() noexcept {
        event_translator_.abortBlock();
        return false;
    };

    const auto queue_count = data.inputParameterChanges->getParameterCount();
    if (queue_count < 0 ||
        queue_count > static_cast<Steinberg::int32>(kMaxEventsPerBlock)) {
        return reject_block();
    }

    for (Steinberg::int32 queue_index = 0;
         queue_index < queue_count; ++queue_index) {
        auto* queue = data.inputParameterChanges->getParameterData(queue_index);
        if (queue == nullptr) {
            return reject_block();
        }

        const auto host_id =
            static_cast<HostParamId>(queue->getParameterId());
        const auto* spec = HostParameterMapping::specForHostId(host_id);
        const auto point_count = queue->getPointCount();
        if (point_count < 0 ||
            point_count > static_cast<Steinberg::int32>(kMaxEventsPerBlock)) {
            return reject_block();
        }

        // Unknown, hidden, internal or topology parameters are not part of
        // this portable host projection. Ignore their queues safely.
        if (spec == nullptr) {
            continue;
        }

        for (Steinberg::int32 point_index = 0;
             point_index < point_count; ++point_index) {
            Steinberg::int32 sample_offset = 0;
            Steinberg::Vst::ParamValue normalized = 0.0;
            if (queue->getPoint(point_index, sample_offset, normalized) !=
                    Steinberg::kResultOk ||
                sample_offset < 0 ||
                sample_offset >= static_cast<Steinberg::int32>(total_frames)) {
                return reject_block();
            }

            Sample native = 0.0F;
            if (!HostParameterMapping::normalizedToNative(
                    host_id, normalized, native) ||
                !event_translator_.parameterChange(
                    static_cast<std::uint32_t>(sample_offset),
                    spec->id,
                    native)) {
                return reject_block();
            }
        }
    }

    if (!event_translator_.valid()) {
        return reject_block();
    }
    return true;
}

bool Processor::stageFlushParameters(
    Steinberg::Vst::ProcessData& data) noexcept {
    if (data.inputEvents != nullptr && data.inputEvents->getEventCount() != 0) {
        return false;
    }
    if (data.inputParameterChanges == nullptr) {
        return true;
    }

    const auto queue_count = data.inputParameterChanges->getParameterCount();
    if (queue_count < 0 ||
        queue_count > static_cast<Steinberg::int32>(kMaxEventsPerBlock)) {
        return false;
    }

    auto staged_values = pending_parameter_values_;
    auto staged_set = pending_parameter_set_;

    for (Steinberg::int32 queue_index = 0;
         queue_index < queue_count; ++queue_index) {
        auto* queue = data.inputParameterChanges->getParameterData(queue_index);
        // Steinberg's flush conformance tests can present an empty parameter
        // collection with no concrete queue. With zero samples this means
        // "no state change", not a malformed audio block.
        if (queue == nullptr) {
            continue;
        }

        const auto host_id =
            static_cast<HostParamId>(queue->getParameterId());
        const auto* spec = HostParameterMapping::specForHostId(host_id);
        const auto point_count = queue->getPointCount();
        if (point_count < 0 ||
            point_count > static_cast<Steinberg::int32>(kMaxEventsPerBlock)) {
            return false;
        }

        // Unknown/hidden parameters are outside the portable projection.
        if (spec == nullptr || point_count == 0) {
            continue;
        }

        std::size_t parameter_index = BreathPipeVoice::kParameterSpecs.size();
        for (std::size_t index = 0U;
             index < BreathPipeVoice::kParameterSpecs.size(); ++index) {
            if (BreathPipeVoice::kParameterSpecs[index].id == spec->id) {
                parameter_index = index;
                break;
            }
        }
        if (parameter_index == BreathPipeVoice::kParameterSpecs.size()) {
            return false;
        }

        Sample native = 0.0F;
        for (Steinberg::int32 point_index = 0;
             point_index < point_count; ++point_index) {
            Steinberg::int32 sample_offset = 0;
            Steinberg::Vst::ParamValue normalized = 0.0;
            if (queue->getPoint(point_index, sample_offset, normalized) !=
                    Steinberg::kResultOk ||
                sample_offset != 0 ||
                !HostParameterMapping::normalizedToNative(
                    host_id, normalized, native)) {
                return false;
            }
        }

        // A zero-sample flush has no DSP timeline. Retain only the final
        // value for each parameter and apply it through the ordinary portable
        // ParameterChange path at sample zero of the next real audio block.
        staged_values[parameter_index] = native;
        staged_set[parameter_index] = true;
    }

    pending_parameter_values_ = staged_values;
    pending_parameter_set_ = staged_set;
    return true;
}

bool Processor::appendPendingParameters() noexcept {
    for (std::size_t index = 0U;
         index < BreathPipeVoice::kParameterSpecs.size(); ++index) {
        if (!pending_parameter_set_[index]) {
            continue;
        }
        if (!event_translator_.parameterChange(
                0U,
                BreathPipeVoice::kParameterSpecs[index].id,
                pending_parameter_values_[index])) {
            return false;
        }
    }
    return true;
}

void Processor::clearPendingParameters() noexcept {
    pending_parameter_set_.fill(false);
}

Steinberg::tresult PLUGIN_API Processor::process(Steinberg::Vst::ProcessData& data) {
    if (!adapter_.prepared() ||
        data.symbolicSampleSize != Steinberg::Vst::kSample32 ||
        data.numSamples < 0) {
        return Steinberg::kResultFalse;
    }

    if (data.numSamples == 0) {
        // VST3 explicitly permits parameter flushing without audio buffers.
        // Do not invent a one-sample DSP block: retain validated control state
        // and feed it into the next real block through the portable event path.
        return stageFlushParameters(data)
                   ? Steinberg::kResultOk
                   : Steinberg::kResultFalse;
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

    const auto fail_closed = [&](std::uint32_t start_frame) {
        for (Steinberg::int32 channel = 0; channel < output_bus.numChannels; ++channel) {
            std::fill_n(
                output_buffers[channel] + start_frame,
                data.numSamples - static_cast<Steinberg::int32>(start_frame),
                0.0F);
        }
        output_bus.silenceFlags = ~Steinberg::Vst::SpeakerArrangement{0};
        return Steinberg::kResultFalse;
    };

    if (!translateEvents(data, total_frames) ||
        !appendPendingParameters() ||
        !translateParameters(data, total_frames)) {
        return fail_closed(0U);
    }

    std::array<Sample*, kMaxChannels> chunk_outputs{};
    std::uint32_t processed = 0U;
    while (processed < total_frames) {
        const auto chunk_frames =
            std::min(total_frames - processed, adapter_.spec().max_block_size);
        for (std::uint32_t channel = 0U; channel < output_channels; ++channel) {
            chunk_outputs[channel] = output_buffers[channel] + processed;
        }

        if (!event_translator_.buildChunk(
                processed, chunk_frames, chunk_events_)) {
            return fail_closed(processed);
        }

        const auto status = adapter_.process(
            nullptr,
            0U,
            chunk_outputs.data(),
            output_channels,
            chunk_frames,
            chunk_events_.span());

        if (status != ProcessStatus::Ok) {
            return fail_closed(processed);
        }
        processed += chunk_frames;
    }

    clearPendingParameters();
    output_bus.silenceFlags = 0;
    return Steinberg::kResultOk;
}

} // namespace resonant::vst3
