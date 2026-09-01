# M1 — First Resonator Completion Gate

Status: **CANDIDATE ENGINEERING PASS — HUMAN LISTENING GATE REQUIRED**

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

## Excitation

- [x] deterministic `ContinuousNoiseExciter` implemented.
- [x] explicit 64-bit seed ownership.
- [x] continuous excitation supported without NoteOn.
- [x] transient Trigger excitation supported.
- [x] NoteOn can act as a bounded transient but is not acoustically required.
- [x] arbitrary external-audio excitation supported.
- [x] returned-resonator interaction path exercised without defining a reed/jet model.
- [x] reset restores deterministic exciter state.

## Movement and expression

- [x] tuning moves through existing sample-rate-aware smoother.
- [x] damping moves sample-continuously.
- [x] regeneration/feedback moves sample-continuously.
- [x] nonlinearity moves sample-continuously.
- [x] excitation/turbulence/interaction move sample-continuously.
- [x] `Pitch` and `Pressure` use the existing sample-accurate event path.
- [x] deterministic sweep fixture contains a sample-accurate mid-render pitch event.

## Sonic-state evidence

- [x] exact silence path exists.
- [x] passive transient/ringing path exists.
- [x] continuous deterministic-noise excitation path exists.
- [x] regenerative state exists.
- [x] aggressive nonlinear finite state exists.
- [x] return-to-silence/passive decay demonstrated by fixture verification.
- [x] six canonical M1 render scenes defined and retained as a CI artifact.
- [ ] human listening gate: fixtures judged musically useful and materially beyond `ReferenceFeedbackProbe`.

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

## Test coverage and deterministic fixtures

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
- [x] six one-second PCM16 render fixtures.
- [x] M1 WAV verifier.
- [x] all M0 and M1 test targets wired and passing on implementation head `27f458d0b2530314378666a208064f5f376de4ed`.

## Portability/build evidence

GitHub Actions run `33566199233` on exact implementation head `27f458d0b2530314378666a208064f5f376de4ed`:

- [x] Ubuntu/GCC Debug PASS.
- [x] Ubuntu/GCC Release + warnings-as-errors PASS.
- [x] macOS/Clang Debug PASS.
- [x] macOS/Clang Release + warnings-as-errors PASS.
- [x] Windows/MSVC Debug PASS.
- [x] Windows/MSVC Release `/W4 /WX` PASS.
- [x] ASan+UBSan PASS.
- [x] core compile with `-fno-exceptions -fno-rtti` PASS.
- [x] original M0 tests remain PASS.
- [x] all M1 tests and fixture verifiers PASS.
- [x] Release listening-fixture artifact retained successfully.

Documentation commits after that implementation head must receive their own final exact-head CI before the PR can leave Draft.

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

Two real non-blocking weaknesses are recorded rather than hidden:

1. generic M0 `EnergyState` labels are operational heuristics, not physical/psychoacoustic classifiers;
2. M1 `tuning_hz` is a continuously movable target but not yet fully phase-compensated pitch truth across damping/interpolation settings.

## Documentation

- [x] M1 milestone contract added.
- [x] ADR-0023 added.
- [x] hostile architecture review added.
- [x] M1 research findings added without modifying frozen M0.19.
- [x] signal flow documented.
- [x] parameter IDs/ranges/defaults documented.
- [x] fractional-delay choice and limitations documented.
- [x] damping/feedback/nonlinearity documented.
- [x] fixed memory bound documented.
- [x] determinism documented.
- [x] limitations/non-goals documented.
- [x] oversampling explicitly deferred pending evidence.
- [x] README updated to M1 candidate state without claiming final completion.

## Protected final gate

M1 is not FINAL PASS until all are true:

- [ ] final documentation head receives exact-head CI PASS;
- [x] hostile architecture review PASS;
- [x] deterministic fixture verification PASS;
- [ ] human listening gate PASS;
- [ ] no unresolved M1 blocker after human listening;
- [ ] PR moved from Draft to Ready only after all non-protected evidence is current;
- [ ] protected merge authorised and completed;
- [ ] post-merge `main` verification green;
- [ ] M1 declared complete and frozen in a post-merge closure record.

## Current decision

**ENGINEERING CANDIDATE PASS.** The code, automated tests, portability matrix, deterministic fixtures and hostile architecture gate pass. M1 is deliberately **not** declared complete because musical usefulness is a human perceptual claim and the protected merge has not occurred.
