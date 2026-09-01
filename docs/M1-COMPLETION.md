# M1 — First Resonator Completion Gate

Status: **CANDIDATE — IMPLEMENTATION AND EVIDENCE IN PROGRESS**

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
- [x] return-to-silence/passive decay is testable.
- [x] six canonical M1 render scenes defined.
- [ ] human listening gate: fixtures judged musically useful and materially beyond `ReferenceFeedbackProbe`.

## Determinism

- [x] same-seed deterministic internal noise design.
- [x] reset restores the original seed state.
- [x] same seed/event sequence regression test added.
- [x] different seed variation test added.
- [x] block-size-invariant deterministic regression test added.
- [x] cross-platform policy remains tolerance-conformant rather than bit-identical promise.

## Real-time correctness

- [x] fixed-capacity delay storage.
- [x] no allocation required by the model's sample path.
- [x] dedicated allocation-counting test added around `Engine::process()`.
- [x] no locks introduced.
- [x] no I/O/filesystem/network dependency introduced into core.
- [x] no unbounded graph/queue work introduced.
- [x] existing M0 failure handling remains authoritative.

## Stability and observability

- [x] real resonator output feeds `EnergyMonitor`.
- [x] excitation/resonator/output/feedback observations wired.
- [x] passive-vs-regenerative late-energy test added.
- [x] aggressive high-feedback nonlinear long-run test added.
- [x] model rejects/contains numerical non-finite state.
- [x] musically aggressive finite operation remains legal.

## Test coverage added

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
- [x] deterministic regression signature source added.
- [x] six one-second PCM16 render fixtures added.
- [x] M1 WAV verifier added.
- [ ] all new test targets wired and passing on exact candidate head.

## Portability/build gate

Required on the exact candidate head:

- [ ] GCC Debug PASS.
- [ ] GCC Release/warnings-as-errors PASS.
- [ ] Clang/macOS Debug PASS.
- [ ] Clang/macOS Release/warnings-as-errors PASS.
- [ ] MSVC Debug PASS.
- [ ] MSVC Release `/W4 /WX` PASS.
- [ ] ASan+UBSan PASS.
- [ ] core compile with `-fno-exceptions -fno-rtti` PASS.
- [ ] original M0 tests remain PASS.
- [ ] all M1 tests PASS.

## Hostile architecture review

Before final PASS, explicitly attempt to force:

- [ ] Host-specific synthesis DSP.
- [ ] Host-block feedback latency.
- [ ] mandatory NoteOn/oscillator semantics.
- [ ] integer-only tuning.
- [ ] sample-rate-specific tuning assumptions.
- [ ] process-time allocation.
- [ ] instability treated as automatic error.
- [ ] deterministic noise delegated to Host randomness.
- [ ] Breath-Pipe/Steampipe-specific semantics into generic core APIs.
- [ ] tuned-delay implementation promoted to universal Resonator semantics.
- [ ] future external excitation/coupling made impossible.

No hostile item may require an undocumented M0 invariant violation.

## Documentation

- [x] M1 milestone contract added.
- [x] ADR-0023 added.
- [x] signal flow documented.
- [x] parameter IDs/ranges/defaults documented.
- [x] fractional-delay choice documented.
- [x] damping/feedback/nonlinearity documented.
- [x] fixed memory bound documented.
- [x] determinism documented.
- [x] limitations/non-goals documented.
- [x] oversampling explicitly deferred pending evidence.
- [ ] README updated after candidate behavior is CI-proven.
- [ ] research record updated with M1 findings after CI/hostile review.

## Protected final gate

M1 is not FINAL PASS until all are true:

- [ ] exact-head CI green;
- [ ] hostile architecture review PASS;
- [ ] deterministic fixture verification PASS;
- [ ] human listening gate PASS;
- [ ] no unresolved M1 blocker;
- [ ] PR moved from Draft to Ready only after the above engineering evidence is current;
- [ ] protected merge authorised and completed;
- [ ] post-merge `main` verification green;
- [ ] M1 declared complete and frozen.

## Current decision

**CANDIDATE ONLY.** The implementation is intentionally not declared complete while exact-head CI, hostile review, listening evidence and the protected merge gate remain unresolved.
