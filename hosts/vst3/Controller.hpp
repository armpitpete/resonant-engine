#pragma once

#include "hosts/vst3/ParameterMapping.hpp"
#include "public.sdk/source/vst/vsteditcontroller.h"

#include <algorithm>
#include <cstddef>
#include <string_view>

namespace resonant::vst3 {

class Controller final : public Steinberg::Vst::EditController {
public:
    static Steinberg::FUnknown* createInstance(void*) {
        return static_cast<Steinberg::Vst::IEditController*>(new Controller{});
    }

    Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) override {
        const auto result = EditController::initialize(context);
        if (result != Steinberg::kResultOk) {
            return result;
        }

        for (const auto& spec : BreathPipeVoice::parameterSpecs()) {
            if (!HostParameterMapping::exposed(spec)) {
                continue;
            }

            Steinberg::Vst::String128 title{};
            Steinberg::Vst::String128 units{};
            copyAscii(title, spec.name);
            copyAscii(units, spec.unit);

            const auto flags = Steinberg::Vst::ParameterInfo::kCanAutomate;
            const auto default_normalized =
                static_cast<Steinberg::Vst::ParamValue>(spec.normalize(spec.default_value));
            const auto tag = static_cast<Steinberg::int32>(
                HostParameterMapping::toHostId(spec.id));

            if (parameters.addParameter(
                    title,
                    spec.unit.empty() ? nullptr : units,
                    0,
                    default_normalized,
                    flags,
                    tag) == nullptr) {
                return Steinberg::kResultFalse;
            }
        }

        return Steinberg::kResultOk;
    }

private:
    static void copyAscii(Steinberg::Vst::String128& destination,
                          std::string_view source) noexcept {
        constexpr std::size_t capacity = 128U;
        const auto count = std::min(source.size(), capacity - 1U);
        for (std::size_t index = 0U; index < count; ++index) {
            destination[index] = static_cast<Steinberg::TChar>(
                static_cast<unsigned char>(source[index]));
        }
        destination[count] = 0;
    }
};

} // namespace resonant::vst3
