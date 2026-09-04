#pragma once

#include "pluginterfaces/base/funknown.h"

namespace resonant::vst3 {

inline const Steinberg::FUID kProcessorUid{
    0x6F0C9B31, 0x7C4D4DA0, 0xA3C39866, 0xF4B95A11};
inline const Steinberg::FUID kControllerUid{
    0xC9E1332D, 0x8F5B4C76, 0xB1476A32, 0x317D20F2};

inline constexpr auto kPluginName = "Resonant Engine Breath Pipe";
inline constexpr auto kControllerName = "Resonant Engine Breath Pipe Controller";
inline constexpr auto kPluginVersion = "0.0.1";

} // namespace resonant::vst3
