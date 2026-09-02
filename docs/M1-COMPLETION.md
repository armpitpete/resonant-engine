# M1 — First Resonator Completion Gate

Status: **CANDIDATE ENGINEERING PASS — LIVE PLAY GATE REQUIRED**

M0 is frozen at merged `main` before this milestone. M1 may extend capability but may not silently reinterpret an M0 invariant.

## Milestone definition

- [x] M0 final closure merged and frozen.
- [x] M1 scope and non-goals defined in `docs/M1-FIRST-RESONATOR.md`.
- [x] first-resonator selection criteria recorded.
- [x] tuned-delay/modal/hybrid options compared.
- [x] tuned-delay reference model selected in ADR-0023.
- [x] selected model explicitly remains one concrete `Resonator`, not the universal engine algorithm.

## Concrete resonator

- [x] fixed-capacity `TunedDelayResonator` implemented in `resonant_core`.
- [x] 16,384-sample / 65,536-byte fixed delay state.
- [x] fractional non-integer delay tuning implemented.
- [x] linear interpolation decision documented.
- [x] sample-rate-derived tuning bounds implemented.
- [x] one-pole frequency-dependent damping implemented in-loop.
- [x] passive loop loss implemented.
- [x] active regeneration control implemented separately from passive loss.
- [x] bounded soft nonlinearity implemented in-loop.
- [x] emergency finite-state containment remains separate from musical nonlinearity.
- [x] no Host DSP added.
- [x] no Engine API rewrite required.

## Excitation and expression

- [x] deterministic `ContinuousNoiseExciter` implemented.
- [x] explicit 64-bit seed ownership.
- [x] continuous excitation supported without NoteOn.
- [x] transient Trigger excitation supported.
- [x] NoteOn can act as a bounded transient but is not acoustically required.
- [x] arbitrary external-audio excitation supported.
- [x] returned-resonator interaction path exercised without defining a reed/jet model.
- [x] reset restores deterministic exciter state.
- [x] tuning uses sample-rate-aware smoothing.
- [x] damping, regeneration, nonlinearity, excitation, turbulence and interaction move continuously.
- [x] `Pitch` and `Pressure` retain the existing sample-accurate event route.

## Sonic-state engineering evidence

- [x] exact silence path exists.
- [x] passive transient/ringing path exists.
- [x] continuous deterministic-noise excitation path exists.
- [x] regenerative state exists.
- [x] aggressive nonlinear finite state exists.
- [x] return-to-silence/passive decay is regression-tested.
- [x] deterministic offline render scenes cover passive, continuous, regenerative, nonlinear, sweep and silence states.

Offline WAV fixtures are **regression evidence only**. They do not satisfy the human M1 gate.

## Interactive acceptance host

- [x] browser acceptance host added under `hosts/browser/m1/`.
- [x] host compiles the real C++ `FirstResonatorVoice` to WebAssembly.
- [x] DSP runs in an AudioWorklet rather than on the browser main thread.
- [x] JavaScript contains transport/UI/MIDI only; synthesis is not duplicated outside `resonant_core`.
- [x] computer-keyboard pitch input provided.
- [x] Web MIDI note input provided.
- [x] CC1/mod wheel and channel pressure map to continuous excitation for the play test.
- [x] live excitation, turbulence, damping, regeneration, nonlinearity and interaction controls provided.
- [x] transient trigger and resonator reset provided.
- [x] Emscripten 6.0.6 AudioWorklet/WASM harness build PASS on correction head `e7befa4ea72bba8ee507684d203290e8c54d3bce`.
- [x] playable browser artifact retained by CI as `m1-playable-browser-*`.
- [ ] **human play-test gate PASS:** live playing demonstrates a musically useful resonator materially beyond `ReferenceFeedbackProbe`.

## Determinism

- [x] same-seed deterministic internal noise design.
- [x] reset restores the original seed state.
- [x] same seed/event sequence regression test passes.
- [x] different seed variation test passes.
- [x] block-size-invariant deterministic regression test passes.
- [x] cross-platform policy remains tolerance-conformant rather than bit-identical promise.

## Real-time correctness

- [x] fixed-capacity delay storage.
- [x] no allocation required by the model's sample path.
- [x] dedicated allocation-counting test around `Engine::process()` passes.
- [x] no locks introduced.
- [x] no I/O/filesystem/network dependency introduced into core.
- [x] no unbounded graph/queue work introduced.
- [x] existing M0 failure handling remains authoritative.

## Stability and observability

- [x] real resonator output feeds `EnergyMonitor`.
- [x] excitation/resonator/output/feedback observations wired.
- [x] passive-vs-regenerative late-energy test passes.
- [x] aggressive high-feedback nonlinear 100,000-sample long-run test passes.
- [x] randomized 128,000-sample parameter/event stress passes.
- [x] model rejects/contains numerical non-finite state.
- [x] musically aggressive finite operation remains legal.
- [x] coarse generic `EnergyState` classification limitation documented rather than overclaimed.

## Automated test coverage

- [x] concept compatibility test.
- [x] multi-sample-rate prepare test.
- [x] silence test.
- [x] transient ringing test.
- [x] external-audio excitation test.
- [x] continuous-without-NoteOn test.
- [x] fractional-delay mapping test at 48/96 kHz.
- [x] deterministic same/different seed tests.
- [x] deterministic reset test.
- [x] passive/regenerative comparison.
- [x] aggressive finite 100,000-sample stress.
- [x] 44.1/48/96/192 kHz matrix.
- [x] 1/7/32/63/128-frame block matrix.
- [x] randomized 128,000-sample parameter/event stress.
- [x] zero-allocation real-time process probe.
- [x] deterministic block-invariant regression signature.
- [x] offline render regression verifier.
- [x] original M0 tests remain in the suite.

## Portability/build evidence

GitHub Actions run `33567597513` on correction head `e7befa4ea72bba8ee507684d203290e8c54d3bce` passed all nine jobs:

- [x] Ubuntu/GCC Debug.
- [x] Ubuntu/GCC Release + warnings-as-errors.
- [x] macOS/Clang Debug.
- [x] macOS/Clang Release + warnings-as-errors.
- [x] Windows/MSVC Debug.
- [x] Windows/MSVC Release `/W4 /WX`.
- [x] ASan+UBSan.
- [x] core compile with `-fno-exceptions -fno-rtti`.
- [x] Emscripten AudioWorklet/WASM browser play-test build and artifact verification.
- [x] original M0 tests remain PASS.
- [x] all M1 native tests and deterministic fixture verifiers remain PASS.

This evidence-record commit is documentation-only and must receive its own exact-head CI PASS before PR #4 can leave Draft. The human play gate remains independently required.

## Hostile architecture review

The full attack record is `docs/M1-HOSTILE-ARCHITECTURE-REVIEW.md`.

- [x] Host-specific synthesis DSP attack fails.
- [x] Host-block feedback latency attack fails.
- [x] mandatory NoteOn/oscillator attack fails.
- [x] integer-only tuning attack fails.
- [x] sample-rate-specific tuning attack fails.
- [x] process-time allocation attack fails.
- [x] automatic-failure-for-musical-instability attack fails.
- [x] Host-randomness dependency attack fails.
- [x] Breath-Pipe/Steampipe-specific generic-API leakage attack fails.
- [x] tuned-delay-as-universal-Resonator attack fails.
- [x] external-excitation compatibility remains intact.
- [x] future coupling compatibility remains intact within M1 scope.
- [x] final pitch compensation remains model-local.
- [x] no undocumented M0 invariant violation found.

**Hostile review result: PASS.**

Two real non-blocking weaknesses remain recorded:

1. generic M0 `EnergyState` labels are operational heuristics, not physical/psychoacoustic classifiers;
2. M1 `tuning_hz` is a continuously movable target but not yet fully phase-compensated pitch truth across damping/interpolation settings.

## Protected final gate

M1 is not FINAL PASS until all are true:

- [ ] current exact head receives full CI PASS, including browser/WASM harness build;
- [x] hostile architecture review PASS;
- [x] deterministic regression evidence PASS;
- [ ] interactive human play-test PASS;
- [ ] no unresolved M1 blocker after live play testing;
- [ ] PR moved from Draft to Ready only after all non-protected evidence is current;
- [ ] protected merge authorised and completed;
- [ ] post-merge `main` verification green;
- [ ] M1 declared complete and frozen in a post-merge closure record.

## Current decision

**ENGINEERING CANDIDATE PASS, HUMAN GATE OPEN.** M1 is not accepted by listening to WAV files. It is accepted by playing the live C++ resonator through the browser host. The protected merge remains blocked until that interactive gate passes.
