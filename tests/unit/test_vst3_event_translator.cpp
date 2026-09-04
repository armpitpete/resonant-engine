#include "hosts/vst3/EventTranslator.hpp"

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

void testNoteOnAndIdentity() {
    resonant::vst3::HostEventTranslator translator;
    translator.beginBlock(128U);

    check(translator.noteOn(32U, 69, 0.0F, 0.75F, 41),
          "note-on translates");
    const auto events = translator.events();
    check(events.size() == 3U, "note-on emits pitch, pressure and note-on");
    check(events[0].type == resonant::EventType::Pitch,
          "pitch ordered first");
    check(events[1].type == resonant::EventType::Pressure,
          "pressure ordered second");
    check(events[2].type == resonant::EventType::NoteOn,
          "note-on ordered last");
    check(events[0].sample_offset == 32U &&
              events[1].sample_offset == 32U &&
              events[2].sample_offset == 32U,
          "sample offset preserved");
    check(events[0].note_id == 42U &&
              events[1].note_id == 42U &&
              events[2].note_id == 42U,
          "supplied VST note identity preserved without colliding with no-note id");
    check(std::abs(events[0].value - 440.0F) < 1.0e-4F,
          "MIDI pitch maps to Hz");
    check(events[1].value == 0.75F && events[2].value == 0.75F,
          "velocity drives pressure and trigger strength");
    check(translator.activeNoteId() == 42U,
          "active note identity retained across blocks");
}

void testReleaseAndPressure() {
    resonant::vst3::HostEventTranslator translator;
    translator.beginBlock(128U);
    check(translator.noteOn(8U, 60, 0.0F, 0.5F, 7),
          "prepare active note");

    translator.beginBlock(128U);
    check(translator.polyPressure(16U, 60, 0.9F, 7),
          "matching poly pressure translates");
    auto events = translator.events();
    check(events.size() == 1U &&
              events[0].type == resonant::EventType::Pressure &&
              events[0].value == 0.9F,
          "matching pressure reaches active note");

    translator.beginBlock(128U);
    check(translator.polyPressure(20U, 61, 0.2F, 8),
          "non-active pressure is safely ignored");
    check(translator.events().empty(),
          "pressure for another note does not retarget monophonic voice");

    translator.beginBlock(128U);
    check(translator.noteOff(40U, 60, 0.25F, 7),
          "matching note-off translates");
    events = translator.events();
    check(events.size() == 2U,
          "matching note-off emits release and pressure-zero");
    check(events[0].type == resonant::EventType::NoteOff &&
              events[1].type == resonant::EventType::Pressure,
          "release ordering is deterministic");
    check(events[1].value == 0.0F,
          "matching note-off closes breath pressure");
    check(!translator.hasActiveNote(),
          "matching note-off clears active identity");
}

void testOrderingAndFallbackIdentity() {
    resonant::vst3::HostEventTranslator translator;
    translator.beginBlock(128U);

    check(translator.noteOn(96U, 72, 0.0F, 0.6F, -1),
          "anonymous note-on translates");
    check(translator.noteOff(24U, 72, 0.1F, -1),
          "anonymous note-off translates");

    const auto events = translator.events();
    check(!events.empty(), "ordered event buffer populated");
    for (std::size_t index = 1; index < events.size(); ++index) {
        check(!resonant::eventLess(events[index], events[index - 1]),
              "events remain deterministically ordered");
    }
    check(resonant::vst3::HostEventTranslator::mapNoteId(-1) ==
              resonant::kNoNoteId,
          "missing host note id maps to portable no-note id");
}

void testMalformedAndOverflowPolicy() {
    resonant::vst3::HostEventTranslator translator;
    translator.beginBlock(64U);

    check(!translator.noteOn(64U, 60, 0.0F, 0.5F, 1),
          "out-of-range offset rejected");
    check(!translator.valid(), "malformed block marked invalid");

    translator.beginBlock(64U);
    check(!translator.noteOn(
              0U,
              60,
              0.0F,
              std::numeric_limits<float>::quiet_NaN(),
              1),
          "non-finite velocity rejected");

    translator.beginBlock(4096U);
    bool overflow_seen = false;
    for (std::uint32_t index = 0U; index < resonant::kMaxEventsPerBlock; ++index) {
        const auto ok = translator.noteOn(
            index % 4096U,
            static_cast<std::int32_t>(48U + index % 24U),
            0.0F,
            0.5F,
            static_cast<std::int32_t>(index));
        if (!ok) {
            overflow_seen = translator.overflowed();
            break;
        }
    }
    check(overflow_seen, "bounded event expansion fails closed on overflow");
}

} // namespace

int main() {
    testNoteOnAndIdentity();
    testReleaseAndPressure();
    testOrderingAndFallbackIdentity();
    testMalformedAndOverflowPolicy();

    if (failures != 0) {
        std::cerr << failures << " M4.4 event translator test(s) failed\n";
        return 1;
    }

    std::cout << "PASS: M4.4 bounded note/expression translation\n";
    return 0;
}
