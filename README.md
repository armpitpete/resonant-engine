# Resonant Engine

Resonant Engine is a portable resonant/physical synthesis platform capable of running from the same core DSP architecture in browsers, DAWs, embedded hardware and other host instruments.

## M0 — Portable Real-Time Foundation

M0 establishes a portable real-time DSP architecture specifically capable of supporting expressive, continuously excited resonant systems, while remaining general enough for strings, structures, coupled resonators and impossible synthetic bodies later.

**Primary sonic design priority:** air / pipe / noise.

M0 is architecturally centered on one C++20 `resonant_core`. Browser/WASM, VST3/JUCE, embedded hardware and other synths are thin Hosts around that shared core; host-specific DSP duplication is prohibited.

## M0 result

The completion branch contains the full 22-section M0 contract/runtime foundation:

- normative product/scope, sonic, terminology and core-boundary contracts;
- hard real-time processing rules;
- float32 non-interleaved block transport with sample-level model feedback;
- sample-accurate deterministic events;
- native/normalized parameters with sample-rate-aware smoothing;
- generic exciter/resonator compatibility contracts;
- feedback, energy/stability and lifecycle contracts;
- deterministic PCG32 randomness/voice seed derivation;
- Debug/Release, MSVC/GCC/Clang and sanitizer CI;
- deterministic offline WAV render host;
- unit/property/regression testing foundation;
- WASM/VST3/embedded portability proof;
- research/reference record and accepted ADR register;
- final hostile Breath Pipe architecture review and M0 completion gate.

The current `ReferenceFeedbackProbe` is an **architectural fixture**, not the Breath Pipe voice and not a finished physical model.

## Protected future Breath Pipe reference

```text
Pressure + Turbulence
        ↓
Exciter interaction
        ↕
Tuned resonator
        ↺
Active feedback
        ↓
Feedback filtering
        ↓
Nonlinearity
        ↺
```

Required future continuum:

```text
silence
→ faint air
→ turbulence
→ emerging pitch
→ unstable resonance
→ stable pipe
→ rich pipe
→ overblow
→ aggressive resonance
→ noise
```

## Build

```sh
cmake -S . -B build -DRESONANT_ENGINE_BUILD_TESTS=ON -DRESONANT_ENGINE_BUILD_RENDER=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Offline deterministic render:

```sh
./build/resonant_render --sample-rate 48000 --block-size 64 --duration 0.1 --seed 777 --output m0-canonical.wav
```

## Documentation

Start with:

- `docs/M0-COMPLETION.md`
- `docs/architecture/PRODUCT_SCOPE.md`
- `docs/architecture/AIR_PIPE_NOISE.md`
- `docs/architecture/GLOSSARY.md`
- `docs/architecture/CORE_BOUNDARY.md`
- `docs/architecture/REALTIME.md`
- `docs/architecture/PROCESSING.md`
- `docs/architecture/EVENTS.md`
- `docs/architecture/PARAMETERS.md`
- `docs/architecture/EXCITER_RESONATOR.md`
- `docs/architecture/FEEDBACK.md`
- `docs/architecture/ENERGY_STABILITY.md`
- `docs/architecture/DETERMINISM.md`
- `docs/architecture/LIFECYCLE.md`
- `docs/architecture/PORTABILITY.md`
- `docs/research/INDEX.md`
- `docs/decisions/ADR_INDEX.md`

## Licence

MIT. See `LICENSE`.
