#include "hosts/vst3/Controller.hpp"
#include "hosts/vst3/PluginIds.hpp"
#include "hosts/vst3/Processor.hpp"

#include "public.sdk/source/main/pluginfactory.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"

using namespace Steinberg;
using namespace Steinberg::Vst;

BEGIN_FACTORY_DEF(
    "Merrin World",
    "https://merrinworld.uk",
    "mailto:merrin@merrinworld.uk")

DEF_CLASS2(
    INLINE_UID_FROM_FUID(resonant::vst3::kProcessorUid),
    PClassInfo::kManyInstances,
    kVstAudioEffectClass,
    resonant::vst3::kPluginName,
    Vst::kDistributable,
    "Instrument|Synth",
    resonant::vst3::kPluginVersion,
    kVstVersionString,
    resonant::vst3::Processor::createInstance)

DEF_CLASS2(
    INLINE_UID_FROM_FUID(resonant::vst3::kControllerUid),
    PClassInfo::kManyInstances,
    kVstComponentControllerClass,
    resonant::vst3::kControllerName,
    0,
    "",
    resonant::vst3::kPluginVersion,
    kVstVersionString,
    resonant::vst3::Controller::createInstance)

END_FACTORY
