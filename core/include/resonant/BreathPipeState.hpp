#pragma once

#include "resonant/BreathPipe.hpp"

#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>

namespace resonant {

struct BreathPipeStateEntry {
    ParameterId id{0U};
    Sample value{0.0F};
};

struct BreathPipeState {
    static constexpr std::uint16_t kCurrentVersion = 1U;

    std::uint16_t version{kCurrentVersion};
    Seed seed{kDefaultSeed};
    std::array<BreathPipeStateEntry, BreathPipeVoice::kParameterSpecs.size()>
        parameters{};
};

[[nodiscard]] constexpr BreathPipeState defaultBreathPipeState() noexcept {
    BreathPipeState state{};
    for (std::size_t index = 0U;
         index < BreathPipeVoice::kParameterSpecs.size(); ++index) {
        state.parameters[index] = {
            BreathPipeVoice::kParameterSpecs[index].id,
            BreathPipeVoice::kParameterSpecs[index].default_value,
        };
    }
    return state;
}

[[nodiscard]] inline bool validBreathPipeState(
    const BreathPipeState& state) noexcept {
    if (state.version != BreathPipeState::kCurrentVersion) {
        return false;
    }
    for (std::size_t index = 0U; index < state.parameters.size(); ++index) {
        const auto& spec = BreathPipeVoice::kParameterSpecs[index];
        const auto& entry = state.parameters[index];
        if (entry.id != spec.id ||
            !std::isfinite(entry.value) ||
            entry.value < spec.minimum ||
            entry.value > spec.maximum) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] inline bool captureBreathPipeState(
    const BreathPipeVoice& voice,
    BreathPipeState& state) noexcept {
    auto captured = defaultBreathPipeState();
    captured.seed = voice.seed();
    for (std::size_t index = 0U; index < captured.parameters.size(); ++index) {
        Sample value = 0.0F;
        const auto id = BreathPipeVoice::kParameterSpecs[index].id;
        if (!voice.parameterTarget(id, value) || !std::isfinite(value)) {
            return false;
        }
        captured.parameters[index] = {id, value};
    }
    if (!validBreathPipeState(captured)) {
        return false;
    }
    state = captured;
    return true;
}

[[nodiscard]] inline bool restoreBreathPipeState(
    BreathPipeVoice& voice,
    const BreathPipeState& state) noexcept {
    if (!validBreathPipeState(state)) {
        return false;
    }

    std::array<Sample, BreathPipeVoice::kParameterSpecs.size()> values{};
    for (std::size_t index = 0U; index < state.parameters.size(); ++index) {
        values[index] = state.parameters[index].value;
    }
    return voice.restorePersistentState(state.seed, values);
}

class BreathPipeStateCodec {
public:
    static constexpr std::array<std::byte, 4U> kMagic{
        std::byte{0x52U}, std::byte{0x45U},
        std::byte{0x42U}, std::byte{0x50U}};
    static constexpr std::size_t kHeaderSize = 16U;
    static constexpr std::size_t kEntrySize = 8U;
    static constexpr std::size_t kEncodedSize =
        kHeaderSize + BreathPipeVoice::kParameterSpecs.size() * kEntrySize;

    [[nodiscard]] static bool encode(
        const BreathPipeState& state,
        std::span<std::byte> destination) noexcept {
        static_assert(sizeof(Sample) == sizeof(std::uint32_t));
        static_assert(std::numeric_limits<Sample>::is_iec559);

        if (destination.size() != kEncodedSize ||
            !validBreathPipeState(state)) {
            return false;
        }

        destination[0] = kMagic[0];
        destination[1] = kMagic[1];
        destination[2] = kMagic[2];
        destination[3] = kMagic[3];
        writeU16(destination, 4U, state.version);
        writeU16(destination, 6U,
                 static_cast<std::uint16_t>(state.parameters.size()));
        writeU64(destination, 8U, state.seed);

        std::size_t offset = kHeaderSize;
        for (const auto& entry : state.parameters) {
            writeU32(destination, offset, entry.id);
            writeU32(destination, offset + 4U,
                     std::bit_cast<std::uint32_t>(entry.value));
            offset += kEntrySize;
        }
        return true;
    }

    [[nodiscard]] static bool decode(
        std::span<const std::byte> source,
        BreathPipeState& state) noexcept {
        static_assert(sizeof(Sample) == sizeof(std::uint32_t));
        static_assert(std::numeric_limits<Sample>::is_iec559);

        if (source.size() != kEncodedSize ||
            source[0] != kMagic[0] ||
            source[1] != kMagic[1] ||
            source[2] != kMagic[2] ||
            source[3] != kMagic[3]) {
            return false;
        }

        BreathPipeState decoded{};
        decoded.version = readU16(source, 4U);
        if (decoded.version != BreathPipeState::kCurrentVersion ||
            readU16(source, 6U) !=
                static_cast<std::uint16_t>(decoded.parameters.size())) {
            return false;
        }
        decoded.seed = readU64(source, 8U);

        std::size_t offset = kHeaderSize;
        for (std::size_t index = 0U;
             index < decoded.parameters.size(); ++index) {
            const auto id = readU32(source, offset);
            const auto bits = readU32(source, offset + 4U);
            const auto value = std::bit_cast<Sample>(bits);
            const auto& spec = BreathPipeVoice::kParameterSpecs[index];

            // The on-wire order is canonical. This simultaneously rejects
            // unknown IDs, duplicate IDs and reordered fields.
            if (id != spec.id ||
                !std::isfinite(value) ||
                value < spec.minimum ||
                value > spec.maximum) {
                return false;
            }

            decoded.parameters[index] = {id, value};
            offset += kEntrySize;
        }

        if (!validBreathPipeState(decoded)) {
            return false;
        }
        state = decoded;
        return true;
    }

private:
    static void writeU16(std::span<std::byte> bytes,
                         std::size_t offset,
                         std::uint16_t value) noexcept {
        bytes[offset] =
            static_cast<std::byte>(static_cast<std::uint8_t>(value & 0xffU));
        bytes[offset + 1U] = static_cast<std::byte>(
            static_cast<std::uint8_t>((value >> 8U) & 0xffU));
    }

    static void writeU32(std::span<std::byte> bytes,
                         std::size_t offset,
                         std::uint32_t value) noexcept {
        for (std::size_t byte = 0U; byte < 4U; ++byte) {
            bytes[offset + byte] = static_cast<std::byte>(
                static_cast<std::uint8_t>((value >> (8U * byte)) & 0xffU));
        }
    }

    static void writeU64(std::span<std::byte> bytes,
                         std::size_t offset,
                         std::uint64_t value) noexcept {
        for (std::size_t byte = 0U; byte < 8U; ++byte) {
            bytes[offset + byte] = static_cast<std::byte>(
                static_cast<std::uint8_t>((value >> (8U * byte)) & 0xffU));
        }
    }

    [[nodiscard]] static std::uint16_t readU16(
        std::span<const std::byte> bytes,
        std::size_t offset) noexcept {
        return static_cast<std::uint16_t>(
            static_cast<std::uint16_t>(std::to_integer<std::uint8_t>(bytes[offset])) |
            (static_cast<std::uint16_t>(
                 std::to_integer<std::uint8_t>(bytes[offset + 1U])) << 8U));
    }

    [[nodiscard]] static std::uint32_t readU32(
        std::span<const std::byte> bytes,
        std::size_t offset) noexcept {
        std::uint32_t value = 0U;
        for (std::size_t byte = 0U; byte < 4U; ++byte) {
            value |= static_cast<std::uint32_t>(
                         std::to_integer<std::uint8_t>(bytes[offset + byte]))
                     << (8U * byte);
        }
        return value;
    }

    [[nodiscard]] static std::uint64_t readU64(
        std::span<const std::byte> bytes,
        std::size_t offset) noexcept {
        std::uint64_t value = 0U;
        for (std::size_t byte = 0U; byte < 8U; ++byte) {
            value |= static_cast<std::uint64_t>(
                         std::to_integer<std::uint8_t>(bytes[offset + byte]))
                     << (8U * byte);
        }
        return value;
    }
};

static_assert(BreathPipeStateCodec::kEncodedSize == 96U);

} // namespace resonant
