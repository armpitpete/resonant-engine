#include "resonant/BreathPipeState.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
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

using Encoded = std::array<std::byte, resonant::BreathPipeStateCodec::kEncodedSize>;

void writeU32(Encoded& bytes, std::size_t offset, std::uint32_t value) {
    for (std::size_t byte = 0U; byte < 4U; ++byte) {
        bytes[offset + byte] = static_cast<std::byte>(
            static_cast<std::uint8_t>((value >> (8U * byte)) & 0xffU));
    }
}

resonant::BreathPipeState nonDefaultState() {
    auto state = resonant::defaultBreathPipeState();
    state.seed = 0x123456789abcdef0ULL;
    state.parameters[0].value = 330.0F;
    state.parameters[1].value = 0.72F;
    state.parameters[2].value = 0.61F;
    state.parameters[3].value = 0.43F;
    state.parameters[4].value = 0.31F;
    state.parameters[5].value = 0.74F;
    state.parameters[6].value = 0.81F;
    state.parameters[7].value = 0.57F;
    state.parameters[8].value = 0.26F;
    state.parameters[9].value = 0.66F;
    return state;
}

bool equalState(const resonant::BreathPipeState& a,
                const resonant::BreathPipeState& b) {
    if (a.version != b.version || a.seed != b.seed) {
        return false;
    }
    for (std::size_t index = 0U; index < a.parameters.size(); ++index) {
        if (a.parameters[index].id != b.parameters[index].id ||
            std::bit_cast<std::uint32_t>(a.parameters[index].value) !=
                std::bit_cast<std::uint32_t>(b.parameters[index].value)) {
            return false;
        }
    }
    return true;
}

void testRoundTripAndDeterministicBytes() {
    const auto state = nonDefaultState();
    Encoded first{};
    Encoded second{};
    check(resonant::BreathPipeStateCodec::encode(state, first),
          "encode non-default portable state");
    check(resonant::BreathPipeStateCodec::encode(state, second),
          "encode identical portable state again");
    check(first == second, "identical state produces identical bytes");

    resonant::BreathPipeState decoded{};
    check(resonant::BreathPipeStateCodec::decode(first, decoded),
          "decode portable state");
    check(equalState(state, decoded), "portable state round-trip is exact");

    for (std::size_t index = 0U;
         index < resonant::BreathPipeVoice::kParameterSpecs.size(); ++index) {
        check(decoded.parameters[index].id ==
                  resonant::BreathPipeVoice::kParameterSpecs[index].id,
              "serialized state covers canonical ParameterId");
    }
}

void testMalformedStateIsRejectedTransactionally() {
    const auto valid = nonDefaultState();
    Encoded bytes{};
    check(resonant::BreathPipeStateCodec::encode(valid, bytes),
          "prepare encoded state for malformed tests");

    auto sentinel = resonant::defaultBreathPipeState();
    sentinel.parameters[1].value = 0.123F;
    const auto original_sentinel = sentinel;

    check(!resonant::BreathPipeStateCodec::decode(
              std::span<const std::byte>{bytes.data(), bytes.size() - 1U},
              sentinel),
          "truncated state rejected");
    check(equalState(sentinel, original_sentinel),
          "truncated state does not partially mutate destination");

    std::array<std::byte, resonant::BreathPipeStateCodec::kEncodedSize + 1U>
        extra{};
    std::copy(bytes.begin(), bytes.end(), extra.begin());
    check(!resonant::BreathPipeStateCodec::decode(extra, sentinel),
          "trailing state bytes rejected");

    auto bad_magic = bytes;
    bad_magic[0] = std::byte{0U};
    check(!resonant::BreathPipeStateCodec::decode(bad_magic, sentinel),
          "invalid state magic rejected");

    auto bad_version = bytes;
    bad_version[4] = std::byte{2U};
    bad_version[5] = std::byte{0U};
    check(!resonant::BreathPipeStateCodec::decode(bad_version, sentinel),
          "unsupported state version rejected");

    auto bad_count = bytes;
    bad_count[6] = std::byte{9U};
    bad_count[7] = std::byte{0U};
    check(!resonant::BreathPipeStateCodec::decode(bad_count, sentinel),
          "invalid parameter count rejected");

    auto unknown_id = bytes;
    writeU32(unknown_id, resonant::BreathPipeStateCodec::kHeaderSize, 9999U);
    check(!resonant::BreathPipeStateCodec::decode(unknown_id, sentinel),
          "unknown ParameterId rejected");

    auto duplicate_id = bytes;
    const auto second_entry =
        resonant::BreathPipeStateCodec::kHeaderSize +
        resonant::BreathPipeStateCodec::kEntrySize;
    writeU32(duplicate_id, second_entry,
             resonant::BreathPipeVoice::kPitchHz);
    check(!resonant::BreathPipeStateCodec::decode(duplicate_id, sentinel),
          "duplicate/reordered ParameterId rejected");

    auto nan_value = bytes;
    writeU32(nan_value,
             resonant::BreathPipeStateCodec::kHeaderSize + 4U,
             0x7fc00000U);
    check(!resonant::BreathPipeStateCodec::decode(nan_value, sentinel),
          "NaN parameter state rejected");

    auto inf_value = bytes;
    writeU32(inf_value,
             resonant::BreathPipeStateCodec::kHeaderSize + 4U,
             0x7f800000U);
    check(!resonant::BreathPipeStateCodec::decode(inf_value, sentinel),
          "infinite parameter state rejected");

    auto out_of_range = bytes;
    writeU32(out_of_range,
             resonant::BreathPipeStateCodec::kHeaderSize + 4U,
             std::bit_cast<std::uint32_t>(5000.0F));
    check(!resonant::BreathPipeStateCodec::decode(out_of_range, sentinel),
          "out-of-range parameter state rejected");

    check(equalState(sentinel, original_sentinel),
          "all malformed decodes preserve previous destination state");
}

void testCaptureAlterRestoreAndFailureAtomicity() {
    resonant::BreathPipeVoice voice{777U};
    const resonant::ProcessSpec spec{48'000.0, 128U, 0U, 2U};
    check(voice.prepare(spec), "prepare Breath Pipe for state recall");

    const auto desired = nonDefaultState();
    check(resonant::restoreBreathPipeState(voice, desired),
          "restore non-default portable state");

    resonant::BreathPipeState captured{};
    check(resonant::captureBreathPipeState(voice, captured),
          "capture restored portable state");
    check(equalState(captured, desired),
          "capture returns restored persistent model state");
    check(voice.seed() == desired.seed,
          "state restore applies deterministic voice seed");

    voice.handleEvent({0U, resonant::EventType::Pitch,
                       0U, 0U, 880.0F, 0.0F});
    voice.handleEvent({0U, resonant::EventType::Pressure,
                       0U, 0U, 0.05F, 0.0F});
    voice.handleEvent({0U, resonant::EventType::PerNoteExpression,
                       0U, 0U, 0.02F, 0.0F});
    resonant::BreathPipeState after_expression{};
    check(resonant::captureBreathPipeState(voice, after_expression),
          "capture state after transient performance expression");
    check(equalState(after_expression, desired),
          "pitch pressure and per-note expression do not alter persistent state");

    check(resonant::restoreBreathPipeState(voice, captured),
          "restore after transient performance expression");

    voice.handleEvent({0U, resonant::EventType::ParameterChange,
                       resonant::BreathPipeVoice::kPressure, 0U, 0.1F, 0.0F});
    voice.handleEvent({0U, resonant::EventType::ParameterChange,
                       resonant::BreathPipeVoice::kTimbre, 0U, 0.05F, 0.0F});

    check(resonant::restoreBreathPipeState(voice, captured),
          "altered voice restores saved state");

    resonant::BreathPipeState restored{};
    check(resonant::captureBreathPipeState(voice, restored),
          "capture after alter-and-restore");
    check(equalState(restored, captured),
          "save alter restore returns exact parameter state");
    check(std::abs(voice.currentPressure() - captured.parameters[1].value) <
              1.0e-7F,
          "state restore is hard lifecycle recall, not prior-state smoothing");

    auto invalid = captured;
    invalid.parameters[1].value =
        std::numeric_limits<resonant::Sample>::quiet_NaN();
    check(!resonant::restoreBreathPipeState(voice, invalid),
          "invalid state application rejected");

    resonant::BreathPipeState after_failure{};
    check(resonant::captureBreathPipeState(voice, after_failure),
          "capture after failed state application");
    check(equalState(after_failure, captured),
          "failed state application leaves live state unchanged");
}

void testDeterministicRenderAfterRestore() {
    resonant::BreathPipeVoice voice{17U};
    const resonant::ProcessSpec spec{48'000.0, 128U, 0U, 1U};
    check(voice.prepare(spec), "prepare deterministic recall voice");

    const auto state = nonDefaultState();
    auto render_pass = [&](const resonant::BreathPipeState& recalled) {
        std::array<std::uint32_t, 1024U> bits{};
        check(resonant::restoreBreathPipeState(voice, recalled),
              "restore state before deterministic render");
        voice.handleEvent(
            {0U, resonant::EventType::NoteOn, 0U, 1U, 0.8F, 0.0F});
        std::array<resonant::Sample, 1U> output{};
        for (std::size_t frame = 0U; frame < bits.size(); ++frame) {
            check(voice.processSample({}, output),
                  "recalled deterministic voice remains finite");
            bits[frame] = std::bit_cast<std::uint32_t>(output[0]);
        }
        return bits;
    };

    // This is the actual recall contract: after arbitrary prior history,
    // loading the same persistent model state must reset transient DSP/RNG
    // progress and reproduce the same trajectory. It intentionally avoids
    // comparing two separately inlined direct processSample call sites,
    // because legal Release FP contraction can give those call sites
    // sub-ULP differences that a feedback resonator later amplifies.
    const auto first = render_pass(state);
    voice.handleEvent({0U, resonant::EventType::ParameterChange,
                       resonant::BreathPipeVoice::kPressure, 0U, 0.1F, 0.0F});
    std::array<resonant::Sample, 1U> disturbed{};
    for (std::size_t frame = 0U; frame < 257U; ++frame) {
        check(voice.processSample({}, disturbed),
              "intervening state remains finite");
    }
    const auto second = render_pass(state);
    check(first == second,
          "save reload reproduces bit-identical deterministic trajectory");

    auto different_seed = state;
    ++different_seed.seed;
    const auto third = render_pass(different_seed);
    check(first != third,
          "serialized seed controls the deterministic turbulence trajectory");
}

} // namespace

int main() {
    testRoundTripAndDeterministicBytes();
    testMalformedStateIsRejectedTransactionally();
    testCaptureAlterRestoreAndFailureAtomicity();
    testDeterministicRenderAfterRestore();

    if (failures != 0) {
        std::cerr << failures << " M4.6 portable state test(s) failed\n";
        return 1;
    }

    std::cout << "PASS: M4.6 portable Breath Pipe state recall\n";
    return 0;
}
