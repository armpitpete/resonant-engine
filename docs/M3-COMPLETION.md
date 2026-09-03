# M3 — Breath Pipe Reference Voice Completion

Status: **IMPLEMENTATION CANDIDATE — 13/16 SECTIONS IMPLEMENTED; CLOSURE GATES OPEN**

Canonical milestone contract: `docs/M3-BREATH-PIPE.md`.

## Section status

- [x] M3.0 — Milestone Contract, Boundaries & Operating Envelope
- [x] M3.1 — Breath Pipe Model / Topology Selection
- [x] M3.2 — Pressure & Turbulence Excitation
- [x] M3.3 — Breath-Pipe Resonator
- [x] M3.4 — Bidirectional Exciter ↔ Resonator Interaction
- [x] M3.5 — Regeneration & Active Feedback
- [x] M3.6 — Nonlinearity & Feedback Spectral Shaping
- [x] M3.7 — Musical Macro Controls & Expressive Mapping
- [x] M3.8 — Regime Transitions, Hysteresis & Overblow
- [x] M3.9 — Pitch & Register Behaviour
- [x] M3.10 — Stability, Extremes & Recovery
- [x] M3.11 — Polyphony, Voice Independence & CPU Scaling
- [x] M3.12 — External Excitation Proof
- [ ] M3.13 — Human B-Series Acceptance Suite
- [ ] M3.14 — Portability, Performance & Evidence
- [ ] M3.15 — Hostile Review, Completion & Freeze

## Automated evidence to date

PR #7 head `714df0b758ff1f92f1590c69169c293b3d30bcea` produced CI run #135 with successful Linux ARM64 Debug/Release builds, 33/33 Release tests, sanitizer coverage, the no-exceptions/no-RTTI probe, M3 WASM build and native/WASM parity.

That run checked out GitHub's synthetic PR merge commit `cac23e55fac5b9e8b1375ec1333f380d669b849e`, not the raw PR head. It is therefore valid tested-merge evidence but is not being relabelled as exact-head evidence.

A dedicated `PR Exact Head` workflow now checks out `github.event.pull_request.head.sha`, verifies `git rev-parse HEAD` against that SHA, runs native Debug/Release, sanitizers, the portability probe and M3 native/WASM parity, and uploads the M3 Lab artifact under the actual source SHA. M3.14 remains open until that workflow passes on the final candidate head and the deliberate hosted platform/browser gates also pass.

## Human acceptance

B01–B18 executable scenarios and presets exist, but automated scenario support does not substitute for the canonical human listening/interaction gate.

- [ ] B01 — Silence
- [ ] B02 — Faint air
- [ ] B03 — Turbulence
- [ ] B04 — Pitch emergence
- [ ] B05 — Stable pipe
- [ ] B06 — Pressure response
- [ ] B07 — Damping response
- [ ] B08 — Regeneration
- [ ] B09 — Self-sustain
- [ ] B10 — Overblow
- [ ] B11 — Forward/reverse continuum
- [ ] B12 — Hysteresis & recovery
- [ ] B13 — Expressive performance
- [ ] B14 — Pitch & register
- [ ] B15 — Aggressive/noise regime
- [ ] B16 — Polyphony
- [ ] B17 — External excitation
- [ ] B18 — Extreme stability

## Remaining M3.14 platform gates

The following remain deliberate manual hosted gates and have not yet been credited as passed for the final candidate:

- Windows MSVC Debug/Release;
- macOS Clang Debug/Release;
- Chromium realtime M3 Lab smoke and CPU budget;
- Firefox realtime M3 Lab smoke and CPU budget;
- Playwright WebKit realtime M3 Lab smoke and CPU budget;
- Microsoft Edge realtime M3 Lab smoke and CPU budget.

No Safari claim is implied by Playwright WebKit.

## Hostile review state

A pre-closure hostile review found no current blocker in the model architecture itself: Breath-Pipe concepts remain model-local, MIDI remains Host-side, excitation enters the resonant interaction path, overblow is modal reorganisation rather than a hidden substitute voice, and loop nonlinearity affects resonator state.

Two closure defects were found:

1. CI #135 was synthetic-merge evidence rather than strict raw-head evidence.
2. This completion ledger was stale and incorrectly still reported `PLANNED — 0/16` / `implementation has not begun`.

Both are being corrected in the implementation-candidate branch. The final hostile review remains open until human acceptance, hosted platform gates and any resulting fixes are complete.

## Final gate

M3 cannot be marked FINAL PASS until the canonical contract's completion gate is satisfied, the final exact head is independently authorised and merged, post-merge `main` verification passes, and the milestone is explicitly frozen.

## Current decision

**The Breath Pipe implementation is an acceptance candidate, not a completed milestone.** The immediate gates are exact-head automated proof, B01–B18 human acceptance, the deliberate Windows/macOS/browser platform run, final hostile review, exact-head merge authorisation, merge, post-merge reconciliation and freeze.
