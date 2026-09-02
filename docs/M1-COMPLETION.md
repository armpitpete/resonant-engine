# M1 — First Resonator Completion Gate

Status: **FINAL PASS — MERGED AND FROZEN**

M1 added the first concrete musical resonator while preserving the frozen M0 Engine/Host architecture.

## Accepted implementation

- [x] `ContinuousNoiseExciter` provides deterministic continuous noise, transient and arbitrary external-audio excitation.
- [x] `TunedDelayResonator` provides fixed-memory fractional-delay resonance with damping, passive loss, active regeneration and bounded in-loop nonlinearity.
- [x] `FirstResonatorVoice` composes the primitives inside `resonant_core` without Host DSP or Engine API changes.
- [x] exact silence, passive ringing, continuous excitation, regenerative and aggressive nonlinear finite states are covered.
- [x] deterministic reset, same/different seed, block-invariant and multi-sample-rate behaviour are tested.
- [x] no process-time allocation, locks, filesystem, network or unbounded work were introduced into the core path.
- [x] real resonator output feeds the existing energy/stability observation path.

## Automated evidence

Exact M1 candidate head:

`910616d0396ab516fa0b3272fe3067c23bffacb6`

GitHub Actions run `33567863832` (CI #60): **PASS**.

The exact candidate passed the compiler/platform matrix, Debug/Release builds, warnings-as-errors gates, ASan+UBSan, no-exceptions/no-RTTI portability probe, deterministic regression tests and Emscripten AudioWorklet/WASM browser harness build.

Offline WAV renders remain regression evidence only; they were not used as the human acceptance gate.

## Human acceptance

- [x] live human interaction/listening PASS.
- [x] the accepted M2 Resonant Engine Lab exercised the same M1 synthesis implementation.
- [x] `core/include/resonant/FirstResonator.hpp` has blob SHA `21eb70b37cc1ba3c699c84e6b5eaa80dfae5ff91` at both the M1 exact head and the accepted M2 candidate used for A01–A09 testing.
- [x] A01–A09 human acceptance passed after an A01 Lab audibility-calibration defect was corrected; no M1 core change was required.
- [x] no unresolved M1 blocker remained after live testing.

## Hostile architecture review

`docs/M1-HOSTILE-ARCHITECTURE-REVIEW.md`: **PASS**.

The review confirmed that M1 did not introduce Host-specific synthesis DSP, host-block feedback latency, mandatory NoteOn semantics, integer-only tuning, sample-rate-specific tuning, process-time allocation, automatic failure for musically aggressive finite states, Host randomness dependence, Breath Pipe/Steampipe-specific generic API leakage, or a claim that tuned delay is the universal Resonant Engine model.

Two recorded non-blocking limitations remain valid:

1. generic M0 `EnergyState` labels are operational heuristics rather than physical or psychoacoustic classifiers;
2. M1 `tuning_hz` is continuously movable but not yet fully phase-compensated pitch truth across all damping/interpolation settings.

## Merge and post-merge verification

PR #4 was merged at the exact authorised head:

`910616d0396ab516fa0b3272fe3067c23bffacb6`

Merge commit on `main`:

`54ebc45e4a0b2c96a7733ce99f49b3855acad9a7`

Post-merge verification established:

- [x] `main` points at the M1 merge commit.
- [x] the merge commit tree SHA is `fc944453ab026448b1e633ccb1c658d7611482c1`.
- [x] the exact green M1 head has the same tree SHA `fc944453ab026448b1e633ccb1c658d7611482c1`.
- [x] therefore CI #60 validates the exact repository tree merged to `main`; the merge changed history metadata, not tree contents.
- [x] protected-authority merge gate was explicitly satisfied.

## Frozen boundary

M0 invariants and the Host/core boundary remain authoritative. The selected tuned-delay resonator is one concrete Resonant Engine model; it is not Breath Pipe, not a Steampipe clone and not a requirement that future models use tuned delays.

## Final decision

**M1 FINAL PASS. M1 is complete and frozen.**
