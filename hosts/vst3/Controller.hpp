#pragma once

#include "hosts/vst3/ParameterMapping.hpp"
#include "public.sdk/source/vst/vsteditcontroller.h"
#include "public.sdk/source/vst/vstparameters.h"

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
            const auto tag = static_cast<Steinberg::Vst::ParamID>(
                HostParameterMapping::toHostId(spec.id));

            auto* parameter = new Steinberg::Vst::RangeParameter(
                title,
                tag,
                spec.unit.empty() ? nullptr : units,
                static_cast<Steinberg::Vst::ParamValue>(spec.minimum),
                static_cast<Steinberg::Vst::ParamValue>(spec.maximum),
                static_cast<Steinberg::Vst::ParamValue>(spec.default_value),
                0,
                flags);
            if (parameters.addParameter(parameter) == nullptr) {
                parameter->release();
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
            destination[index] = static_cast<Steinberg::Vst::TChar>(
                static_cast<unsigned char>(source[index]));
        }
        destination[count] = 0;
    }
};

} // namespace resonant::vst3
