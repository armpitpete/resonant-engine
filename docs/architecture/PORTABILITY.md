# M0.17 — Cross-Platform Portability Proof

Status: **APPROVED FOR M0**

## Audit

The M0 core was reviewed against the following assumptions:

| Risk | M0 result |
|---|---|
| Windows-only APIs | none in core |
| POSIX-only APIs | none in core |
| filesystem during DSP | none in core |
| threads/locks | none required by core processing |
| SIMD ISA | none assumed |
| endianness | DSP operates on typed samples; WAV byte order lives in render host |
| alignment | no over-aligned/SIMD load assumption |
| floating point | float32 audio + selective double; finite checks explicit |
| 64-bit-only pointers | no pointer-width serialization or arithmetic |
| exceptions/RTTI | processing contract does not require them; CI compiles core TU with both disabled |
| heap in processing | demonstrated path has allocation-count test |

## WASM / AudioWorklet

The core uses C++20 standard-library facilities that are available in modern Emscripten toolchains (`std::span`, fixed arrays, arithmetic). It contains no DOM, JavaScript, Web Audio, threads or filesystem dependency. A browser Host can compile `resonant_core` to WASM and have an AudioWorklet wrapper translate Web Audio blocks/events into `AudioBlockView` + Event spans. The sample-internal loop means browser render quantum size does not become feedback delay.

M0 does not ship the production WASM wrapper; no architectural rewrite is known to be required.

## VST3 / JUCE

A VST3 Host wrapper can translate process buffers/sample offsets/automation into the same core structures. JUCE may be used by a wrapper but cannot leak into `resonant_core`. The core does not assume plugin buses, parameter classes or DAW transport objects.

## Embedded ARM

No OS API, exceptions/RTTI requirement, dynamic process allocation or SIMD ISA is required. Fixed event buffers and caller-owned audio suit static-memory firmware. `resonant_core` itself holds only model state; current probe state is small (well under a kilobyte). Host/device drivers, DMA, MIDI/USB and flash/filesystem remain outside the core.

Stack work in `Engine::process()` is two fixed `std::array<float,2>` frames plus counters/spans, independent of host block size. No maximum static audio buffer is allocated inside core.

## CPU predictability

Processing is O(frames + events + model sample work) with explicit caps (4096 frames, 1024 events). FixedEventBuffer insertion is O(capacity²) in the worst case but is a Host/preparation-side utility and is not used inside the engine sample loop. Future models must provide their own bounded-work evidence.

## M0 blockers

No portability blocker requiring a core rewrite was found. Deferred target work:

- actual Emscripten/AudioWorklet integration;
- VST3/JUCE wrapper binaries;
- a specific ARM cross-toolchain/board build;
- platform performance budgets/SIMD tuning.

Those are implementation milestones, not unresolved M0 architectural blockers.

**M0.17 Cross-Platform Portability Proof: APPROVED.**
