#pragma once

#include "resonant/BreathPipeState.hpp"

#include "pluginterfaces/base/ibstream.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace resonant::vst3 {

class PortableStateStreamAdapter {
public:
    using EncodedState =
        std::array<std::byte, BreathPipeStateCodec::kEncodedSize>;

    [[nodiscard]] static bool write(Steinberg::IBStream* stream,
                                    const BreathPipeState& state) noexcept {
        if (stream == nullptr) {
            return false;
        }

        EncodedState bytes{};
        if (!BreathPipeStateCodec::encode(state, bytes)) {
            return false;
        }

        std::size_t offset = 0U;
        while (offset < bytes.size()) {
            Steinberg::int32 written = 0;
            const auto remaining = static_cast<Steinberg::int32>(
                bytes.size() - offset);
            if (stream->write(bytes.data() + offset,
                              remaining,
                              &written) != Steinberg::kResultOk ||
                written <= 0 ||
                written > remaining) {
                return false;
            }
            offset += static_cast<std::size_t>(written);
        }
        return true;
    }

    [[nodiscard]] static bool read(Steinberg::IBStream* stream,
                                   BreathPipeState& state) noexcept {
        if (stream == nullptr) {
            return false;
        }

        EncodedState bytes{};
        std::size_t offset = 0U;
        while (offset < bytes.size()) {
            Steinberg::int32 read = 0;
            const auto remaining = static_cast<Steinberg::int32>(
                bytes.size() - offset);
            if (stream->read(bytes.data() + offset,
                             remaining,
                             &read) != Steinberg::kResultOk ||
                read <= 0 ||
                read > remaining) {
                return false;
            }
            offset += static_cast<std::size_t>(read);
        }

        std::byte trailing{};
        Steinberg::int32 trailing_read = 0;
        (void)stream->read(&trailing, 1, &trailing_read);
        if (trailing_read != 0) {
            return false;
        }

        BreathPipeState decoded{};
        if (!BreathPipeStateCodec::decode(bytes, decoded)) {
            return false;
        }
        state = decoded;
        return true;
    }
};

} // namespace resonant::vst3
