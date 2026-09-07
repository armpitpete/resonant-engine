#include "hosts/vst3/Processor.hpp"

#include "public.sdk/source/vst/hosting/parameterchanges.h"

#include "resonant/BreathPipe.hpp"
#include "resonant/Engine.hpp"
#include "resonant/Event.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string_view>

namespace {

int failures = 0;

constexpr double kSampleRate = 48'000.0;
constexpr float kAbsoluteTolerance = 2.0e-6F;
constexpr float kRelativeTolerance = 2.0e-4F;

void check(bool condition, std::string_view name) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << name << '\n';
    }
}

template <std::size_t Capacity>
class FixedHostEventList final : public Steinberg::Vst::IEventList {
public:
    bool push(const Steinberg::Vst::Event& event) noexcept {
        if (size_ >= events_.size()) {
            return false;
        }
        events_[size_++] = event;
        return true;
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

    Steinberg::int32 PLUGIN_API getEventCount() override {
        return static_cast<Steinberg::int32>(size_);
    }

    Steinberg::tresult PLUGIN_API getEvent(
        Steinberg::int32 index,
        Steinberg::Vst::Event& event) override {
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

Steinberg::Vst::Event hostNoteOn(Steinberg::int32 offset,
                                 Steinberg::int32 pitch,
                                 float tuning_cents,
                                 float velocity,
                                 Steinberg::int32 note_id) {
    Steinberg::Vst::Event event{};
    event.busIndex = 0;
    event.sampleOffset = offset;
    event.type = Steinberg::Vst::Event::kNoteOnEvent;
    event.noteOn.channel = 0;
    event.noteOn.pitch =
        static_cast<decltype(event.noteOn.pitch)>(pitch);
    event.noteOn.tuning = tuning_cents;
    event.noteOn.velocity = velocity;
    event.noteOn.length = 0;
    event.noteOn.noteId = note_id;
    return event;
}

Steinberg::Vst::Event hostNoteOff(Steinberg::int32 offset,
                                  Steinberg::int32 pitch,
                                  float release_velocity,
                                  Steinberg::int32 note_id) {
    Steinberg::Vst::Event event{};
    event.busIndex = 0;
    event.sampleOffset = offset;
    event.type = Steinberg::Vst::Event::kNoteOffEvent;
    event.noteOff.channel = 0;
    event.noteOff.pitch =
        static_cast<decltype(event.noteOff.pitch)>(pitch);
    event.noteOff.tuning = 0.0F;
    event.noteOff.velocity = release_velocity;
    event.noteOff.noteId = note_id;
    return event;
}

resonant::Sample portablePitch(Steinberg::int32 pitch,
                               float tuning_cents) {
    const auto semitones =
        static_cast<double>(pitch - 69) +
        static_cast<double>(tuning_cents) / 100.0;
    return static_cast<resonant::Sample>(
        440.0 * std::exp2(semitones / 12.0));
}

resonant::NoteId portableNoteId(Steinberg::int32 host_note_id) {
    return host_note_id >= 0
               ? static_cast<resonant::NoteId>(
                     static_cast<std::uint32_t>(host_note_id) + 1U)
               : resonant::kNoNoteId;
}

bool pushPortableNoteOn(
    resonant::FixedEventBuffer<resonant::kMaxEventsPerBlock>& events,
    std::uint32_t offset,
    Steinberg::int32 pitch,
    float tuning_cents,
    float velocity,
    Steinberg::int32 host_note_id) {
    const auto pitch_hz = portablePitch(pitch, tuning_cents);
    const auto note_id = portableNoteId(host_note_id);
    return events.push(
               {offset, resonant::EventType::Pitch, 0U,
                resonant::kNoNoteId, pitch_hz, 0.0F}) &&
           events.push(
               {offset, resonant::EventType::Pressure, 0U,
                resonant::kNoNoteId, velocity, 0.0F}) &&
           events.push(
               {offset, resonant::EventType::NoteOn, 0U,
                note_id, velocity, 0.0F});
}

bool pushPortableNoteOff(
    resonant::FixedEventBuffer<resonant::kMaxEventsPerBlock>& events,
    std::uint32_t offset,
    float release_velocity,
    Steinberg::int32 host_note_id) {
    const auto note_id = portableNoteId(host_note_id);
    return events.push(
               {offset, resonant::EventType::NoteOff, 0U,
                note_id, release_velocity, 0.0F}) &&
           events.push(
               {offset, resonant::EventType::Pressure, 0U,
                resonant::kNoNoteId, 0.0F, 0.0F});
}

resonant::Sample portableNativeValue(resonant::ParameterId id,
                                     double normalized) {
    const auto* spec = resonant::BreathPipeVoice::parameterSpec(id);
    if (spec == nullptr) {
        return std::numeric_limits<resonant::Sample>::quiet_NaN();
    }
    return spec->denormalize(static_cast<resonant::Sample>(normalized));
}

bool pushPortableParameter(
    resonant::FixedEventBuffer<resonant::kMaxEventsPerBlock>& events,
    std::uint32_t offset,
    resonant::ParameterId id,
    double normalized) {
    const auto native = portableNativeValue(id, normalized);
    return std::isfinite(native) &&
           events.push(
               {offset, resonant::EventType::ParameterChange, id,
                resonant::kNoNoteId, native, 0.0F});
}

void addHostParameterPoint(Steinberg::Vst::ParameterChanges& changes,
                           Steinberg::Vst::ParamID id,
                           Steinberg::int32 offset,
                           double normalized) {
    Steinberg::int32 queue_index = 0;
    auto* queue = changes.addParameterData(id, queue_index);
    check(queue != nullptr, "create host automation queue");
    if (queue == nullptr) {
        return;
    }
    Steinberg::int32 point_index = 0;
    check(queue->addPoint(offset, normalized, point_index) ==
              Steinberg::kResultOk,
          "add host automation point");
}

template <std::size_t Frames>
void fillExternalInput(std::array<float, Frames>& left,
                       std::array<float, Frames>& right,
                       std::size_t start_frame) {
    for (std::size_t frame = 0U; frame < Frames; ++frame) {
        if (frame < start_frame) {
            left[frame] = 0.0F;
            right[frame] = 0.0F;
            continue;
        }
        const auto a = static_cast<int>(frame % 23U) - 11;
        const auto b = static_cast<int>((frame * 3U) % 29U) - 14;
        left[frame] = static_cast<float>(a) * 0.0017F;
        right[frame] = static_cast<float>(b) * -0.0013F;
    }
}

template <std::size_t Frames>
bool renderDirect(
    const std::array<float, Frames>& input_left,
    const std::array<float, Frames>& input_right,
    const resonant::FixedEventBuffer<resonant::kMaxEventsPerBlock>&
        absolute_events,
    std::array<float, Frames>& output_left,
    std::array<float, Frames>& output_right) {
    resonant::Engine<resonant::BreathPipeVoice> engine{
        resonant::BreathPipeVoice{resonant::kDefaultSeed}};
    const resonant::ProcessSpec spec{
        kSampleRate,
        resonant::kMaxBlockSize,
        2U,
        2U,
    };
    if (!engine.prepare(spec)) {
        return false;
    }

    std::uint32_t start = 0U;
    while (start < static_cast<std::uint32_t>(Frames)) {
        const auto remaining =
            static_cast<std::uint32_t>(Frames) - start;
        const auto frames =
            std::min(resonant::kMaxBlockSize, remaining);

        resonant::FixedEventBuffer<resonant::kMaxEventsPerBlock> chunk_events;
        const auto end = start + frames;
        for (const auto& event : absolute_events.span()) {
            if (event.sample_offset < start) {
                continue;
            }
            if (event.sample_offset >= end) {
                break;
            }
            auto rebased = event;
            rebased.sample_offset -= start;
            if (!chunk_events.push(rebased)) {
                return false;
            }
        }

        const std::array<const float*, 2U> inputs{{
            input_left.data() + start,
            input_right.data() + start,
        }};
        const std::array<float*, 2U> outputs{{
            output_left.data() + start,
            output_right.data() + start,
        }};
        const resonant::AudioBlockView block{
            inputs.data(),
            outputs.data(),
            2U,
            2U,
            frames,
        };
        if (engine.process(block, chunk_events.span()) !=
            resonant::ProcessStatus::Ok) {
            return false;
        }
        start += frames;
    }
    return true;
}

template <std::size_t Frames, std::size_t EventCapacity>
bool renderVst3(
    const std::array<float, Frames>& input_left,
    const std::array<float, Frames>& input_right,
    FixedHostEventList<EventCapacity>& events,
    Steinberg::Vst::ParameterChanges& changes,
    std::array<float, Frames>& output_left,
    std::array<float, Frames>& output_right) {
    resonant::vst3::Processor processor;

    Steinberg::Vst::ProcessSetup setup{};
    setup.processMode = Steinberg::Vst::kRealtime;
    setup.symbolicSampleSize = Steinberg::Vst::kSample32;
    setup.maxSamplesPerBlock =
        static_cast<Steinberg::int32>(Frames);
    setup.sampleRate = kSampleRate;

    if (processor.setupProcessing(setup) != Steinberg::kResultOk ||
        processor.setActive(true) != Steinberg::kResultOk) {
        return false;
    }

    std::array<float*, 2U> input_channels{{
        const_cast<float*>(input_left.data()),
        const_cast<float*>(input_right.data()),
    }};
    std::array<float*, 2U> output_channels{{
        output_left.data(),
        output_right.data(),
    }};

    Steinberg::Vst::AudioBusBuffers input_bus{};
    input_bus.numChannels = 2;
    input_bus.channelBuffers32 = input_channels.data();

    Steinberg::Vst::AudioBusBuffers output_bus{};
    output_bus.numChannels = 2;
    output_bus.channelBuffers32 = output_channels.data();

    Steinberg::Vst::ProcessData data{};
    data.processMode = Steinberg::Vst::kRealtime;
    data.symbolicSampleSize = Steinberg::Vst::kSample32;
    data.numSamples = static_cast<Steinberg::int32>(Frames);
    data.numInputs = 1;
    data.numOutputs = 1;
    data.inputs = &input_bus;
    data.outputs = &output_bus;
    data.inputEvents = &events;
    data.inputParameterChanges = &changes;

    const auto result = processor.process(data);
    (void)processor.setActive(false);
    return result == Steinberg::kResultOk;
}

struct ParityMetrics {
    double max_abs_diff{0.0};
    double rms_diff{0.0};
    std::size_t worst_frame{0U};
    bool within_tolerance{true};
};

template <std::size_t Frames>
ParityMetrics compareStereo(
    const std::array<float, Frames>& core_left,
    const std::array<float, Frames>& core_right,
    const std::array<float, Frames>& vst_left,
    const std::array<float, Frames>& vst_right) {
    ParityMetrics metrics{};
    double sum_squares = 0.0;
    std::size_t count = 0U;

    for (std::size_t frame = 0U; frame < Frames; ++frame) {
        const std::array<std::pair<float, float>, 2U> pairs{{
            {core_left[frame], vst_left[frame]},
            {core_right[frame], vst_right[frame]},
        }};
        for (const auto& [a, b] : pairs) {
            if (!std::isfinite(a) || !std::isfinite(b)) {
                metrics.within_tolerance = false;
                continue;
            }
            const auto diff =
                std::abs(static_cast<double>(a) - static_cast<double>(b));
            const auto tolerance =
                static_cast<double>(kAbsoluteTolerance) +
                static_cast<double>(kRelativeTolerance) *
                    std::max(std::abs(static_cast<double>(a)),
                             std::abs(static_cast<double>(b)));
            if (diff > metrics.max_abs_diff) {
                metrics.max_abs_diff = diff;
                metrics.worst_frame = frame;
            }
            if (diff > tolerance) {
                metrics.within_tolerance = false;
            }
            sum_squares += diff * diff;
            ++count;
        }
    }

    if (count != 0U) {
        metrics.rms_diff =
            std::sqrt(sum_squares / static_cast<double>(count));
    }
    return metrics;
}

template <std::size_t Frames>
bool exactlyZeroBefore(const std::array<float, Frames>& samples,
                       std::size_t frame) {
    return std::all_of(
        samples.begin(),
        samples.begin() + static_cast<std::ptrdiff_t>(frame),
        [](float sample) { return sample == 0.0F; });
}

void testReferenceSequenceParity() {
    constexpr std::size_t kFrames = 256U;
    constexpr Steinberg::int32 kPitch = 64;
    constexpr float kTuning = 13.5F;
    constexpr float kVelocity = 0.72F;
    constexpr Steinberg::int32 kNoteId = 17;
    constexpr std::uint32_t kNoteOnOffset = 19U;
    constexpr std::uint32_t kPitchAutomationOffset = 57U;
    constexpr std::uint32_t kExternalStart = 96U;
    constexpr std::uint32_t kTimbreAutomationOffset = 127U;
    constexpr std::uint32_t kNoteOffOffset = 191U;

    std::array<float, kFrames> input_left{};
    std::array<float, kFrames> input_right{};
    fillExternalInput(input_left, input_right, kExternalStart);

    resonant::FixedEventBuffer<resonant::kMaxEventsPerBlock> direct_events;
    check(pushPortableNoteOn(
              direct_events, kNoteOnOffset, kPitch, kTuning,
              kVelocity, kNoteId),
          "build independent portable note-on sequence");
    check(pushPortableParameter(
              direct_events, kPitchAutomationOffset,
              resonant::BreathPipeVoice::kPitchHz, 0.90),
          "build independent portable pitch automation");
    check(pushPortableParameter(
              direct_events, kExternalStart - 1U,
              resonant::BreathPipeVoice::kExternalAmount, 0.72),
          "build independent portable external-amount automation");
    check(pushPortableParameter(
              direct_events, kTimbreAutomationOffset,
              resonant::BreathPipeVoice::kTimbre, 0.81),
          "build independent portable timbre automation");
    check(pushPortableNoteOff(
              direct_events, kNoteOffOffset, 0.20F, kNoteId),
          "build independent portable note-off sequence");

    FixedHostEventList<2U> host_events;
    check(host_events.push(hostNoteOn(
              static_cast<Steinberg::int32>(kNoteOnOffset),
              kPitch, kTuning, kVelocity, kNoteId)),
          "build host note-on sequence");
    check(host_events.push(hostNoteOff(
              static_cast<Steinberg::int32>(kNoteOffOffset),
              kPitch, 0.20F, kNoteId)),
          "build host note-off sequence");

    Steinberg::Vst::ParameterChanges host_changes{3};
    addHostParameterPoint(
        host_changes,
        static_cast<Steinberg::Vst::ParamID>(
            resonant::BreathPipeVoice::kPitchHz),
        static_cast<Steinberg::int32>(kPitchAutomationOffset),
        0.90);
    addHostParameterPoint(
        host_changes,
        static_cast<Steinberg::Vst::ParamID>(
            resonant::BreathPipeVoice::kExternalAmount),
        static_cast<Steinberg::int32>(kExternalStart - 1U),
        0.72);
    addHostParameterPoint(
        host_changes,
        static_cast<Steinberg::Vst::ParamID>(
            resonant::BreathPipeVoice::kTimbre),
        static_cast<Steinberg::int32>(kTimbreAutomationOffset),
        0.81);

    std::array<float, kFrames> core_left{};
    std::array<float, kFrames> core_right{};
    std::array<float, kFrames> vst_left{};
    std::array<float, kFrames> vst_right{};

    check(renderDirect(
              input_left, input_right, direct_events,
              core_left, core_right),
          "render independent direct-core reference sequence");
    check(renderVst3(
              input_left, input_right, host_events, host_changes,
              vst_left, vst_right),
          "render VST3 reference sequence");

    const auto metrics =
        compareStereo(core_left, core_right, vst_left, vst_right);
    check(metrics.within_tolerance,
          "direct-core and VST3 reference sequence stay within frozen tolerance");

    check(exactlyZeroBefore(core_left, kNoteOnOffset) &&
              exactlyZeroBefore(vst_left, kNoteOnOffset),
          "both paths remain exactly silent before note-on sample");
    check(std::abs(core_left[kNoteOnOffset]) > 1.0e-12F &&
              std::abs(vst_left[kNoteOnOffset]) > 1.0e-12F,
          "note-on becomes audible on the exact requested sample");

    std::cout << "M4.9 PARITY reference max_abs_diff="
              << metrics.max_abs_diff
              << " rms_diff=" << metrics.rms_diff
              << " worst_frame=" << metrics.worst_frame
              << " abs_tolerance=" << kAbsoluteTolerance
              << " rel_tolerance=" << kRelativeTolerance << '\n';

    FixedHostEventList<2U> shifted_events;
    check(shifted_events.push(hostNoteOn(
              static_cast<Steinberg::int32>(kNoteOnOffset),
              kPitch, kTuning, kVelocity, kNoteId)) &&
              shifted_events.push(hostNoteOff(
                  static_cast<Steinberg::int32>(kNoteOffOffset),
                  kPitch, 0.20F, kNoteId)),
          "build shifted-control host events");

    Steinberg::Vst::ParameterChanges shifted_changes{3};
    addHostParameterPoint(
        shifted_changes,
        static_cast<Steinberg::Vst::ParamID>(
            resonant::BreathPipeVoice::kPitchHz),
        static_cast<Steinberg::int32>(kPitchAutomationOffset + 1U),
        0.90);
    addHostParameterPoint(
        shifted_changes,
        static_cast<Steinberg::Vst::ParamID>(
            resonant::BreathPipeVoice::kExternalAmount),
        static_cast<Steinberg::int32>(kExternalStart - 1U),
        0.72);
    addHostParameterPoint(
        shifted_changes,
        static_cast<Steinberg::Vst::ParamID>(
            resonant::BreathPipeVoice::kTimbre),
        static_cast<Steinberg::int32>(kTimbreAutomationOffset),
        0.81);

    std::array<float, kFrames> shifted_left{};
    std::array<float, kFrames> shifted_right{};
    check(renderVst3(
              input_left, input_right, shifted_events, shifted_changes,
              shifted_left, shifted_right),
          "render one-sample-shifted automation control");

    const auto shifted_metrics =
        compareStereo(core_left, core_right, shifted_left, shifted_right);
    check(!shifted_metrics.within_tolerance,
          "frozen comparator detects one-sample automation timing drift");

    std::cout << "M4.9 TIMING shifted_automation_by=1"
              << " max_abs_diff=" << shifted_metrics.max_abs_diff
              << " rms_diff=" << shifted_metrics.rms_diff
              << " detected="
              << (!shifted_metrics.within_tolerance ? 1 : 0) << '\n';
}

void testChunkBoundaryParity() {
    constexpr std::size_t kFrames = 5'000U;
    constexpr Steinberg::int32 kPitch = 67;
    constexpr float kTuning = -5.0F;
    constexpr float kVelocity = 0.61F;
    constexpr Steinberg::int32 kNoteId = 3;
    constexpr std::uint32_t kNoteOnOffset = 4'095U;
    constexpr std::uint32_t kAutomationOffset = 4'096U;
    constexpr std::uint32_t kNoteOffOffset = 4'988U;

    std::array<float, kFrames> input_left{};
    std::array<float, kFrames> input_right{};
    fillExternalInput(input_left, input_right, kAutomationOffset);

    resonant::FixedEventBuffer<resonant::kMaxEventsPerBlock> direct_events;
    check(pushPortableNoteOn(
              direct_events, kNoteOnOffset, kPitch, kTuning,
              kVelocity, kNoteId),
          "build chunk-boundary portable note-on");
    check(pushPortableParameter(
              direct_events, kAutomationOffset,
              resonant::BreathPipeVoice::kPitchHz, 0.78),
          "build chunk-boundary portable automation");
    check(pushPortableParameter(
              direct_events, kAutomationOffset,
              resonant::BreathPipeVoice::kExternalAmount, 0.86),
          "build chunk-boundary external automation");
    check(pushPortableNoteOff(
              direct_events, kNoteOffOffset, 0.15F, kNoteId),
          "build chunk-boundary portable note-off");

    FixedHostEventList<2U> host_events;
    check(host_events.push(hostNoteOn(
              static_cast<Steinberg::int32>(kNoteOnOffset),
              kPitch, kTuning, kVelocity, kNoteId)) &&
              host_events.push(hostNoteOff(
                  static_cast<Steinberg::int32>(kNoteOffOffset),
                  kPitch, 0.15F, kNoteId)),
          "build chunk-boundary host events");

    Steinberg::Vst::ParameterChanges host_changes{2};
    addHostParameterPoint(
        host_changes,
        static_cast<Steinberg::Vst::ParamID>(
            resonant::BreathPipeVoice::kPitchHz),
        static_cast<Steinberg::int32>(kAutomationOffset),
        0.78);
    addHostParameterPoint(
        host_changes,
        static_cast<Steinberg::Vst::ParamID>(
            resonant::BreathPipeVoice::kExternalAmount),
        static_cast<Steinberg::int32>(kAutomationOffset),
        0.86);

    std::array<float, kFrames> core_left{};
    std::array<float, kFrames> core_right{};
    std::array<float, kFrames> vst_left{};
    std::array<float, kFrames> vst_right{};

    check(renderDirect(
              input_left, input_right, direct_events,
              core_left, core_right),
          "render direct core across 4096-frame chunk boundary");
    check(renderVst3(
              input_left, input_right, host_events, host_changes,
              vst_left, vst_right),
          "render VST3 across 4096-frame chunk boundary");

    const auto metrics =
        compareStereo(core_left, core_right, vst_left, vst_right);
    check(metrics.within_tolerance,
          "chunk-boundary direct-core/VST3 parity stays within frozen tolerance");

    check(exactlyZeroBefore(core_left, kNoteOnOffset) &&
              exactlyZeroBefore(vst_left, kNoteOnOffset),
          "chunk-boundary paths remain silent through frame 4094");
    check(std::abs(core_left[kNoteOnOffset]) > 1.0e-12F &&
              std::abs(vst_left[kNoteOnOffset]) > 1.0e-12F,
          "frame 4095 note-on is not delayed into the second chunk");

    std::cout << "M4.9 PARITY chunk_boundary max_abs_diff="
              << metrics.max_abs_diff
              << " rms_diff=" << metrics.rms_diff
              << " worst_frame=" << metrics.worst_frame
              << " abs_tolerance=" << kAbsoluteTolerance
              << " rel_tolerance=" << kRelativeTolerance << '\n';
}

} // namespace

int main() {
    testReferenceSequenceParity();
    testChunkBoundaryParity();

    if (failures != 0) {
        std::cerr << failures << " M4.9 native-core/VST3 parity test(s) failed\n";
        return 1;
    }

    std::cout << "PASS: M4.9 native-core/VST3 parity\n";
    return 0;
}
