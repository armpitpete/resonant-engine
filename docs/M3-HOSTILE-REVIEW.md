# M3 — Pre-Closure Hostile Review

Status: **PRE-CLOSURE REVIEW — FINAL REVIEW STILL REQUIRED**

Candidate reviewed: PR #7 head `714df0b758ff1f92f1590c69169c293b3d30bcea`.

This review is deliberately adversarial. It does not declare M3 complete and does not substitute for B01–B18 human acceptance or the final platform gates.

## Findings

### No blocker found — architecture boundaries

- Breath-Pipe-specific controls and state remain inside `BreathPipeVoice` / Breath Pipe Lab code rather than changing generic `Engine` semantics.
- MIDI-note and velocity interpretation remains in the Lab/Host adapter. The model receives pitch, pressure and model parameters rather than MIDI concepts.
- No JUCE, Web Audio, browser or device dependency enters `resonant_core`.
- The selected modal topology remains explicitly reference-voice-specific rather than promoted into a generic engine primitive.

### No blocker found — energetic model integrity

- Zero pressure with no trigger/external input produces no stochastic excitation because the noise term is pressure-scaled.
- Returned resonator state perturbs the exciter before the nonlinear jet stage; the model is not merely `noise -> filter`.
- Resonant modes are damped stateful poles that remain silent from reset until energy is supplied; there is no free-running hidden oscillator used to fake pitch onset.
- High-pressure overblow continuously redistributes modal radii/energy rather than switching to a second voice or preset engine.
- Nonlinear drive alters the modal recursion itself; output shaping is not the sole nonlinear mechanism.
- External audio enters the exciter/resonator interaction path and is not mixed directly to output as a disguised effect bypass.

### No blocker found — boundedness / recovery structure

- The model uses fixed-size state, bounded three-mode work and explicit finite-value containment.
- Numerical failure propagates to the existing protected/failure path rather than being silently labelled musical instability.
- Per-voice seeds remain deterministic and independent through the Lab allocator.

## Closure blockers found

### H1 — CI provenance was not strict raw-head evidence

CI run #135 was associated with PR head `714df0b758ff1f92f1590c69169c293b3d30bcea`, but GitHub's default `pull_request` checkout built synthetic merge commit `cac23e55fac5b9e8b1375ec1333f380d669b849e`.

That is useful tested-merge evidence, but the M3 contract explicitly requires an exact candidate head. It must not be described as raw-head proof.

Resolution: add a dedicated `PR Exact Head` workflow that checks out `github.event.pull_request.head.sha`, asserts `git rev-parse HEAD` equals that SHA, and reruns the native, sanitizer, portability and M3 native/WASM parity gates.

### H2 — completion ledger materially contradicted repository state

`docs/M3-COMPLETION.md` still said `PLANNED — 0/16 SECTIONS COMPLETE` and `implementation has not begun` even though the PR contains the Breath Pipe implementation, tests, Lab adapter and acceptance suite.

Resolution: reconcile the ledger to implementation-candidate state while leaving M3.13–M3.15 open.

## Gates intentionally still open

- B01–B18 human listening/interaction acceptance;
- Windows MSVC Debug/Release;
- macOS Clang Debug/Release;
- Chromium, Firefox, Playwright WebKit and Microsoft Edge realtime M3 Lab smoke/CPU gates;
- final hostile review after all acceptance-driven changes;
- exact final-head merge authorisation;
- merge and post-merge `main` proof;
- FINAL PASS documentation and freeze.

## Final-review rule

Repeat the hostile review on the final frozen candidate head after human and platform acceptance. Any code or contract change made in response to those gates invalidates this review as the final M3.15 review.
