#pragma once

#include "resonant/Event.hpp"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <span>

namespace resonant::vst3 {

class HostEventTranslator {
public:
    void reset() noexcept {
        events_.clear();
        frames_ = 0U;
        malformed_ = false;
        overflowed_ = false;
        active_ = false;
        active_note_id_ = kNoNoteId;
        active_pitch_ = -1;
        active_base_pitch_hz_ = 0.0F;
        checkpoint_active_ = false;
        checkpoint_note_id_ = kNoNoteId;
        checkpoint_pitch_ = -1;
        checkpoint_base_pitch_hz_ = 0.0F;
        have_last_offset_ = false;
        last_offset_ = 0U;
    }

    void beginBlock(std::uint32_t frames) noexcept {
        events_.clear();
        frames_ = frames;
        malformed_ = false;
        overflowed_ = false;
        checkpoint_active_ = active_;
        checkpoint_note_id_ = active_note_id_;
        checkpoint_pitch_ = active_pitch_;
        checkpoint_base_pitch_hz_ = active_base_pitch_hz_;
        have_last_offset_ = false;
        last_offset_ = 0U;
    }

    [[nodiscard]] bool noteOn(std::uint32_t sample_offset,
                              std::int32_t pitch,
                              float tuning_cents,
                              float velocity,
                              std::int32_t host_note_id) noexcept {
        if (!acceptOffset(sample_offset) || !validPitch(pitch) ||
            !std::isfinite(tuning_cents) || !validUnit(velocity) ||
            !reserve(3U)) {
            malformed_ = !overflowed_;
            return false;
        }

        const auto note_id = mapNoteId(host_note_id);
        const auto pitch_hz = midiPitchHz(pitch, tuning_cents);
        if (!std::isfinite(pitch_hz)) {
            malformed_ = true;
            return false;
        }

        (void)events_.push(
            {sample_offset, EventType::Pitch, 0U, kNoNoteId, pitch_hz, 0.0F});
        (void)events_.push(
            {sample_offset, EventType::Pressure, 0U, kNoNoteId, velocity, 0.0F});
        (void)events_.push(
            {sample_offset, EventType::NoteOn, 0U, note_id, velocity, 0.0F});

        active_ = true;
        active_note_id_ = note_id;
        active_pitch_ = pitch;
        active_base_pitch_hz_ = pitch_hz;
        return true;
    }

    [[nodiscard]] bool noteExpressionTuning(std::uint32_t sample_offset,
                                            double normalized_value,
                                            std::int32_t host_note_id) noexcept {
        if (!acceptOffset(sample_offset) || !std::isfinite(normalized_value) ||
            normalized_value < 0.0 || normalized_value > 1.0) {
            malformed_ = true;
            return false;
        }

        const auto note_id = mapNoteId(host_note_id);
        if (!matchesActive(note_id, active_pitch_) || active_base_pitch_hz_ <= 0.0F) {
            return true;
        }
        if (!reserve(1U)) {
            return false;
        }

        const auto semitones = 240.0 * (normalized_value - 0.5);
        const auto pitch_hz = static_cast<Sample>(
            static_cast<double>(active_base_pitch_hz_) * std::exp2(semitones / 12.0));
        if (!std::isfinite(pitch_hz)) {
            malformed_ = true;
            return false;
        }

        (void)events_.push(
            {sample_offset, EventType::Pitch, 0U, kNoNoteId, pitch_hz, 0.0F});
        return true;
    }

    [[nodiscard]] bool noteOff(std::uint32_t sample_offset,
                               std::int32_t pitch,
                               float release_velocity,
                               std::int32_t host_note_id) noexcept {
        if (!acceptOffset(sample_offset) || !validPitch(pitch) ||
            !validUnit(release_velocity)) {
            malformed_ = true;
            return false;
        }

        const auto note_id = mapNoteId(host_note_id);
        const auto releases_active = matchesActive(note_id, pitch);
        if (!reserve(releases_active ? 2U : 1U)) {
            return false;
        }

        (void)events_.push(
            {sample_offset, EventType::NoteOff, 0U, note_id, release_velocity, 0.0F});
        if (releases_active) {
            (void)events_.push(
                {sample_offset, EventType::Pressure, 0U, kNoNoteId, 0.0F, 0.0F});
            active_ = false;
            active_note_id_ = kNoNoteId;
            active_pitch_ = -1;
            active_base_pitch_hz_ = 0.0F;
        }
        return true;
    }

    [[nodiscard]] bool polyPressure(std::uint32_t sample_offset,
                                    std::int32_t pitch,
                                    float pressure,
                                    std::int32_t host_note_id) noexcept {
        if (!acceptOffset(sample_offset) || !validPitch(pitch) ||
            !validUnit(pressure)) {
            malformed_ = true;
            return false;
        }

        const auto note_id = mapNoteId(host_note_id);
        if (!matchesActive(note_id, pitch)) {
            return true;
        }
        if (!reserve(1U)) {
            return false;
        }

        (void)events_.push(
            {sample_offset, EventType::Pressure, 0U, kNoNoteId, pressure, 0.0F});
        return true;
    }

    [[nodiscard]] std::span<const Event> events() const noexcept {
        return events_.span();
    }

    void abortBlock() noexcept {
        events_.clear();
        malformed_ = false;
        overflowed_ = false;
        active_ = checkpoint_active_;
        active_note_id_ = checkpoint_note_id_;
        active_pitch_ = checkpoint_pitch_;
        active_base_pitch_hz_ = checkpoint_base_pitch_hz_;
        have_last_offset_ = false;
        last_offset_ = 0U;
    }

    [[nodiscard]] bool valid() const noexcept {
        return !malformed_ && !overflowed_ && !events_.overflowed();
    }

    [[nodiscard]] bool overflowed() const noexcept {
        return overflowed_ || events_.overflowed();
    }

    [[nodiscard]] bool hasActiveNote() const noexcept { return active_; }
    [[nodiscard]] NoteId activeNoteId() const noexcept { return active_note_id_; }

    [[nodiscard]] static NoteId mapNoteId(std::int32_t host_note_id) noexcept {
        return host_note_id >= 0
                   ? static_cast<NoteId>(static_cast<std::uint32_t>(host_note_id) + 1U)
                   : kNoNoteId;
    }

    [[nodiscard]] static Sample midiPitchHz(std::int32_t pitch,
                                            float tuning_cents) noexcept {
        if (!validPitch(pitch) || !std::isfinite(tuning_cents)) {
            return 0.0F;
        }
        const auto semitones =
            static_cast<double>(pitch - 69) + static_cast<double>(tuning_cents) / 100.0;
        return static_cast<Sample>(440.0 * std::exp2(semitones / 12.0));
    }

private:
    [[nodiscard]] static bool validPitch(std::int32_t pitch) noexcept {
        return pitch >= 0 && pitch <= 127;
    }

    [[nodiscard]] static bool validUnit(float value) noexcept {
        return std::isfinite(value) && value >= 0.0F && value <= 1.0F;
    }

    [[nodiscard]] bool acceptOffset(std::uint32_t sample_offset) noexcept {
        if (frames_ == 0U || sample_offset >= frames_ ||
            (have_last_offset_ && sample_offset < last_offset_)) {
            return false;
        }
        have_last_offset_ = true;
        last_offset_ = sample_offset;
        return true;
    }

    [[nodiscard]] bool reserve(std::size_t count) noexcept {
        if (events_.size() + count > kMaxEventsPerBlock) {
            overflowed_ = true;
            return false;
        }
        return true;
    }

    [[nodiscard]] bool matchesActive(NoteId note_id,
                                     std::int32_t pitch) const noexcept {
        if (!active_) {
            return false;
        }
        if (note_id != kNoNoteId && active_note_id_ != kNoNoteId) {
            return note_id == active_note_id_;
        }
        return pitch == active_pitch_;
    }

    FixedEventBuffer<kMaxEventsPerBlock> events_{};
    std::uint32_t frames_{0U};
    bool malformed_{false};
    bool overflowed_{false};
    bool active_{false};
    NoteId active_note_id_{kNoNoteId};
    std::int32_t active_pitch_{-1};
    Sample active_base_pitch_hz_{0.0F};
    bool checkpoint_active_{false};
    NoteId checkpoint_note_id_{kNoNoteId};
    std::int32_t checkpoint_pitch_{-1};
    Sample checkpoint_base_pitch_hz_{0.0F};
    bool have_last_offset_{false};
    std::uint32_t last_offset_{0U};
};

} // namespace resonant::vst3
