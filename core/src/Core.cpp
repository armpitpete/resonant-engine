#include "resonant/BreathPipe.hpp"
#include "resonant/Engine.hpp"
#include "resonant/Energy.hpp"
#include "resonant/Event.hpp"
#include "resonant/Feedback.hpp"
#include "resonant/FirstResonator.hpp"
#include "resonant/Interfaces.hpp"
#include "resonant/Parameter.hpp"
#include "resonant/Random.hpp"
#include "resonant/ReferenceFeedbackProbe.hpp"
#include "resonant/Types.hpp"

namespace resonant {
static_assert(sizeof(Sample) == 4, "M0 requires float32 audio samples");
static_assert(EngineModel<FirstResonatorVoice>);
static_assert(EngineModel<BreathPipeVoice>);
} // namespace resonant
