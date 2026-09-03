# M0.20 — Architecture Decision Register

Status: **APPROVED**

All decisions below are **Accepted for M0**. A later contradiction must supersede the affected ADR explicitly rather than silently changing architecture.

## ADR-0001 — Canonical product architecture
**Decision:** Resonant Engine is a reusable resonant/physical DSP platform, not one synth. **Alternatives:** single finished instrument; host-specific products. **Consequences:** abstractions are judged across multiple resonator families and hosts.

## ADR-0002 — Single shared DSP core
**Decision:** one `resonant_core` serves browser, plugin, embedded and imported-host uses. **Alternatives:** separate DSP ports per host. **Consequences:** host wrappers stay thin; portability constraints are paid in the core once.

## ADR-0003 — C++20
**Decision:** C++20 is the core language baseline. **Alternatives:** C++17 for older toolchains; Rust; C. **Consequences:** `std::span`/concepts are available; targets need a modern compiler.

## ADR-0004 — CMake
**Decision:** CMake 3.20+ is the canonical native build description. **Alternatives:** hand-authored IDE projects, Meson, Bazel. **Consequences:** broad compiler/host support with minimal bootstrap dependency.

## ADR-0005 — Float32 audio path
**Decision:** audio buffers/samples are float32. **Alternatives:** float64 or fixed point as the canonical path. **Consequences:** normal plugin/WASM/embedded fit; algorithms requiring more precision use local accumulators.

## ADR-0006 — Selective double precision
**Decision:** use double for accumulated energy, time/rate conversion and coefficient math where justified. **Alternatives:** all-float or all-double. **Consequences:** better numerical margin without doubling audio-buffer bandwidth.

## ADR-0007 — No JUCE inside core
**Decision:** JUCE may exist only in a Host wrapper. **Alternatives:** JUCE types as public API. **Consequences:** embedded/WASM users are not forced to adopt JUCE.

## ADR-0008 — No Web Audio inside core
**Decision:** DOM/Web Audio/JavaScript assumptions stay in browser Host code. **Alternatives:** AudioWorklet-oriented core API. **Consequences:** browser is first-class without becoming the reference implementation.

## ADR-0009 — Host blocks, sample-level internal feedback
**Decision:** Host buffers are blocks; model feedback executes sample by sample inside those blocks. **Alternatives:** block-level graph feedback. **Consequences:** active resonators do not acquire host-block latency.

## ADR-0010 — Sample-accurate events
**Decision:** events carry a sample offset inside the current block with deterministic ordering. **Alternatives:** block-rate controls only. **Consequences:** pressure/pitch/automation can affect the exact sample required by expressive models.

## ADR-0011 — Parameter smoothing in core/model semantics
**Decision:** None/Linear/OnePole smoothing is sample-rate aware and performed in the shared model path when needed. **Alternatives:** force each Host to smooth. **Consequences:** physical behavior is consistent across hosts.

## ADR-0012 — Deterministic PCG32 PRNG
**Decision:** PCG32 with explicit uint64 seeds and SplitMix64 voice derivation. **Alternatives:** `std::mt19937`, `random_device`, host RNG. **Consequences:** compact deterministic turbulence/noise foundation with no entropy API in real time.

## ADR-0013 — No allocation in real-time processing
**Decision:** allocation/deallocation is prohibited transitively from `process()`. **Alternatives:** allocator pools or best-effort heap use. **Consequences:** storage must be prepared/fixed/caller-owned; latency behavior is predictable.

## ADR-0014 — No locks in real-time processing
**Decision:** blocking synchronization is prohibited from `process()`. **Alternatives:** mutex-protected shared state. **Consequences:** state handover/configuration needs an explicit non-real-time boundary.

## ADR-0015 — Feedback belongs to resonator topology
**Decision:** feedback filtering/nonlinearity/polarity/interaction can occur inside the model loop. **Alternatives:** treat feedback as a Host effect/send. **Consequences:** self-sustaining and Breath Pipe-like systems remain expressible.

## ADR-0016 — Active resonators are first-class
**Decision:** regenerative/self-sustaining/controlled-unstable regimes are legal. **Alternatives:** only passive decays. **Consequences:** stability APIs diagnose numerical failure without suppressing musical instability.

## ADR-0017 — Energy/stability observability
**Decision:** cheap operational energy/finite/runaway diagnostics are part of the architecture. **Alternatives:** rely only on final output clipping. **Consequences:** hosts/tests can see growth and failure while models can remain musically unstable.

## ADR-0018 — Future arbitrary excitation
**Decision:** external audio, microphone/sample streams and resonator output must be usable as excitation without replacing the Engine. **Alternatives:** restrict excitation to built-in noise/impulses or one fixed exciter family. **Consequences:** future sampled, live-input and resonator-as-exciter models fit the same core.

## ADR-0019 — Future resonator coupling
**Decision:** resonators may expose bounded coupling ports and a containing model may route them bidirectionally without changing the Engine contract. **Alternatives:** one-way serial resonator chains or a mandatory graph runtime in M0. **Consequences:** coupled structures remain possible while graph scheduling is deferred until needed.

## ADR-0020 — Browser via WASM/shared core
**Decision:** browser Host compiles/bridges the same core through WASM/AudioWorklet. **Alternatives:** JavaScript/WebAudio reimplementation. **Consequences:** no duplicated synthesis algorithms; wrapper handles browser scheduling/buffers.

## ADR-0021 — VST as Host wrapper
**Decision:** VST3 and optional JUCE integration live in a thin Host wrapper around `resonant_core`. **Alternatives:** plugin-SDK types in the core or a plugin-specific DSP fork. **Consequences:** DAW integration is first-class without defining core semantics.

## ADR-0022 — Embedded hardware is a first-class target
**Decision:** embedded device, DMA, MIDI/USB and control APIs remain outside `resonant_core`, which must stay viable for bounded-memory ARM-class Hosts. **Alternatives:** desktop-first core with a later embedded port or device-specific DSP fork. **Consequences:** embedded constraints influence core boundaries now while board-specific implementation remains later work.

## Contradiction review

No accepted M0 ADR contradicts another: ADR-0002/0007/0008/0020/0021/0022 align dependency direction; ADR-0009/0010/0011 align sample timing; ADR-0013/0014 align real-time constraints; ADR-0015/0016/0017 align feedback/stability; ADR-0018 and ADR-0019 extend excitation/coupling without narrowing the Engine.

**M0.20 ADR set: APPROVED.**

---

# Post-M0 decisions

Later milestone decisions extend the frozen M0 set. They do not silently rewrite an M0 ADR.

## ADR-0023 — M1 first resonator

**File:** `ADR-0023-M1-FIRST-RESONATOR.md`  
**Decision:** M1 uses a deterministic continuously excited noise source, fixed-capacity tuned-delay resonator, active regeneration and bounded nonlinearity as the first concrete musical model while preserving the generic Engine/Host contracts.  
**Scope:** accepted for M1; the selected model is not Breath Pipe and does not make tuned delay universal.

## ADR-0024 — M2 Resonant Engine Lab

**File:** `ADR-0024-M2-RESONANT-ENGINE-LAB.md`  
**Decision:** the browser-facing M2 surface is a diagnostic/human-acceptance Lab around shared C++ DSP, not a browser synth. Test orchestration, measurement and evidence may live outside `resonant_core`; browser-specific synthesis may not.  
**Scope:** accepted for M2; M2 is complete and frozen.

## ADR-0025 — M3 Breath Pipe Reference Voice

**File:** `ADR-0025-M3-BREATH-PIPE-REFERENCE-VOICE.md`  
**Decision:** M3 proves the protected Breath Pipe continuum with one continuously interacting reference model, begins with an evidence-backed topology-selection gate, separates musical macro controls from DSP coefficients, keeps MIDI/Host semantics outside the model, requires explicit generic-primitive promotion, and proves external excitation through the real interaction path.  
**Scope:** M3 implementation candidate; final acceptance remains gated by human/platform/hostile/merge evidence.

## ADR-0026 — M3 Breath Pipe topology selection

**File:** `ADR-0026-M3-BREATH-PIPE-TOPOLOGY.md`  
**Decision:** select the bounded three-mode interacting modal resonator for the M3 Breath Pipe reference voice; retain tuned-delay and scattering/waveguide families as valid future alternatives rather than genericizing the selected topology.  
**Scope:** candidate decision until exact-head selection/contract tests pass; does not become M3 FINAL PASS by itself.
