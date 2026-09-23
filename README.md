# Resonant Engine

Resonant Engine is a portable resonant/physical synthesis platform capable of running from the same core DSP architecture in browsers, DAWs, embedded hardware and other host instruments.

## M0 — Portable Real-Time Foundation

**Status: COMPLETE AND FROZEN.**

M0 establishes a portable real-time DSP architecture specifically capable of supporting expressive, continuously excited resonant systems, while remaining general enough for strings, structures, coupled resonators and impossible synthetic bodies later.

**Primary sonic design priority:** air / pipe / noise.

M0 is architecturally centered on one C++20 `resonant_core`. Browser/WASM, VST3/JUCE, embedded hardware and other synths are thin Hosts around that shared core; host-specific DSP duplication is prohibited.

The `ReferenceFeedbackProbe` remains an **M0 architectural fixture**, not the Breath Pipe voice and not the preferred musical model.

M0 is frozen as the baseline for later milestones. Later work may extend it through explicit architecture decisions, but must not silently reinterpret the M0 invariants or Host/core boundary.

## M1 — First Resonator

**Status: COMPLETE AND FROZEN.**

M1 adds the first concrete musical resonator without changing the frozen Engine/Host architecture:

- `ContinuousNoiseExciter` — deterministic continuous noise, transient and arbitrary external-audio excitation;
- `TunedDelayResonator` — fixed-memory fractional-delay resonance with damping, passive loss, active regeneration and bounded in-loop nonlinearity;
- `FirstResonatorVoice` — sample-accurate, smoothed composition of those primitives with energy/stability observation;
- deterministic offline render scenes used only for regression evidence;
- a browser AudioWorklet/WASM acceptance harness that runs the same C++ `FirstResonatorVoice` live.

M1 exact head `910616d0396ab516fa0b3272fe3067c23bffacb6` passed CI #60 and hostile review, then passed the human live gate through the later M2 Lab using the identical `FirstResonatorVoice` core. PR #4 merged to `main` as `54ebc45e4a0b2c96a7733ce99f49b3855acad9a7`; the merged tree is identical to the green exact-head tree.

The M1 reference model is deliberately generic and replaceable. It is **not Breath Pipe**, not a Steampipe clone and not a rule that future models must use tuned delays.

## M2 — Resonant Engine Lab

**Status: COMPLETE AND FROZEN.**

M2 makes the headless engine observable, measurable and reproducibly testable by humans.

**Resonant Engine Lab is not a browser synth.** It is a bounded diagnostic/test host around the shared C++ engine.

The Lab provides the same accepted core through WASM plus deterministic acceptance, measurement and evidence tooling. Human A01–A09 acceptance is **PASS**. M2 merged and is frozen.

Canonical M2 contract: `docs/M2-RESONANT-ENGINE-LAB.md`.

## M3 — Breath Pipe Reference Voice

**Status: ACCEPTED M3 BASELINE FROZEN ON `main` — M3.7 EXPRESSIVE REPAIR OPEN IN PR #20.**

M3 established the first fully expressive Breath Pipe reference voice and its accepted B01–B18 baseline. Direct M4.11 Cubase listening later exposed insufficient perceptual leverage in four primary musical macros. PR #20 therefore explicitly reopens only the M3.7 expressive mapping for Pressure, Turbulence, Damping and Nonlinear Drive while preserving the Breath Pipe topology and canonical Stable Pipe identity.

This is a proposed superseding repair, not a claim that the original frozen M3 implementation was untouched. If PR #20 passes its human and machine gates, the repaired M3.7 contract will be re-frozen with M4 closure.

Canonical M3 contract: `docs/M3-BREATH-PIPE.md`.

## M4 — DAW/VST3 Reference Host

**Status: M4.0–M4.10 MERGED AND COMPLETE — M4.11 CUBASE HUMAN ACCEPTANCE IN PROGRESS.**

M4 is the first real DAW-host proof for Resonant Engine. M4.0–M4.10 provide the thin VST3 instrument/controller, lifecycle, event/automation translation, portable state, external excitation, realtime proof, native/VST3 parity and Steinberg conformance evidence.

M4.11 is being executed in Cubase 15 on Windows. Scan/load and exact-candidate installation are established, but H04 expressive automation remains a direct-human FAIL. Processor-path diagnostics show strong full-range parameter effects, so further blind DSP widening is prohibited; the next gate is explicit Cubase endpoint automation to determine whether the operator-facing host path traverses the intended range. H05–H09 remain blocked until H04 passes.

M4 does not add a custom GUI, preset browser, or polyphonic VST3 allocator. Those remain later product work.

Canonical M4 contract: `docs/M4-DAW-VST3-HOST.md`.

## Build and test

```sh
cmake -S . -B build -DRESONANT_ENGINE_BUILD_TESTS=ON -DRESONANT_ENGINE_BUILD_RENDER=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Offline render tools are deterministic regression evidence. They are not substitutes for human listening gates.

## Documentation

- `docs/M4-DAW-VST3-HOST.md`
- `docs/M4.11-HUMAN-DAW-ACCEPTANCE.md`
- `docs/M3-BREATH-PIPE.md`
- `docs/M3-COMPLETION.md`
- `docs/M2-RESONANT-ENGINE-LAB.md`
- `docs/M2-COMPLETION.md`
- `docs/M1-FIRST-RESONATOR.md`
- `docs/M1-COMPLETION.md`
- `docs/M0-COMPLETION.md`
- `docs/decisions/ADR_INDEX.md`

## Licence

MIT. See `LICENSE`.
