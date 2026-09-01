#include "resonant/Engine.hpp"
#include "resonant/FirstResonator.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

namespace {

struct Options {
    std::string output{"m1-first-resonator.wav"};
    std::string scene{"continuous"};
    double sample_rate{48'000.0};
    std::uint32_t block_size{64};
    double duration_seconds{1.0};
    resonant::Seed seed{777};
};

void usage() {
    std::cout
        << "resonant_m1_render - deterministic M1 first-resonator renderer\n"
        << "Usage: resonant_m1_render [options]\n"
        << "  --output PATH        Output WAV path\n"
        << "  --scene NAME         passive-pluck|continuous|feedback|nonlinear|sweep|silent\n"
        << "  --sample-rate HZ     8000..384000 (default 48000)\n"
        << "  --block-size FRAMES  1..4096 (default 64)\n"
        << "  --duration SECONDS   >0 and <=60 (default 1.0)\n"
        << "  --seed INTEGER       Deterministic 64-bit seed (default 777)\n"
        << "  --help               Show this help\n";
}

bool parseUnsigned(std::string_view text, std::uint64_t& value) {
    try {
        std::size_t consumed = 0;
        const auto parsed = std::stoull(std::string{text}, &consumed, 0);
        if (consumed != text.size()) {
            return false;
        }
        value = parsed;
        return true;
    } catch (...) {
        return false;
    }
}

bool parseDouble(std::string_view text, double& value) {
    try {
        std::size_t consumed = 0;
        const auto parsed = std::stod(std::string{text}, &consumed);
        if (consumed != text.size() || !std::isfinite(parsed)) {
            return false;
        }
        value = parsed;
        return true;
    } catch (...) {
        return false;
    }
}

bool validScene(std::string_view scene) {
    return scene == "passive-pluck" || scene == "continuous" ||
           scene == "feedback" || scene == "nonlinear" ||
           scene == "sweep" || scene == "silent";
}

bool parseOptions(int argc, char** argv, Options& options) {
    for (int i = 1; i < argc; ++i) {
        const std::string_view arg{argv[i]};
        if (arg == "--help") {
            usage();
            return false;
        }
        if (i + 1 >= argc) {
            std::cerr << "Missing value for " << arg << '\n';
            return false;
        }
        const std::string_view value{argv[++i]};
        if (arg == "--output") {
            options.output = std::string{value};
        } else if (arg == "--scene") {
            options.scene = std::string{value};
        } else if (arg == "--sample-rate") {
            if (!parseDouble(value, options.sample_rate)) {
                return false;
            }
        } else if (arg == "--block-size") {
            std::uint64_t parsed = 0;
            if (!parseUnsigned(value, parsed) ||
                parsed > std::numeric_limits<std::uint32_t>::max()) {
                return false;
            }
            options.block_size = static_cast<std::uint32_t>(parsed);
        } else if (arg == "--duration") {
            if (!parseDouble(value, options.duration_seconds)) {
                return false;
            }
        } else if (arg == "--seed") {
            if (!parseUnsigned(value, options.seed)) {
                return false;
            }
        } else {
            std::cerr << "Unknown option: " << arg << '\n';
            return false;
        }
    }

    const resonant::ProcessSpec spec{options.sample_rate, options.block_size, 1, 1};
    return !options.output.empty() && validScene(options.scene) && spec.valid() &&
           options.duration_seconds > 0.0 && options.duration_seconds <= 60.0;
}

void writeU16(std::ofstream& out, std::uint16_t value) {
    const char bytes[2]{static_cast<char>(value & 0xffU),
                        static_cast<char>((value >> 8U) & 0xffU)};
    out.write(bytes, 2);
}

void writeU32(std::ofstream& out, std::uint32_t value) {
    const char bytes[4]{
        static_cast<char>(value & 0xffU),
        static_cast<char>((value >> 8U) & 0xffU),
        static_cast<char>((value >> 16U) & 0xffU),
        static_cast<char>((value >> 24U) & 0xffU)};
    out.write(bytes, 4);
}

bool writeWav16(const std::string& path, const std::vector<float>& samples,
                std::uint32_t sample_rate) {
    if (samples.size() >
        (std::numeric_limits<std::uint32_t>::max() - 44U) / 2U) {
        return false;
    }
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        return false;
    }
    const auto data_bytes = static_cast<std::uint32_t>(
        samples.size() * sizeof(std::int16_t));
    out.write("RIFF", 4);
    writeU32(out, 36U + data_bytes);
    out.write("WAVE", 4);
    out.write("fmt ", 4);
    writeU32(out, 16U);
    writeU16(out, 1U);
    writeU16(out, 1U);
    writeU32(out, sample_rate);
    writeU32(out, sample_rate * 2U);
    writeU16(out, 2U);
    writeU16(out, 16U);
    out.write("data", 4);
    writeU32(out, data_bytes);
    for (const auto sample : samples) {
        const auto clamped = std::clamp(
            std::isfinite(sample) ? sample : 0.0F, -1.0F, 1.0F);
        const auto pcm = static_cast<std::int16_t>(
            std::lrint(clamped * 32767.0F));
        writeU16(out, static_cast<std::uint16_t>(pcm));
    }
    return static_cast<bool>(out);
}

bool addInitialSceneEvents(std::string_view scene,
                           resonant::FixedEventBuffer<12>& events) {
    using Voice = resonant::FirstResonatorVoice;
    if (scene == "silent") {
        return true;
    }
    if (scene == "passive-pluck") {
        return events.push({0, resonant::EventType::Pitch, 0, 0, 220.0F, 0.0F}) &&
               events.push({0, resonant::EventType::ParameterChange,
                            Voice::kDamping, 0, 0.34F, 0.0F}) &&
               events.push({0, resonant::EventType::ParameterChange,
                            Voice::kFeedback, 0, 0.0F, 0.0F}) &&
               events.push({0, resonant::EventType::ParameterChange,
                            Voice::kNonlinearity, 0, 0.10F, 0.0F}) &&
               events.push({0, resonant::EventType::Trigger, 0, 0, 0.9F, 0.0F});
    }
    if (scene == "continuous" || scene == "sweep") {
        return events.push({0, resonant::EventType::Pitch, 0, 0,
                            scene == "sweep" ? 110.0F : 330.0F, 0.0F}) &&
               events.push({0, resonant::EventType::Pressure, 0, 0, 0.42F, 0.0F}) &&
               events.push({0, resonant::EventType::ParameterChange,
                            Voice::kTurbulence, 0, 0.82F, 0.0F}) &&
               events.push({0, resonant::EventType::ParameterChange,
                            Voice::kDamping, 0, 0.30F, 0.0F}) &&
               events.push({0, resonant::EventType::ParameterChange,
                            Voice::kFeedback, 0, 0.24F, 0.0F}) &&
               events.push({0, resonant::EventType::ParameterChange,
                            Voice::kNonlinearity, 0, 0.30F, 0.0F});
    }
    if (scene == "feedback") {
        return events.push({0, resonant::EventType::Pitch, 0, 0, 196.0F, 0.0F}) &&
               events.push({0, resonant::EventType::Pressure, 0, 0, 0.28F, 0.0F}) &&
               events.push({0, resonant::EventType::ParameterChange,
                            Voice::kTurbulence, 0, 0.72F, 0.0F}) &&
               events.push({0, resonant::EventType::ParameterChange,
                            Voice::kDamping, 0, 0.24F, 0.0F}) &&
               events.push({0, resonant::EventType::ParameterChange,
                            Voice::kFeedback, 0, 0.60F, 0.0F}) &&
               events.push({0, resonant::EventType::ParameterChange,
                            Voice::kNonlinearity, 0, 0.48F, 0.0F}) &&
               events.push({0, resonant::EventType::ParameterChange,
                            Voice::kInteraction, 0, 0.20F, 0.0F});
    }
    if (scene == "nonlinear") {
        return events.push({0, resonant::EventType::Pitch, 0, 0, 110.0F, 0.0F}) &&
               events.push({0, resonant::EventType::Pressure, 0, 0, 0.35F, 0.0F}) &&
               events.push({0, resonant::EventType::ParameterChange,
                            Voice::kTurbulence, 0, 0.95F, 0.0F}) &&
               events.push({0, resonant::EventType::ParameterChange,
                            Voice::kDamping, 0, 0.08F, 0.0F}) &&
               events.push({0, resonant::EventType::ParameterChange,
                            Voice::kFeedback, 0, 1.20F, 0.0F}) &&
               events.push({0, resonant::EventType::ParameterChange,
                            Voice::kNonlinearity, 0, 1.0F, 0.0F});
    }
    return false;
}

} // namespace

int main(int argc, char** argv) {
    Options options;
    if (!parseOptions(argc, argv, options)) {
        if (argc > 1 && std::string_view{argv[1]} == "--help") {
            return 0;
        }
        usage();
        return 2;
    }

    resonant::Engine<resonant::FirstResonatorVoice> engine{
        resonant::FirstResonatorVoice{options.seed}};
    const resonant::ProcessSpec spec{
        options.sample_rate, options.block_size, 1, 1};
    if (!engine.prepare(spec)) {
        std::cerr << "Failed to prepare M1 engine\n";
        return 3;
    }

    const auto total_frames = static_cast<std::uint64_t>(
        std::llround(options.duration_seconds * options.sample_rate));
    std::vector<float> rendered;
    rendered.reserve(static_cast<std::size_t>(total_frames));
    std::vector<float> input(options.block_size, 0.0F);
    std::vector<float> output(options.block_size, 0.0F);

    std::uint64_t rendered_frames = 0;
    bool first_block = true;
    bool sweep_event_sent = false;
    const auto sweep_frame = total_frames / 2U;

    while (rendered_frames < total_frames) {
        const auto frames = static_cast<std::uint32_t>(
            std::min<std::uint64_t>(options.block_size,
                                    total_frames - rendered_frames));
        const float* inputs[1]{input.data()};
        float* outputs[1]{output.data()};
        resonant::AudioBlockView block{inputs, outputs, 1, 1, frames};
        resonant::FixedEventBuffer<12> events;

        if (first_block) {
            if (!addInitialSceneEvents(options.scene, events)) {
                std::cerr << "Internal M1 scene event capacity error\n";
                return 6;
            }
            first_block = false;
        }

        if (options.scene == "sweep" && !sweep_event_sent &&
            sweep_frame >= rendered_frames &&
            sweep_frame < rendered_frames + frames) {
            const auto offset = static_cast<std::uint32_t>(
                sweep_frame - rendered_frames);
            if (!events.push({offset, resonant::EventType::Pitch, 0, 0,
                              880.0F, 0.0F})) {
                std::cerr << "Internal M1 sweep event capacity error\n";
                return 6;
            }
            sweep_event_sent = true;
        }

        if (engine.process(block, events.span()) != resonant::ProcessStatus::Ok) {
            std::cerr << "M1 Engine processing failed\n";
            return 4;
        }
        rendered.insert(rendered.end(), output.begin(), output.begin() + frames);
        rendered_frames += frames;
    }

    if (!writeWav16(options.output, rendered,
                    static_cast<std::uint32_t>(options.sample_rate))) {
        std::cerr << "Failed to write WAV: " << options.output << '\n';
        return 5;
    }

    const auto& diagnostics = engine.model().diagnostics();
    std::cout << "Wrote " << rendered.size() << " M1 frames to "
              << options.output << " scene=" << options.scene
              << " seed=" << options.seed
              << " rms=" << diagnostics.output_rms
              << " peak=" << diagnostics.peak << '\n';
    return 0;
}
