# M0.22 — M0 Completion Gate

Status: **CANDIDATE PASS — LOCAL EVIDENCE COMPLETE; GITHUB CI PENDING**

This gate is evaluated against the canonical 22-section task list. Do not change this status to final PASS until the completion branch CI passes on its exact head.

## Documentation

- [x] Product vision canonical — README + `PRODUCT_SCOPE.md`.
- [x] M0 milestone statement canonical.
- [x] Scope/non-goals documented.
- [x] Sonic Design Contract complete — `AIR_PIPE_NOISE.md`.
- [x] Terminology complete — `GLOSSARY.md`.
- [x] Core boundary complete — `CORE_BOUNDARY.md`.
- [x] Real-time contract complete — `REALTIME.md`.
- [x] Processing contract complete — `PROCESSING.md`.
- [x] Event contract complete — `EVENTS.md`.
- [x] Parameter contract complete — `PARAMETERS.md`.
- [x] Exciter/resonator contracts complete — `EXCITER_RESONATOR.md`.
- [x] Feedback contract complete — `FEEDBACK.md`.
- [x] Energy/stability contract complete — `ENERGY_STABILITY.md`.
- [x] Determinism contract complete — `DETERMINISM.md`.
- [x] Lifecycle contract complete — `LIFECYCLE.md`.
- [x] Research record complete — `docs/research/INDEX.md`.
- [x] ADR set complete — `docs/decisions/ADR_INDEX.md`.

## Build

Local Linux/GCC evidence on the completion worktree:

- [x] clean configure succeeds;
- [x] `resonant_core` builds;
- [x] `resonant_tests` builds;
- [x] `resonant_render` builds;
- [x] Debug build: 8/8 CTest tests PASS;
- [x] Release build: 8/8 CTest tests PASS with warnings-as-errors;
- [ ] GitHub CI passes on exact branch head — pending branch upload/CI;
- [x] local ASan+UBSan Debug run: 8/8 PASS;
- [x] no-exceptions/no-RTTI core translation-unit compile probe passes.

## Runtime foundation

- [x] Engine constructs.
- [x] Engine prepares.
- [x] Engine processes silence.
- [x] Engine resets and destroys by normal RAII.
- [x] Events schedule sample accurately, including sample 0/final sample.
- [x] Parameter smoothing works and is sample-rate aware.
- [x] Deterministic RNG works with fixed/derived voice seeds.
- [x] Processing stays finite across tested sample rates/block sizes.
- [x] demonstrated `Engine::process()` path performs zero dynamic allocations.

## Offline render/testing foundation

- [x] deterministic offline host accepts sample rate, block size, duration and seed;
- [x] it can render the zero-excitation placeholder path;
- [x] it writes PCM16 mono WAV outside the core;
- [x] canonical render fixture exists;
- [x] WAV structure validation passes;
- [x] unit/property/regression directories and targets exist;
- [x] test seed/tolerance/fixture policies are documented.

## Portability

- [x] no browser-specific DSP;
- [x] no VST/JUCE-specific DSP;
- [x] no embedded/device-specific DSP;
- [x] WASM/AudioWorklet architecture review passes;
- [x] VST3/JUCE wrapper architecture review passes;
- [x] embedded ARM architecture review passes;
- [x] no known architectural rewrite is required for those targets;
- [x] OS/filesystem/thread/SIMD/endianness/alignment/pointer-width/floating-point/RTTI/exception/heap assumptions audited.

## Future synthesis capability

- [x] active and passive resonators fit;
- [x] continuous and transient excitation fit;
- [x] arbitrary external-audio excitation fits;
- [x] feedback filtering/nonlinearity fit inside topology;
- [x] energy/stability observation fits;
- [x] future resonator coupling/graphs fit without replacing Engine interfaces;
- [x] deliberately impossible synthetic bodies are not excluded by a physical-realism requirement.

## Special gate

- [x] final Breath Pipe Reference Voice hostile architecture review PASS;
- [x] no architectural hack required to express it;
- [x] Host/DSP separation confirmed;
- [x] blocking weakness found during final local hostile review (unsafe invalid-block clearing) was fixed and retested;
- [ ] no unresolved M0 blocker — contingent only on exact-head GitHub CI;
- [x] final hostile architecture review completed locally;
- [ ] M0 declared complete — pending exact-head CI and protected merge;
- [ ] M1 authorized to start — follows final M0 merge gate.

## Gate decision

**Candidate PASS.** The architecture/runtime/docs satisfy M0 locally. The only remaining evidence step is exact-head multi-platform GitHub CI, followed by the protected merge gate. If that CI remains green and hostile PR review finds no new blocker, M0.22 may be changed to final PASS without adding scope.
