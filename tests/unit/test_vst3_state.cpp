#include "hosts/vst3/Controller.hpp"
#include "hosts/vst3/Processor.hpp"

#include "public.sdk/source/vst/hosting/parameterchanges.h"
#include "pluginterfaces/vst/ivstevents.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <span>
#include <string_view>

namespace {

int failures = 0;

void check(bool condition, std::string_view name) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << name << '\n';
    }
}

class ChunkedStream final : public Steinberg::IBStream {
public:
    explicit ChunkedStream(std::size_t max_chunk = 256U) noexcept
        : max_chunk_(max_chunk) {}

    void load(std::span<const std::byte> bytes) noexcept {
        size_ = std::min(bytes.size(), storage_.size());
        std::copy_n(bytes.begin(), size_, storage_.begin());
        position_ = 0U;
    }

    void rewind() noexcept { position_ = 0U; }

    [[nodiscard]] std::span<const std::byte> bytes() const noexcept {
        return {storage_.data(), size_};
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
        const auto count = std::min({available, requested, max_chunk_});
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
        void* buffer,
        Steinberg::int32 num_bytes,
        Steinberg::int32* num_bytes_written) override {
        if (num_bytes_written != nullptr) {
            *num_bytes_written = 0;
        }
        if (buffer == nullptr || num_bytes < 0 ||
            position_ >= storage_.size()) {
            return Steinberg::kResultFalse;
        }

        const auto requested = static_cast<std::size_t>(num_bytes);
        const auto available = storage_.size() - position_;
        const auto count = std::min({available, requested, max_chunk_});
        if (count == 0U) {
            return Steinberg::kResultFalse;
        }
        std::memcpy(storage_.data() + position_, buffer, count);
        position_ += count;
        size_ = std::max(size_, position_);
        if (num_bytes_written != nullptr) {
            *num_bytes_written = static_cast<Steinberg::int32>(count);
        }
        return Steinberg::kResultOk;
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
        if (next < 0 ||
            next > static_cast<Steinberg::int64>(size_)) {
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
    std::size_t max_chunk_{256U};
};

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

resonant::BreathPipeState nonDefaultState() {
    auto state = resonant::defaultBreathPipeState();
    state.parameters[0].value = 440.0F;
    state.parameters[1].value = 0.69F;
    state.parameters[2].value = 0.38F;
    state.parameters[3].value = 0.63F;
    state.parameters[4].value = 0.27F;
    state.parameters[5].value = 0.84F;
    state.parameters[6].value = 0.42F;
    state.parameters[7].value = 0.51F;
    state.parameters[8].value = 0.33F;
    state.parameters[9].value = 0.77F;
    return state;
}

bool prepare(resonant::vst3::Processor& processor) {
    Steinberg::Vst::ProcessSetup setup{};
    setup.processMode = Steinberg::Vst::kRealtime;
    setup.symbolicSampleSize = Steinberg::Vst::kSample32;
    setup.maxSamplesPerBlock = 128;
    setup.sampleRate = 48'000.0;
    return processor.setupProcessing(setup) == Steinberg::kResultOk;
}

void loadEncoded(ChunkedStream& stream,
                 const resonant::BreathPipeState& state) {
    std::array<std::byte, resonant::BreathPipeStateCodec::kEncodedSize> bytes{};
    check(resonant::BreathPipeStateCodec::encode(state, bytes),
          "encode portable state into test stream");
    stream.load(bytes);
}

bool readProcessorState(resonant::vst3::Processor& processor,
                        resonant::BreathPipeState& state,
                        std::size_t chunk = 256U) {
    ChunkedStream output{chunk};
    if (processor.getState(&output) != Steinberg::kResultOk) {
        return false;
    }
    return resonant::BreathPipeStateCodec::decode(output.bytes(), state);
}

Steinberg::tresult processBlock(
    resonant::vst3::Processor& processor,
    std::array<float, 128U>& left,
    std::array<float, 128U>& right,
    Steinberg::Vst::IParameterChanges* changes = nullptr,
    Steinberg::Vst::IEventList* events = nullptr) {
    std::array<float*, 2U> channels{{left.data(), right.data()}};
    Steinberg::Vst::AudioBusBuffers output{};
    output.numChannels = 2;
    output.channelBuffers32 = channels.data();

    Steinberg::Vst::ProcessData data{};
    data.processMode = Steinberg::Vst::kRealtime;
    data.symbolicSampleSize = Steinberg::Vst::kSample32;
    data.numSamples = 128;
    data.numOutputs = 1;
    data.outputs = &output;
    data.inputParameterChanges = changes;
    data.inputEvents = events;
    return processor.process(data);
}

void testPreSetupLoadAndPartialStreams() {
    const auto desired = nonDefaultState();
    ChunkedStream input{7U};
    loadEncoded(input, desired);

    resonant::vst3::Processor processor;
    check(processor.setState(&input) == Steinberg::kResultOk,
          "component accepts state before setupProcessing");
    check(prepare(processor), "prepare processor after pre-setup state load");

    resonant::BreathPipeState recalled{};
    check(readProcessorState(processor, recalled, 5U),
          "getState supports partial stream writes");
    check(resonant::validBreathPipeState(recalled),
          "recalled processor state is portable and valid");
    for (std::size_t index = 0U; index < desired.parameters.size(); ++index) {
        check(std::bit_cast<std::uint32_t>(desired.parameters[index].value) ==
                  std::bit_cast<std::uint32_t>(recalled.parameters[index].value),
              "pre-setup state survives setupProcessing");
    }
}

void testAlterRestoreActivationAndPendingFlush() {
    resonant::vst3::Processor processor;
    check(prepare(processor), "prepare processor for alter/restore");

    const auto saved = nonDefaultState();
    ChunkedStream saved_stream{11U};
    loadEncoded(saved_stream, saved);
    check(processor.setState(&saved_stream) == Steinberg::kResultOk,
          "load saved non-default state");

    Steinberg::Vst::Event note{};
    note.busIndex = 0;
    note.sampleOffset = 0;
    note.type = Steinberg::Vst::Event::kNoteOnEvent;
    note.noteOn.channel = 0;
    note.noteOn.pitch = 60;
    note.noteOn.tuning = 0.0F;
    note.noteOn.velocity = 0.2F;
    note.noteOn.noteId = 17;
    SingleEventList note_events{note};

    std::array<float, 128U> note_left{};
    std::array<float, 128U> note_right{};
    check(processBlock(processor, note_left, note_right, nullptr, &note_events) ==
              Steinberg::kResultOk,
          "process note performance over saved parameter state");

    resonant::BreathPipeState after_note{};
    check(readProcessorState(processor, after_note),
          "capture state after transient note performance");
    check(std::abs(after_note.parameters[0].value - saved.parameters[0].value) <
              1.0e-7F &&
              std::abs(after_note.parameters[1].value - saved.parameters[1].value) <
              1.0e-7F,
          "note pitch and velocity do not overwrite persistent project state");

    Steinberg::Vst::ParameterChanges changes{1};
    Steinberg::int32 queue_index = 0;
    auto* queue = changes.addParameterData(
        resonant::BreathPipeVoice::kPressure, queue_index);
    Steinberg::int32 point_index = 0;
    check(queue != nullptr &&
              queue->addPoint(0, 0.1, point_index) == Steinberg::kResultOk,
          "build automation that alters saved pressure");

    std::array<float, 128U> left{};
    std::array<float, 128U> right{};
    check(processBlock(processor, left, right, &changes) == Steinberg::kResultOk,
          "process altered parameter state");

    resonant::BreathPipeState altered{};
    check(readProcessorState(processor, altered), "capture altered processor state");
    check(std::abs(altered.parameters[1].value - 0.1F) < 1.0e-6F,
          "getState observes canonical core automation target");

    saved_stream.rewind();
    check(processor.setState(&saved_stream) == Steinberg::kResultOk,
          "restore saved state after alteration");

    resonant::BreathPipeState restored{};
    check(readProcessorState(processor, restored), "capture restored processor state");
    check(std::abs(restored.parameters[1].value - saved.parameters[1].value) <
              1.0e-7F,
          "save alter restore returns saved pressure");

    check(processor.setActive(true) == Steinberg::kResultOk &&
              processor.setActive(false) == Steinberg::kResultOk,
          "activate/deactivate lifecycle succeeds after recall");
    resonant::BreathPipeState after_activation{};
    check(readProcessorState(processor, after_activation),
          "capture state after activate/deactivate");
    check(std::abs(after_activation.parameters[1].value -
                   saved.parameters[1].value) < 1.0e-7F,
          "activate/deactivate reset preserves portable parameters");

    Steinberg::Vst::ParameterChanges flush_changes{1};
    Steinberg::int32 flush_queue_index = 0;
    auto* flush_queue = flush_changes.addParameterData(
        resonant::BreathPipeVoice::kPressure, flush_queue_index);
    Steinberg::int32 flush_point_index = 0;
    check(flush_queue != nullptr &&
              flush_queue->addPoint(0, 0.9, flush_point_index) ==
                  Steinberg::kResultOk,
          "build pending zero-sample pressure flush");

    Steinberg::Vst::ProcessData flush{};
    flush.processMode = Steinberg::Vst::kRealtime;
    flush.symbolicSampleSize = Steinberg::Vst::kSample32;
    flush.numSamples = 0;
    flush.inputParameterChanges = &flush_changes;
    check(processor.process(flush) == Steinberg::kResultOk,
          "stage zero-sample pressure before state save");

    resonant::BreathPipeState with_pending{};
    check(readProcessorState(processor, with_pending),
          "getState captures pending zero-sample parameter state");
    check(std::abs(with_pending.parameters[1].value - 0.9F) < 1.0e-6F,
          "pending flush is included without fake audio processing");
}

void testMalformedStreamsAreTransactional() {
    resonant::vst3::Processor processor;
    check(prepare(processor), "prepare processor for malformed state");

    const auto baseline = nonDefaultState();
    ChunkedStream valid_stream{};
    loadEncoded(valid_stream, baseline);
    check(processor.setState(&valid_stream) == Steinberg::kResultOk,
          "load baseline before malformed state");

    check(processor.setState(nullptr) == Steinberg::kResultFalse,
          "null setState stream rejected");
    check(processor.getState(nullptr) == Steinberg::kResultFalse,
          "null getState stream rejected");

    ChunkedStream short_stream{3U};
    std::array<std::byte, 10U> short_bytes{};
    short_stream.load(short_bytes);
    check(processor.setState(&short_stream) == Steinberg::kResultFalse,
          "short state stream rejected");

    std::array<std::byte,
               resonant::BreathPipeStateCodec::kEncodedSize + 1U> trailing{};
    std::array<std::byte,
               resonant::BreathPipeStateCodec::kEncodedSize> baseline_bytes{};
    check(resonant::BreathPipeStateCodec::encode(baseline, baseline_bytes),
          "encode baseline for trailing-byte stream test");
    std::copy(baseline_bytes.begin(), baseline_bytes.end(), trailing.begin());
    trailing.back() = std::byte{0x7fU};
    ChunkedStream trailing_stream{5U};
    trailing_stream.load(trailing);
    check(processor.setState(&trailing_stream) == Steinberg::kResultFalse,
          "component state with trailing bytes rejected");

    ChunkedStream malformed{};
    loadEncoded(malformed, baseline);
    auto malformed_bytes =
        std::array<std::byte, resonant::BreathPipeStateCodec::kEncodedSize>{};
    std::copy(malformed.bytes().begin(), malformed.bytes().end(),
              malformed_bytes.begin());
    malformed_bytes[0] = std::byte{0U};
    ChunkedStream bad_magic{4U};
    bad_magic.load(malformed_bytes);
    check(processor.setState(&bad_magic) == Steinberg::kResultFalse,
          "malformed component state rejected");

    resonant::BreathPipeState after_failure{};
    check(readProcessorState(processor, after_failure),
          "capture state after malformed load");
    check(std::abs(after_failure.parameters[1].value -
                   baseline.parameters[1].value) < 1.0e-7F &&
              std::abs(after_failure.parameters[9].value -
                       baseline.parameters[9].value) < 1.0e-7F,
          "malformed setState leaves previous live state unchanged");
}

void testDeterministicProcessorRecall() {
    const auto state = nonDefaultState();

    resonant::vst3::Processor a;
    resonant::vst3::Processor b;
    ChunkedStream a_state{9U};
    ChunkedStream b_state{13U};
    loadEncoded(a_state, state);
    loadEncoded(b_state, state);
    check(a.setState(&a_state) == Steinberg::kResultOk &&
              b.setState(&b_state) == Steinberg::kResultOk,
          "load identical state into fresh processors");
    check(prepare(a) && prepare(b), "prepare identically restored processors");

    std::array<float, 128U> left_a{};
    std::array<float, 128U> right_a{};
    std::array<float, 128U> left_b{};
    std::array<float, 128U> right_b{};

    for (std::size_t block = 0U; block < 8U; ++block) {
        check(processBlock(a, left_a, right_a) == Steinberg::kResultOk &&
                  processBlock(b, left_b, right_b) == Steinberg::kResultOk,
              "restored processors render finite blocks");
        for (std::size_t frame = 0U; frame < left_a.size(); ++frame) {
            check(std::bit_cast<std::uint32_t>(left_a[frame]) ==
                      std::bit_cast<std::uint32_t>(left_b[frame]) &&
                  std::bit_cast<std::uint32_t>(right_a[frame]) ==
                      std::bit_cast<std::uint32_t>(right_b[frame]),
                  "identical processor state renders bit-identically");
        }
    }
}

void testControllerComponentStateSynchronization() {
    resonant::vst3::Controller controller;
    check(controller.initialize(nullptr) == Steinberg::kResultOk,
          "initialize controller for component-state sync");

    const auto state = nonDefaultState();
    ChunkedStream stream{6U};
    loadEncoded(stream, state);
    check(controller.setComponentState(&stream) == Steinberg::kResultOk,
          "controller accepts same portable component state");

    for (const auto& entry : state.parameters) {
        const auto host_id =
            resonant::vst3::HostParameterMapping::toHostId(entry.id);
        const auto expected =
            resonant::vst3::HostParameterMapping::nativeToNormalized(
                host_id, entry.value);
        const auto actual = controller.getParamNormalized(
            static_cast<Steinberg::Vst::ParamID>(host_id));
        check(std::abs(actual - expected) < 1.0e-12,
              "controller parameter synchronized from portable component state");
    }

    check(controller.setComponentState(nullptr) == Steinberg::kResultFalse,
          "controller rejects null component state");
    (void)controller.terminate();
}

} // namespace

int main() {
    testPreSetupLoadAndPartialStreams();
    testAlterRestoreActivationAndPendingFlush();
    testMalformedStreamsAreTransactional();
    testDeterministicProcessorRecall();
    testControllerComponentStateSynchronization();

    if (failures != 0) {
        std::cerr << failures << " M4.6 VST3 state recall test(s) failed\n";
        return 1;
    }

    std::cout << "PASS: M4.6 VST3 portable state recall\n";
    return 0;
}
