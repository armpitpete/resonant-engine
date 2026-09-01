#include "resonant/Engine.hpp"
#include "resonant/Random.hpp"
#include "resonant/ReferenceFeedbackProbe.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

namespace {

struct Options {
    std::string output{"resonant-render.wav"};
    double sample_rate{48'000.0};
    std::uint32_t block_size{64};
    double duration_seconds{1.0};
    resonant::Seed seed{resonant::kDefaultSeed};
    bool silent{false};
};

void usage() {
    std::cout
        << "resonant_render - deterministic Resonant Engine M0 renderer\n"
        << "Usage: resonant_render [options]\n"
        << "  --output PATH        Output WAV path (default resonant-render.wav)\n"
        << "  --sample-rate HZ     8000..384000 (default 48000)\n"
        << "  --block-size FRAMES  1..4096 (default 64)\n"
        << "  --duration SECONDS   >0 and <=60 (default 1.0)\n"
        << "  --seed INTEGER       Deterministic 64-bit seed\n"
        << "  --silent             Render the zero-excitation placeholder path\n"
        << "  --help               Show this help\n";
}

bool parseUnsigned(std::string_view text, std::uint64_t& value) {
    try {
        std::size_t consumed = 0;
        const auto parsed = std::stoull(std::string{text}, &consumed, 0);
        if (consumed != text.size()) return false;
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
        if (consumed != text.size() || !std::isfinite(parsed)) return false;
        value = parsed;
        return true;
    } catch (...) {
        return false;
    }
}

bool parseOptions(int argc, char** argv, Options& options) {
    for (int i = 1; i < argc; ++i) {
        const std::string_view arg{argv[i]};
        if (arg == "--help") {
            usage();
            return false;
        }
        if (arg == "--silent") {
            options.silent = true;
            continue;
        }
        if (i + 1 >= argc) {
            std::cerr << "Missing value for " << arg << '\n';
            return false;
        }
        const std::string_view value{argv[++i]};
        if (arg == "--output") {
            options.output = std::string{value};
        } else if (arg == "--sample-rate") {
            if (!parseDouble(value, options.sample_rate)) return false;
        } else if (arg == "--block-size") {
            std::uint64_t parsed = 0;
            if (!parseUnsigned(value, parsed) || parsed > std::numeric_limits<std::uint32_t>::max()) return false;
            options.block_size = static_cast<std::uint32_t>(parsed);
        } else if (arg == "--duration") {
            if (!parseDouble(value, options.duration_seconds)) return false;
        } else if (arg == "--seed") {
            if (!parseUnsigned(value, options.seed)) return false;
        } else {
            std::cerr << "Unknown option: " << arg << '\n';
            return false;
        }
    }
    const resonant::ProcessSpec spec{options.sample_rate, options.block_size, 1, 1};
    return !options.output.empty() && spec.valid() &&
           options.duration_seconds > 0.0 && options.duration_seconds <= 60.0;
}

void writeU16(std::ofstream& out, std::uint16_t value) {
    const char bytes[2]{static_cast<char>(value & 0xffU), static_cast<char>((value >> 8U) & 0xffU)};
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
    if (samples.size() > (std::numeric_limits<std::uint32_t>::max() - 44U) / 2U) return false;
    std::ofstream out(path, std::ios::binary);
    if (!out) return false;
    const auto data_bytes = static_cast<std::uint32_t>(samples.size() * sizeof(std::int16_t));
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
    for (float sample : samples) {
        const float c = std::clamp(std::isfinite(sample) ? sample : 0.0F, -1.0F, 1.0F);
        const auto pcm = static_cast<std::int16_t>(std::lrint(c * 32767.0F));
        writeU16(out, static_cast<std::uint16_t>(pcm));
    }
    return static_cast<bool>(out);
}

} // namespace

int main(int argc, char** argv) {
    Options options;
    if (!parseOptions(argc, argv, options)) {
        if (argc > 1 && std::string_view{argv[1]} == "--help") return 0;
        usage();
        return 2;
    }

    resonant::Engine<resonant::ReferenceFeedbackProbe> engine;
    const resonant::ProcessSpec spec{options.sample_rate, options.block_size, 1, 1};
    if (!engine.prepare(spec)) {
        std::cerr << "Failed to prepare engine\n";
        return 3;
    }

    const auto total_frames = static_cast<std::uint64_t>(
        std::llround(options.duration_seconds * options.sample_rate));
    std::vector<float> rendered;
    rendered.reserve(static_cast<std::size_t>(total_frames));
    std::vector<float> input(options.block_size, 0.0F);
    std::vector<float> output(options.block_size, 0.0F);
    resonant::Pcg32 rng{options.seed};

    std::uint64_t rendered_frames = 0;
    bool first_block = true;
    while (rendered_frames < total_frames) {
        const auto frames = static_cast<std::uint32_t>(
            std::min<std::uint64_t>(options.block_size, total_frames - rendered_frames));
        for (std::uint32_t i = 0; i < frames; ++i) {
            input[i] = options.silent ? 0.0F : rng.nextSignedFloat() * 0.012F;
        }
        const float* inputs[1]{input.data()};
        float* outputs[1]{output.data()};
        resonant::AudioBlockView block{inputs, outputs, 1, 1, frames};
        resonant::FixedEventBuffer<8> events;
        if (first_block && !options.silent) {
            if (!events.push({0, resonant::EventType::ParameterChange,
                              resonant::ReferenceFeedbackProbe::kExcitation, 0, 0.12F, 0.0F}) ||
                !events.push({0, resonant::EventType::ParameterChange,
                              resonant::ReferenceFeedbackProbe::kFeedback, 0, 0.28F, 0.0F}) ||
                !events.push({0, resonant::EventType::ParameterChange,
                              resonant::ReferenceFeedbackProbe::kFeedbackFilter, 0, 0.2F, 0.0F}) ||
                !events.push({0, resonant::EventType::ParameterChange,
                              resonant::ReferenceFeedbackProbe::kNonlinearity, 0, 0.4F, 0.0F})) {
                std::cerr << "Internal render event capacity error\n";
                return 6;
            }
            first_block = false;
        }
        if (engine.process(block, events.span()) != resonant::ProcessStatus::Ok) {
            std::cerr << "Engine processing failed\n";
            return 4;
        }
        rendered.insert(rendered.end(), output.begin(), output.begin() + frames);
        rendered_frames += frames;
    }

    if (!writeWav16(options.output, rendered, static_cast<std::uint32_t>(options.sample_rate))) {
        std::cerr << "Failed to write WAV: " << options.output << '\n';
        return 5;
    }

    std::cout << "Wrote " << rendered.size() << " frames to " << options.output
              << " seed=" << options.seed << '\n';
    return 0;
}
