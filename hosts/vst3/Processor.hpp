#pragma once

#include "hosts/vst3/CoreAdapter.hpp"
#include "hosts/vst3/EventTranslator.hpp"
#include "hosts/vst3/StateAdapter.hpp"
#include "public.sdk/source/vst/vstaudioeffect.h"

#include <array>
#include <atomic>
#include <cstdint>

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
    Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream* state) override;
    Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream* state) override;
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
    [[nodiscard]] bool capturePortableState(BreathPipeState& state) noexcept;
    [[nodiscard]] bool capturePortableStateUnlocked(
        BreathPipeState& state) const noexcept;
    void storePortableState(const BreathPipeState& state) noexcept;
    [[nodiscard]] bool queuePortableState(const BreathPipeState& state) noexcept;
    [[nodiscard]] bool applyQueuedPortableState() noexcept;
    [[nodiscard]] bool publishAdapterState() noexcept;
    void publishPendingParameterState() noexcept;
    [[nodiscard]] bool stateRequestPending() const noexcept;
    [[nodiscard]] bool markCurrentStateRequestApplied() noexcept;
    void clearPendingParameters() noexcept;

    static_assert(std::atomic<std::uint32_t>::is_always_lock_free,
                  "VST3 state handoff requires lock-free 32-bit atomics");

    BreathPipeCoreAdapter adapter_{};
    HostEventTranslator event_translator_{};
    FixedEventBuffer<kMaxEventsPerBlock> chunk_events_{};
    std::array<Sample, BreathPipeVoice::kParameterSpecs.size()>
        pending_parameter_values_{};
    std::array<bool, BreathPipeVoice::kParameterSpecs.size()>
        pending_parameter_set_{};

    // Steinberg permits component state calls while realtime processing is
    // active. The UI thread may wait briefly for this always-lock-free writer
    // token; the audio thread only attempts it once and never spins.
    std::atomic_flag state_writer_guard_ = ATOMIC_FLAG_INIT;
    std::atomic<std::uint32_t> state_request_sequence_{0U};
    std::uint32_t applied_state_request_sequence_{0U};
    std::atomic<std::uint32_t> state_seed_low_{0U};
    std::atomic<std::uint32_t> state_seed_high_{0U};
    std::array<std::atomic<std::uint32_t>,
               BreathPipeVoice::kParameterSpecs.size()>
        state_parameter_bits_{};
};

} // namespace resonant::vst3
