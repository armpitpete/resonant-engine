#include "hosts/vst3/Processor.hpp"

#include "public.sdk/source/vst/hosting/parameterchanges.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <string_view>

namespace {

int failures = 0;

constexpr Steinberg::int32 kFrames = 128;
constexpr double kSampleRate = 48'000.0;

void check(bool condition, std::string_view name) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << name << '\n';
    }
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

Steinberg::Vst::Event noteOn(Steinberg::int32 offset,
                             Steinberg::int32 note_id = 17) {
    Steinberg::Vst::Event event{};
    event.busIndex = 0;
    event.sampleOffset = offset;
    event.type = Steinberg::Vst::Event::kNoteOnEvent;
    event.noteOn.channel = 0;
    event.noteOn.pitch = 64;
    event.noteOn.tuning = 0.0F;
    event.noteOn.velocity = 0.72F;
    event.noteOn.length = 0;
    event.noteOn.noteId = note_id;
    return event;
}

Steinberg::Vst::Event noteOff(Steinberg::int32 offset,
                              Steinberg::int32 note_id = 17) {
    Steinberg::Vst::Event event{};
    event.busIndex = 0;
    event.sampleOffset = offset;
    event.type = Steinberg::Vst::Event::kNoteOffEvent;
    event.noteOff.channel = 0;
    event.noteOff.pitch = 64;
    event.noteOff.tuning = 0.0F;
    event.noteOff.velocity = 0.0F;
    event.noteOff.noteId = note_id;
    return event;
}

bool setup(resonant::vst3::Processor& processor,
           Steinberg::int32 process_mode) {
    if (processor.initialize(nullptr) != Steinberg::kResultOk) {
        return false;
    }

    Steinberg::Vst::ProcessSetup setup{};
    setup.processMode = process_mode;
    setup.symbolicSampleSize = Steinberg::Vst::kSample32;
    setup.maxSamplesPerBlock = kFrames;
    setup.sampleRate = kSampleRate;
    return processor.setupProcessing(setup) == Steinberg::kResultOk;
}

struct ProcessOutcome {
    Steinberg::tresult result{Steinberg::kResultFalse};
    Steinberg::Vst::SpeakerArrangement silence_flags{0};
};

ProcessOutcome processBlock(
    resonant::vst3::Processor& processor,
    Steinberg::int32 process_mode,
    std::array<float, static_cast<std::size_t>(kFrames)>& left,
    std::array<float, static_cast<std::size_t>(kFrames)>& right,
    Steinberg::Vst::IEventList* events = nullptr,
    Steinberg::Vst::IParameterChanges* changes = nullptr) {
    std::array<float*, 2U> channels{{left.data(), right.data()}};
    Steinberg::Vst::AudioBusBuffers output{};
    output.numChannels = 2;
    output.channelBuffers32 = channels.data();

    Steinberg::Vst::ProcessData data{};
    data.processMode = process_mode;
    data.symbolicSampleSize = Steinberg::Vst::kSample32;
    data.numSamples = kFrames;
    data.numInputs = 0;
    data.numOutputs = 1;
    data.outputs = &output;
    data.inputEvents = events;
    data.inputParameterChanges = changes;

    const auto result = processor.process(data);
    return {result, output.silenceFlags};
}

template <std::size_t N>
bool allFinite(const std::array<float, N>& samples) {
    return std::all_of(samples.begin(), samples.end(),
                       [](float sample) { return std::isfinite(sample); });
}

template <std::size_t N>
bool allZero(const std::array<float, N>& samples) {
    return std::all_of(samples.begin(), samples.end(),
                       [](float sample) { return sample == 0.0F; });
}

template <std::size_t N>
double energy(const std::array<float, N>& samples) {
    double total = 0.0;
    for (const auto sample : samples) {
        total += std::abs(static_cast<double>(sample));
    }
    return total;
}

void testOfflineProcessing() {
    resonant::vst3::Processor processor;
    check(setup(processor, Steinberg::Vst::kOffline),
          "setup processor for offline processing");
    check(processor.setActive(true) == Steinberg::kResultOk,
          "activate processor for offline processing");

    FixedEventList<1U> events;
    check(events.push(noteOn(0)), "build offline note-on event");

    std::array<float, static_cast<std::size_t>(kFrames)> left{};
    std::array<float, static_cast<std::size_t>(kFrames)> right{};
    const auto outcome = processBlock(
        processor, Steinberg::Vst::kOffline, left, right, &events);

    const bool finite = allFinite(left) && allFinite(right);
    const bool audible = energy(left) > 1.0e-10;
    check(outcome.result == Steinberg::kResultOk,
          "offline ProcessData is accepted");
    check(finite, "offline render remains finite");
    check(audible, "offline render produces Breath Pipe audio");

    std::cout << "M4.10 OFFLINE process_mode=offline finite="
              << (finite ? 1 : 0)
              << " audible=" << (audible ? 1 : 0) << '\n';

    check(processor.setActive(false) == Steinberg::kResultOk,
          "deactivate processor after offline processing");
    check(processor.terminate() == Steinberg::kResultOk,
          "terminate processor after offline processing");
}

void testRepeatedLifecycleReset() {
    resonant::vst3::Processor processor;
    check(setup(processor, Steinberg::Vst::kRealtime),
          "setup processor for repeated lifecycle");

    constexpr std::size_t kCycles = 8U;
    bool reset_silence = true;
    bool note_audio = true;

    for (std::size_t cycle = 0U; cycle < kCycles; ++cycle) {
        check(processor.setActive(true) == Steinberg::kResultOk,
              "activate processor in lifecycle cycle");

        std::array<float, static_cast<std::size_t>(kFrames)> silent_left{};
        std::array<float, static_cast<std::size_t>(kFrames)> silent_right{};
        const auto silent = processBlock(
            processor, Steinberg::Vst::kRealtime, silent_left, silent_right);
        reset_silence = reset_silence &&
                        silent.result == Steinberg::kResultOk &&
                        allZero(silent_left) && allZero(silent_right);

        FixedEventList<1U> events;
        check(events.push(noteOn(0, static_cast<Steinberg::int32>(cycle + 1U))),
              "build lifecycle note-on event");
        std::array<float, static_cast<std::size_t>(kFrames)> note_left{};
        std::array<float, static_cast<std::size_t>(kFrames)> note_right{};
        const auto sounded = processBlock(
            processor, Steinberg::Vst::kRealtime,
            note_left, note_right, &events);
        note_audio = note_audio &&
                     sounded.result == Steinberg::kResultOk &&
                     allFinite(note_left) && allFinite(note_right) &&
                     energy(note_left) > 1.0e-10;

        check(processor.setActive(false) == Steinberg::kResultOk,
              "deactivate processor in lifecycle cycle");
    }

    check(reset_silence,
          "each reactivate begins from exact reset silence");
    check(note_audio,
          "each lifecycle cycle can render a fresh finite note");

    std::cout << "M4.10 LIFECYCLE cycles=" << kCycles
              << " reset_silence=" << (reset_silence ? 1 : 0)
              << " fresh_note_audio=" << (note_audio ? 1 : 0) << '\n';

    check(processor.terminate() == Steinberg::kResultOk,
          "terminate processor after repeated lifecycle");
}

void testAutomationExtremesAndMalformedRecovery() {
    resonant::vst3::Processor processor;
    check(setup(processor, Steinberg::Vst::kRealtime),
          "setup processor for automation conformance");
    check(processor.setActive(true) == Steinberg::kResultOk,
          "activate processor for automation conformance");

    FixedEventList<1U> on_events;
    check(on_events.push(noteOn(0)), "build automation-test note-on");
    std::array<float, static_cast<std::size_t>(kFrames)> on_left{};
    std::array<float, static_cast<std::size_t>(kFrames)> on_right{};
    check(processBlock(processor, Steinberg::Vst::kRealtime,
                       on_left, on_right, &on_events).result ==
              Steinberg::kResultOk,
          "establish active note before hostile automation");

    Steinberg::Vst::ParameterChanges malformed{1};
    Steinberg::int32 malformed_queue_index = 0;
    auto* malformed_queue = malformed.addParameterData(
        static_cast<Steinberg::Vst::ParamID>(
            resonant::BreathPipeVoice::kPressure),
        malformed_queue_index);
    Steinberg::int32 malformed_point_index = 0;
    check(malformed_queue != nullptr &&
              malformed_queue->addPoint(
                  kFrames, 0.5, malformed_point_index) ==
                  Steinberg::kResultOk,
          "construct out-of-block automation point");

    std::array<float, static_cast<std::size_t>(kFrames)> bad_left{};
    std::array<float, static_cast<std::size_t>(kFrames)> bad_right{};
    bad_left.fill(0.5F);
    bad_right.fill(0.5F);
    const auto bad = processBlock(
        processor, Steinberg::Vst::kRealtime,
        bad_left, bad_right, nullptr, &malformed);
    const bool malformed_fail_closed =
        bad.result == Steinberg::kResultFalse &&
        allZero(bad_left) && allZero(bad_right) &&
        bad.silence_flags != 0;
    check(malformed_fail_closed,
          "malformed automation fails closed and clears output");

    Steinberg::Vst::ParameterChanges malformed_value{1};
    Steinberg::int32 malformed_value_queue_index = 0;
    auto* malformed_value_queue = malformed_value.addParameterData(
        static_cast<Steinberg::Vst::ParamID>(
            resonant::BreathPipeVoice::kPressure),
        malformed_value_queue_index);
    Steinberg::int32 malformed_value_point_index = 0;
    check(malformed_value_queue != nullptr &&
              malformed_value_queue->addPoint(
                  0, 1.25, malformed_value_point_index) ==
                  Steinberg::kResultOk,
          "construct out-of-range normalized automation value");

    std::array<float, static_cast<std::size_t>(kFrames)> bad_value_left{};
    std::array<float, static_cast<std::size_t>(kFrames)> bad_value_right{};
    bad_value_left.fill(0.5F);
    bad_value_right.fill(0.5F);
    const auto bad_value = processBlock(
        processor, Steinberg::Vst::kRealtime,
        bad_value_left, bad_value_right, nullptr, &malformed_value);
    const bool malformed_value_fail_closed =
        bad_value.result == Steinberg::kResultFalse &&
        allZero(bad_value_left) && allZero(bad_value_right) &&
        bad_value.silence_flags != 0;
    check(malformed_value_fail_closed,
          "out-of-range automation value fails closed and clears output");

    Steinberg::Vst::ParameterChanges extremes{1};
    Steinberg::int32 extreme_queue_index = 0;
    auto* extreme_queue = extremes.addParameterData(
        static_cast<Steinberg::Vst::ParamID>(
            resonant::BreathPipeVoice::kPressure),
        extreme_queue_index);
    Steinberg::int32 first_index = 0;
    Steinberg::int32 second_index = 0;
    check(extreme_queue != nullptr &&
              extreme_queue->addPoint(0, 1.0, first_index) ==
                  Steinberg::kResultOk &&
              extreme_queue->addPoint(kFrames - 1, 0.0, second_index) ==
                  Steinberg::kResultOk,
          "construct valid endpoint automation");

    std::array<float, static_cast<std::size_t>(kFrames)> extreme_left{};
    std::array<float, static_cast<std::size_t>(kFrames)> extreme_right{};
    const auto extreme = processBlock(
        processor, Steinberg::Vst::kRealtime,
        extreme_left, extreme_right, nullptr, &extremes);
    const bool extreme_finite =
        extreme.result == Steinberg::kResultOk &&
        allFinite(extreme_left) && allFinite(extreme_right);
    check(extreme_finite,
          "valid normalized endpoint automation remains finite");

    FixedEventList<1U> off_events;
    check(off_events.push(noteOff(0)),
          "build recovery note-off after hostile automation");
    std::array<float, static_cast<std::size_t>(kFrames)> off_left{};
    std::array<float, static_cast<std::size_t>(kFrames)> off_right{};
    check(processBlock(processor, Steinberg::Vst::kRealtime,
                       off_left, off_right, &off_events).result ==
              Steinberg::kResultOk,
          "valid note-off remains accepted after malformed automation");

    check(processor.setActive(false) == Steinberg::kResultOk &&
              processor.setActive(true) == Steinberg::kResultOk,
          "ordinary host lifecycle reset remains available after hostile automation");

    std::array<float, static_cast<std::size_t>(kFrames)> recovered_left{};
    std::array<float, static_cast<std::size_t>(kFrames)> recovered_right{};
    const auto recovered = processBlock(
        processor, Steinberg::Vst::kRealtime,
        recovered_left, recovered_right);
    const bool recovery_silence =
        recovered.result == Steinberg::kResultOk &&
        allZero(recovered_left) && allZero(recovered_right);
    check(recovery_silence,
          "hostile automation cannot leave a stuck voice after lifecycle reset");

    std::cout << "M4.10 AUTOMATION malformed_fail_closed="
              << (malformed_fail_closed ? 1 : 0)
              << " extreme_finite=" << (extreme_finite ? 1 : 0)
              << " recovered_silence=" << (recovery_silence ? 1 : 0)
              << '\n';
    std::cout << "M4.10 AUTOMATION_VALUE malformed_value_fail_closed="
              << (malformed_value_fail_closed ? 1 : 0) << '\n';

    check(processor.setActive(false) == Steinberg::kResultOk,
          "deactivate processor after hostile automation recovery");
    check(processor.terminate() == Steinberg::kResultOk,
          "terminate processor after hostile automation recovery");
}

} // namespace

int main() {
    testOfflineProcessing();
    testRepeatedLifecycleReset();
    testAutomationExtremesAndMalformedRecovery();

    if (failures != 0) {
        std::cerr << failures << " M4.10 VST3 conformance test(s) failed\n";
        return 1;
    }

    std::cout << "PASS: M4.10 VST3 conformance\n";
    return 0;
}
