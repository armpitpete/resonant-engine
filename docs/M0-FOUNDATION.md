# M0 Foundation Execution Status

This status file is aligned to the canonical 22-section M0 task list supplied for Resonant Engine.

## Current bounded slice

| Section | Canonical title | Branch status |
|---|---|---|
| M0.1 | Product & Scope Contract | **APPROVED** |
| M0.2 | Sonic Design Contract | **APPROVED** |
| M0.3 | Core Terminology & Concept Model | **APPROVED** |
| M0.4 | Portable DSP Core Boundary | **APPROVED** |
| M0.21 | Breath Pipe Architecture Review | **EARLY PASS — final review still required later** |

## M0.1 evidence

`docs/architecture/PRODUCT_SCOPE.md` now defines:

- canonical product vision and M0 statement;
- what Resonant Engine is and is not;
- browser, VST3/DAW, embedded, imported-synth and future Geophony use;
- the single-shared-core rule and prohibition on host-specific DSP duplication;
- M0 boundaries and non-goals;
- post-M0/M1 relationship;
- MIT as the initial licence;
- Semantic Versioning and pre-1.0 break policy;
- C++20 and initial compiler/toolchain baseline;
- M0 acceptance basis;
- explicit anti-Steampipe-clone scope review.

`docs/architecture/INVARIANTS.md` is the canonical invariant set.

**M0.1: complete on branch.**

## M0.2 evidence

`docs/architecture/AIR_PIPE_NOISE.md` defines:

- air as energy/excitation rather than a noise overlay;
- pressure and turbulence;
- continuous and transient excitation;
- excitation/resonator interaction;
- passive, regenerative, self-sustaining and unstable resonance;
- overblow;
- noise-to-pitch and pitch-to-noise transition behavior;
- physically plausible and deliberately impossible behavior;
- transition behavior as more important than static preset snapshots;
- the protected Breath Pipe Reference Voice and signal path;
- required future expressive controls and continuous behavior;
- sonic acceptance vocabulary;
- realism as optional;
- the requirement that a technically good model must also become a good instrument.

**M0.2: complete on branch.**

## M0.3 evidence

`docs/architecture/GLOSSARY.md` defines every canonical term listed by M0.3 and explicitly resolves common confusions including Engine/Host, Exciter/Excitation, Pressure/expression event, Turbulence/noise, Resonator/oscillator, Loss/Damping, Nonlinearity/Saturation, musical/numerical instability, Movement/Modulation, State/Preset, Processing Block/feedback interval, Control Rate/sample accuracy and Determinism/bit identity.

The glossary is normative for implementation.

**M0.3: complete on branch.**

## M0.4 evidence

`docs/architecture/CORE_BOUNDARY.md` defines:

- responsibilities inside/outside `resonant_core`;
- browser/Web Audio/JavaScript/JUCE/VST/DAW/USB/MIDI-device/GUI/OS/filesystem dependency prohibitions;
- the core-facing Host boundary;
- conceptual audio-buffer, event-input, parameter, external-audio, diagnostics and lifecycle interfaces;
- dependency direction;
- thin-wrapper criterion;
- browser, VST3 and embedded architecture reviews.

CMake now exposes the shared target as `resonant_core` with C++20. `cmake/check_core_boundary.cmake` adds an automated dependency-boundary guard for current core headers.

Detailed buffer/event/parameter shapes remain intentionally deferred to M0.6–M0.8 so M0.4 does not freeze them prematurely.

**M0.4: complete on branch.**

## Architectural probe

`include/resonant/Engine.hpp` and `ReferenceFeedbackProbe.hpp` remain **probes**, not the final M0 API. Their purpose is to prove that Host block processing can contain model-owned sample-by-sample closed-loop behavior.

The probe must not be allowed to silently decide later event, parameter, channel, graph or exciter/resonator interfaces before their canonical M0 sections execute.

## Early M0.21

The Breath Pipe review has been repeated against the canonical M0.1–M0.4 contracts. Its detailed walk, hostile review, weaknesses and early-pass decision are in `docs/M0.21-BREATH-PIPE-ARCHITECTURE-REVIEW.md`.

This is **not** the final M0.21 gate. The canonical execution order requires another hostile Breath Pipe review after M0.5–M0.20 have established the full architecture.

## Next canonical execution slice

Proceed to **M0.5–M0.13** in order:

1. M0.5 — Real-Time Processing Contract
2. M0.6 — Audio Processing Model
3. M0.7 — Event & Timing Contract
4. M0.8 — Parameter Contract
5. M0.9 — Exciter & Resonator Interfaces
6. M0.10 — Feedback Path Contract
7. M0.11 — Energy & Stability Model
8. M0.12 — Determinism & Randomness Contract
9. M0.13 — State & Lifecycle Contract

M0.20 follows that slice to freeze the major decisions before build/repository/test/render/portability work hardens them further.
