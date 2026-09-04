#pragma once

#include "public.sdk/source/vst/vsteditcontroller.h"

namespace resonant::vst3 {

class Controller final : public Steinberg::Vst::EditController {
public:
    static Steinberg::FUnknown* createInstance(void*) {
        return static_cast<Steinberg::Vst::IEditController*>(new Controller{});
    }

    Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) override {
        return EditController::initialize(context);
    }
};

} // namespace resonant::vst3
