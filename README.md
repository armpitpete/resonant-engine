# Resonant Engine

Resonant Engine is a portable resonant/physical synthesis platform capable of running from the same core DSP architecture in browsers, DAWs, embedded hardware and other host instruments.

## M0 — Portable Real-Time Foundation

**Status: COMPLETE AND FROZEN.**

M0 establishes a portable real-time DSP architecture specifically capable of supporting expressive, continuously excited resonant systems, while remaining general enough for strings, structures, coupled resonators and impossible synthetic bodies later.

**Primary sonic design priority:** air / pipe / noise.

M0 is architecturally centered on one C++20 `resonant_core`. Browser/WASM, VST3/JUCE, embedded hardware and other synths are thin Hosts around that shared core; host-specific DSP duplication is prohibited.

## M0 result

The merged `main` branch contains the full 22-section M0 contract/runtime foundation:

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
- WASM/VST3/embedded portability proof for M0 scope;
- research/reference record and accepted ADR register;
- final hostile Breath Pipe architecture review and M0 completion gate.

The `ReferenceFeedbackProbe` remains an **M0 architectural fixture**, not the Breath Pipe voice and not the preferred musical model.

M0 is frozen as the baseline for later milestones. Later work may extend it through explicit architecture decisions, but must not silently reinterpret the M0 invariants or Host/core boundary.

## M1 — First Resonator

**Status: CANDIDATE — engineering gates pass; human listening gate remains.**

M1 adds the first concrete musical resonator without changing the frozen Engine/Host architecture:

- `ContinuousNoiseExciter` — deterministic continuous noise, transient and arbitrary external-audio excitation;
- `TunedDelayResonator` — fixed-memory fractional-delay resonance with damping, passive loss, active regeneration and bounded in-loop nonlinearity;
- `FirstResonatorVoice` — sample-accurate, smoothed composition of those primitives with energy/stability observation;
- six deterministic listening/regression fixtures: passive pluck, continuous, feedback, nonlinear, sweep and silence.

The M1 reference model is deliberately generic and replaceable. It is **not Breath Pipe**, not a Steampipe clone and not a rule that future Resonant Engine models must use tuned delays.

Exact implementation-head CI has passed Ubuntu, macOS and Windows in Debug/Release, ASan+UBSan and the no-exceptions/no-RTTI core probe. The hostile architecture review found no M0 invariant violation. M1 remains a candidate until its retained audio fixtures pass human perceptual acceptance and the final protected merge gate.

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

M0 deterministic architecture fixture:

```sh
./build/resonant_render --sample-rate 48000 --block-size 64 --duration 0.1 --seed 777 --output m0-canonical.wav
```

M1 first-resonator listening fixture:

```sh
./build/resonant_m1_render --scene continuous --sample-rate 48000 --block-size 64 --duration 1.0 --seed 777 --output m1-continuous.wav
```

Available M1 scenes:

```text
passive-pluck
continuous
feedback
nonlinear
sweep
silent
```

## Documentation

M1:

- `docs/M1-FIRST-RESONATOR.md`
- `docs/M1-COMPLETION.md`
- `docs/M1-HOSTILE-ARCHITECTURE-REVIEW.md`
- `docs/decisions/ADR-0023-M1-FIRST-RESONATOR.md`
- `docs/research/M1-FIRST-RESONATOR.md`

Frozen M0 baseline:

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
