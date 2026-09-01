#pragma once

#include "resonant/Types.hpp"

#include <array>
#include <cstdint>
#include <span>

namespace resonant {

using ParameterId = std::uint32_t;
using NoteId = std::uint32_t;

inline constexpr NoteId kNoNoteId = 0;

enum class EventType : std::uint8_t {
    NoteOff,
    Pitch,
    Pressure,
    ParameterChange,
    PerNoteExpression,
    NoteOn,
    Trigger,
};

struct Event {
    std::uint32_t sample_offset{0};
    EventType type{EventType::ParameterChange};
    ParameterId target{0};
    NoteId note_id{kNoNoteId};
    Sample value{0.0F};
    Sample value2{0.0F};

    [[nodiscard]] bool finite() const noexcept {
        return std::isfinite(value) && std::isfinite(value2);
    }
};

[[nodiscard]] constexpr std::uint8_t eventPriority(EventType type) noexcept {
    switch (type) {
    case EventType::NoteOff: return 0;
    case EventType::Pitch: return 1;
    case EventType::Pressure: return 2;
    case EventType::ParameterChange: return 3;
    case EventType::PerNoteExpression: return 4;
    case EventType::NoteOn: return 5;
    case EventType::Trigger: return 6;
    }
    return 7;
}

[[nodiscard]] constexpr bool eventLess(const Event& a, const Event& b) noexcept {
    if (a.sample_offset != b.sample_offset) {
        return a.sample_offset < b.sample_offset;
    }
    const auto ap = eventPriority(a.type);
    const auto bp = eventPriority(b.type);
    if (ap != bp) {
        return ap < bp;
    }
    if (a.note_id != b.note_id) {
        return a.note_id < b.note_id;
    }
    return a.target < b.target;
}

[[nodiscard]] inline bool eventsAreOrdered(std::span<const Event> events,
                                           std::uint32_t frames) noexcept {
    for (std::size_t i = 0; i < events.size(); ++i) {
        if (events[i].sample_offset >= frames || !events[i].finite()) {
            return false;
        }
        if (i > 0 && eventLess(events[i], events[i - 1])) {
            return false;
        }
    }
    return true;
}

template <std::size_t Capacity = kMaxEventsPerBlock>
class FixedEventBuffer {
public:
    [[nodiscard]] bool push(Event event) noexcept {
        if (size_ >= Capacity) {
            overflowed_ = true;
            return false;
        }
        std::size_t pos = size_;
        while (pos > 0 && eventLess(event, events_[pos - 1])) {
            events_[pos] = events_[pos - 1];
            --pos;
        }
        events_[pos] = event;
        ++size_;
        return true;
    }

    void clear() noexcept {
        size_ = 0;
        overflowed_ = false;
    }

    [[nodiscard]] std::span<const Event> span() const noexcept {
        return {events_.data(), size_};
    }

    [[nodiscard]] std::size_t size() const noexcept { return size_; }
    [[nodiscard]] bool overflowed() const noexcept { return overflowed_; }

private:
    std::array<Event, Capacity> events_{};
    std::size_t size_{0};
    bool overflowed_{false};
};

} // namespace resonant
