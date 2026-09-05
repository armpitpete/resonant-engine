#pragma once

#include "hosts/vst3/CoreAdapter.hpp"
#include "hosts/vst3/EventTranslator.hpp"
#include "public.sdk/source/vst/vstaudioeffect.h"

#include <array>

namespace resonant::vst3 {

class Processor final : public Steinberg::Vst::AudioEffect {
public:
    Processor();

    static Steinberg::FUnknown* createInstance(void*) {
        return static_cast<Steinberg::Vst::IAudioProcessor*>(new Processor{});
    }

    Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) override;
    Steinberg::tresult PLUGIN_API setupProcessing(
        Steinberg::Vst::ProcessSetup& setup) override;
    Steinberg::tresult PLUGIN_API setActive(Steinberg::TBool state) override;
    Steinberg::tresult PLUGIN_API process(
        Steinberg::Vst::ProcessData& data) override;
    Steinberg::tresult PLUGIN_API canProcessSampleSize(
        Steinberg::int32 symbolic_sample_size) override;

private:
    [[nodiscard]] bool translateEvents(Steinberg::Vst::ProcessData& data,
                                       std::uint32_t total_frames) noexcept;
    [[nodiscard]] bool translateParameters(Steinberg::Vst::ProcessData& data,
                                           std::uint32_t total_frames) noexcept;
    [[nodiscard]] bool stageFlushParameters(
        Steinberg::Vst::ProcessData& data) noexcept;
    [[nodiscard]] bool appendPendingParameters() noexcept;
    void clearPendingParameters() noexcept;

    BreathPipeCoreAdapter adapter_{};
    HostEventTranslator event_translator_{};
    FixedEventBuffer<kMaxEventsPerBlock> chunk_events_{};
    std::array<Sample, BreathPipeVoice::kParameterSpecs.size()>
        pending_parameter_values_{};
    std::array<bool, BreathPipeVoice::kParameterSpecs.size()>
        pending_parameter_set_{};
};

} // namespace resonant::vst3
