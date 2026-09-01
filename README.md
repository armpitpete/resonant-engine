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

**Status: CANDIDATE — engineering gates pass; live play-test gate remains.**

M1 adds the first concrete musical resonator without changing the frozen Engine/Host architecture:

- `ContinuousNoiseExciter` — deterministic continuous noise, transient and arbitrary external-audio excitation;
- `TunedDelayResonator` — fixed-memory fractional-delay resonance with damping, passive loss, active regeneration and bounded in-loop nonlinearity;
- `FirstResonatorVoice` — sample-accurate, smoothed composition of those primitives with energy/stability observation;
- deterministic offline render scenes used only for regression evidence;
- a browser AudioWorklet/WASM play-test host that runs the same C++ `FirstResonatorVoice` live.

The M1 reference model is deliberately generic and replaceable. It is **not Breath Pipe**, not a Steampipe clone and not a rule that future Resonant Engine models must use tuned delays.

Exact implementation-head CI has passed Ubuntu, macOS and Windows in Debug/Release, ASan+UBSan and the no-exceptions/no-RTTI core probe. The hostile architecture review found no M0 invariant violation. M1 remains a candidate until the live synth passes human playability acceptance and the final protected merge gate.

### Play the M1 synth

Build the browser acceptance host with Emscripten:

```sh
./hosts/browser/m1/build.sh
```

Then serve `build/m1-browser/` on localhost. The CI-produced `m1-playable-browser-*` artifact contains the already-built harness and launch scripts.

The play-test supports computer-keyboard notes, Web MIDI notes, CC1/mod-wheel or channel pressure as continuous excitation, and live control of turbulence, damping, regeneration, nonlinearity and resonator/exciter interaction.

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

## Build and test

```sh
cmake -S . -B build -DRESONANT_ENGINE_BUILD_TESTS=ON -DRESONANT_ENGINE_BUILD_RENDER=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Offline render tools exist for deterministic regression evidence. They are not the human M1 acceptance method.

## Documentation

M1:

- `docs/M1-FIRST-RESONATOR.md`
- `docs/M1-COMPLETION.md`
- `docs/M1-HOSTILE-ARCHITECTURE-REVIEW.md`
- `docs/decisions/ADR-0023-M1-FIRST-RESONATOR.md`
- `docs/research/M1-FIRST-RESONATOR.md`
- `hosts/browser/m1/README.md`

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
