#include "hosts/vst3/EventTranslator.hpp"
#include "hosts/vst3/ParameterMapping.hpp"

#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string_view>

namespace {

int failures = 0;

void check(bool condition, std::string_view name) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << name << '\n';
    }
}

void testCanonicalMetadataAndIds() {
    using resonant::BreathPipeVoice;
    using resonant::vst3::HostParameterMapping;

    check(HostParameterMapping::exposedCount() ==
              BreathPipeVoice::kParameterSpecs.size(),
          "all Breath Pipe portable parameters are host-visible");

    for (const auto& spec : BreathPipeVoice::parameterSpecs()) {
        check(spec.valid(), "portable Breath Pipe parameter metadata is valid");
        check(HostParameterMapping::exposed(spec),
              "portable Breath Pipe parameter exposed");
        check(HostParameterMapping::toHostId(spec.id) == spec.id,
              "ParamID is stable direct projection of ParameterId");
        check(HostParameterMapping::specForHostId(spec.id) == &spec,
              "host ParamID resolves canonical core metadata");

        const auto normalized_default =
            HostParameterMapping::nativeToNormalized(spec.id, spec.default_value);
        resonant::Sample round_trip = 0.0F;
        check(HostParameterMapping::normalizedToNative(
                  spec.id, normalized_default, round_trip),
              "default normalized value converts to native");
        check(std::abs(round_trip - spec.default_value) < 1.0e-5F,
              "normalized/native round trip preserves default");
    }

    check(HostParameterMapping::specForHostId(0U) == nullptr,
          "reserved ParamID zero is not exposed");
    resonant::Sample native = 0.0F;
    check(!HostParameterMapping::normalizedToNative(0U, 0.5, native),
          "unknown ParamID is rejected");
    check(!HostParameterMapping::normalizedToNative(
              BreathPipeVoice::kTurbulence,
              std::numeric_limits<double>::quiet_NaN(),
              native),
          "non-finite normalized automation is rejected");
    check(!HostParameterMapping::normalizedToNative(
              BreathPipeVoice::kTurbulence, -0.01, native),
          "normalized automation below zero is rejected");
    check(!HostParameterMapping::normalizedToNative(
              BreathPipeVoice::kTurbulence, 1.01, native),
          "normalized automation above one is rejected");
}

void testVisibilityPolicy() {
    using resonant::ParameterKind;
    using resonant::ParameterSpec;
    using resonant::SmoothingMode;
    using resonant::vst3::HostParameterMapping;

    constexpr ParameterSpec hidden{
        700U, "Hidden", "", 0.0F, 1.0F, 0.5F,
        ParameterKind::Continuous, SmoothingMode::None, 0.0, false};
    constexpr ParameterSpec internal{
        701U, "Internal", "", 0.0F, 1.0F, 0.5F,
        ParameterKind::Internal, SmoothingMode::None, 0.0, true};
    constexpr ParameterSpec topology{
        702U, "Topology", "", 0.0F, 1.0F, 0.5F,
        ParameterKind::Topology, SmoothingMode::None, 0.0, true};

    check(!HostParameterMapping::exposed(hidden),
          "host_visible false remains hidden");
    check(!HostParameterMapping::exposed(internal),
          "internal parameter cannot leak into VST3");
    check(!HostParameterMapping::exposed(topology),
          "topology parameter cannot leak into VST3");
}

void testSampleAccurateParameterEvents() {
    using resonant::BreathPipeVoice;
    using resonant::EventType;
    using resonant::vst3::HostEventTranslator;
    using resonant::vst3::HostParameterMapping;

    HostEventTranslator translator;
    translator.beginBlock(64U);

    resonant::Sample turbulence = 0.0F;
    resonant::Sample regeneration = 0.0F;
    check(HostParameterMapping::normalizedToNative(
              BreathPipeVoice::kTurbulence, 0.75, turbulence),
          "turbulence normalized point maps to native");
    check(HostParameterMapping::normalizedToNative(
              BreathPipeVoice::kRegeneration, 1.0, regeneration),
          "regeneration normalized point maps to native range");

    check(translator.parameterChange(
              40U, BreathPipeVoice::kRegeneration, regeneration),
          "later parameter point accepted");
    check(translator.parameterChange(
              8U, BreathPipeVoice::kTurbulence, turbulence),
          "earlier point from another queue accepted");

    const auto events = translator.events();
    check(events.size() == 2U, "two automation points emitted");
    check(events[0].sample_offset == 8U &&
              events[0].type == EventType::ParameterChange &&
              events[0].target == BreathPipeVoice::kTurbulence &&
              std::abs(events[0].value - 0.75F) < 1.0e-6F,
          "automation event keeps exact early sample and native value");
    check(events[1].sample_offset == 40U &&
              events[1].target == BreathPipeVoice::kRegeneration &&
              std::abs(events[1].value - 1.5F) < 1.0e-6F,
          "automation event keeps exact later sample and scaled native value");

    translator.beginBlock(64U);
    check(translator.noteOn(16U, 69, 0.0F, 0.5F, 2),
          "note event accepted");
    check(translator.parameterChange(
              16U, BreathPipeVoice::kTimbre, 0.9F),
          "same-sample automation accepted");
    const auto same_sample = translator.events();
    check(same_sample.size() == 4U,
          "note expansion and automation share bounded event buffer");
    check(same_sample[0].type == EventType::Pitch &&
              same_sample[1].type == EventType::Pressure &&
              same_sample[2].type == EventType::ParameterChange &&
              same_sample[3].type == EventType::NoteOn,
          "portable event priority gives deterministic same-sample automation order");
}

void testMalformedAndOverflowPolicy() {
    resonant::vst3::HostEventTranslator translator;
    translator.beginBlock(32U);

    check(translator.noteOn(4U, 60, 0.0F, 0.6F, 77),
          "note state can change before automation rejection");
    check(!translator.parameterChange(32U, 203U, 0.5F),
          "out-of-range automation offset rejected");
    check(!translator.valid(), "bad automation invalidates block");
    translator.abortBlock();
    check(!translator.hasActiveNote(),
          "rejected automation block rolls note identity back");

    translator.beginBlock(32U);
    check(!translator.parameterChange(
              0U, 203U, std::numeric_limits<float>::infinity()),
          "non-finite native automation rejected");
    translator.abortBlock();

    translator.beginBlock(4096U);
    bool overflow = false;
    for (std::uint32_t index = 0U;
         index <= resonant::kMaxEventsPerBlock; ++index) {
        if (!translator.parameterChange(
                index % 4096U,
                resonant::BreathPipeVoice::kTurbulence,
                0.5F)) {
            overflow = translator.overflowed();
            break;
        }
    }
    check(overflow, "automation translation is fixed-capacity and fails closed");
}

} // namespace

int main() {
    testCanonicalMetadataAndIds();
    testVisibilityPolicy();
    testSampleAccurateParameterEvents();
    testMalformedAndOverflowPolicy();

    if (failures != 0) {
        std::cerr << failures << " M4.5 parameter mapping test(s) failed\n";
        return 1;
    }

    std::cout << "PASS: M4.5 portable VST3 parameter mapping\n";
    return 0;
}
