# Resonant Engine — M0 Foundation Status

The canonical M0 plan contains 22 sections. M0.1–M0.4 and the early M0.21 gate were merged in PR #1. The completion branch executes the remaining sections and repeats M0.21/M0.22 against the resulting architecture.

| Section | Canonical title | Status on completion branch |
|---|---|---|
| M0.1 | Product & Scope Contract | APPROVED / merged previously |
| M0.2 | Sonic Design Contract | APPROVED / merged previously |
| M0.3 | Core Terminology & Concept Model | APPROVED / merged previously |
| M0.4 | Portable DSP Core Boundary | APPROVED / merged previously |
| M0.5 | Real-Time Processing Contract | APPROVED |
| M0.6 | Audio Processing Model | APPROVED |
| M0.7 | Event & Timing Contract | APPROVED |
| M0.8 | Parameter Contract | APPROVED |
| M0.9 | Exciter & Resonator Interfaces | APPROVED |
| M0.10 | Feedback Path Contract | APPROVED |
| M0.11 | Energy & Stability Model | APPROVED |
| M0.12 | Determinism & Randomness Contract | APPROVED |
| M0.13 | State & Lifecycle Contract | APPROVED |
| M0.14 | Build System & Toolchain | APPROVED |
| M0.15 | Offline Render Host | APPROVED |
| M0.16 | Testing Foundation | APPROVED |
| M0.17 | Cross-Platform Portability Proof | APPROVED FOR M0 |
| M0.18 | Repository Structure | APPROVED |
| M0.19 | Research & Reference Record | APPROVED |
| M0.20 | Architecture Decision Records | APPROVED |
| M0.21 | Breath Pipe Architecture Review | FINAL PASS |
| M0.22 | M0 Completion Gate | CANDIDATE PASS — exact-head GitHub CI/merge pending |

## Runtime evidence currently demonstrated locally

- C++20 `resonant_core` with no Host framework dependency;
- float32, non-interleaved block interface, max 2 channels / 4096 frames;
- sample-level model processing inside Host blocks;
- fixed-capacity, deterministically ordered sample-offset events;
- parameter range conversion and sample-rate-aware smoothing;
- deterministic PCG32 + voice seed derivation;
- fixed-memory energy/runaway observation;
- feedback stage with filter/nonlinearity/polarity/envelope insertion semantics;
- lifecycle/invalid-context failure behavior;
- deterministic offline PCM16 WAV host;
- unit/property/regression/WAV/dependency tests;
- zero allocation detected in demonstrated real-time process path;
- Debug, Release and ASan+UBSan local tests all 8/8 PASS;
- core compile probe passes with `-fno-exceptions -fno-rtti`.

See `docs/M0-COMPLETION.md` for the final evidence gate.
