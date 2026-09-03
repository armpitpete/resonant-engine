# M3 — Breath Pipe Reference Voice Completion

Status: **CLOSURE CANDIDATE — M3.0–M3.13 COMPLETE; PLATFORM/FINAL CLOSURE GATES OPEN**

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
- [x] M3.13 — Human B-Series Acceptance Suite — PASS
- [ ] M3.14 — Portability, Performance & Evidence — exact-head Linux/WASM evidence PASS; hosted platform allocation blocked
- [ ] M3.15 — Hostile Review, Completion & Freeze

## Exact-head automated evidence

The first useful PR CI evidence was run #135 against GitHub's synthetic PR merge commit, so it remains tested-merge evidence rather than raw-head proof.

Dedicated `PR Exact Head` run #3 then passed all five raw-head jobs on `39a8708dda67d7ff3720dfc13ba307d93f780a6e`:

- native Debug;
- native Release;
- ASan + UBSan;
- no-exceptions/no-RTTI portability probe;
- M3 WASM build, exact artifact provenance and native/WASM parity.

The exact artifact recorded both `commit` and `testedCommit` as `39a8708dda67d7ff3720dfc13ba307d93f780a6e` and contained all B01–B18 scenarios.

After the human listener reported low volume, the M3 Lab audition monitor alone was raised in `b7d251aa9082ae2a4226f0dfde44ce8855e47101`. No C++ DSP/model file changed.

`PR Exact Head` run #5 (`33786351281`) then passed all five raw-head jobs on `b7d251aa9082ae2a4226f0dfde44ce8855e47101`:

- native Debug — PASS;
- native Release — PASS;
- ASan + UBSan — PASS;
- no-exceptions/no-RTTI portability probe — PASS;
- M3 WASM build, exact artifact provenance and native/WASM parity — PASS.

This proves the post-monitor head without changing the approved Breath Pipe synthesis implementation.

## Human B01–B18 evidence

The listener ran the complete B01–B18 batch on the exact `39a8708d…` artifact at 48 kHz / 128 frames in Chrome on Windows. All 18 scenarios recorded `AUTOMATED_RESULT: PASS` with no hard failures. The direct human verdict after listening was: **sounds are good; volume is low**.

The low-level complaint was traced to the Lab monitor itself: the inherited control defaulted to `0.6x` and was capped at `0.8x`. The M3 projection now defaults to `2x` audition gain and allows up to `4x`; analyser, WAV capture, telemetry and the Breath Pipe DSP remain before that gain.

The corrected monitor was then re-auditioned against the same sound-approved WASM DSP. The listener's final confirmation was: **Level good, sound still good.**

That closes M3.13 as a human-suite PASS. The acceptance record deliberately preserves the distinction between automated per-scenario evidence and the direct human suite verdict rather than fabricating retrospective per-test JSON verdicts.

- [x] B01 — Silence
- [x] B02 — Faint air
- [x] B03 — Turbulence
- [x] B04 — Pitch emergence
- [x] B05 — Stable pipe
- [x] B06 — Pressure response
- [x] B07 — Damping response
- [x] B08 — Regeneration
- [x] B09 — Self-sustain
- [x] B10 — Overblow
- [x] B11 — Forward/reverse continuum
- [x] B12 — Hysteresis & recovery
- [x] B13 — Expressive performance
- [x] B14 — Pitch & register
- [x] B15 — Aggressive/noise regime
- [x] B16 — Polyphony
- [x] B17 — External excitation
- [x] B18 — Extreme stability

## B08 block-mean / DC evidence disposition

B08 recorded a maximum 128-frame block mean of about `0.05117`, which the inherited M2 telemetry called `scenarioMaxAbsDc` / `scenarioExcessiveDc`. That name is misleading for a low-frequency regenerative signal because 128 frames at 48 kHz cover only 2.67 ms.

The long-window analyser measured about `0.00055` at the regenerative mark and about `0.00003` on recovery, with near-zero final DC. No non-finite output, clipping, protected state or hard failure occurred.

M3 therefore treats the short-window quantity as a **block-mean diagnostic, not a DC-offset failure**. The built M3 projection renames those evidence fields to `outputBlockMean`, `scenarioMaxAbsBlockMean` and `scenarioLargeBlockMean`; the long-window analyser remains the DC-offset diagnostic.

## B14 tuning evidence disposition

The browser's generic decimated autocorrelator returned its 2 kHz ceiling for the C2 and C3 B14 marks. This is not accepted as tuning evidence.

The authoritative M3 tuning gate is `tests/unit/test_breath_pipe_contract.cpp`, which directly measures C2, C3, C4, C5 and C6 at 44.1, 48 and 96 kHz and enforces:

- median absolute tuning error <= 15 cents;
- no measured note > 30 cents error.

That contract passed on exact-head runs #3 and #5. The browser fundamental display remains useful as a diagnostic but is explicitly labelled non-authoritative for the C2–C6 contract.

## M3.14 hosted platform gate

Required final hosted evidence remains:

- Windows MSVC Debug/Release;
- macOS Clang Debug/Release;
- Chromium realtime M3 Lab smoke and CPU budget;
- Firefox realtime M3 Lab smoke and CPU budget;
- Playwright WebKit realtime M3 Lab smoke and CPU budget;
- Microsoft Edge realtime M3 Lab smoke and CPU budget.

Two `M3 Final Platform` attempts requested `ubuntu-latest`, `windows-latest` and `macos-latest`. Every runnable hosted job failed before step 1 with `runner_id: 0`, no runner name and no workflow steps; browser smoke then skipped because its build prerequisite never ran.

This is not a demonstrated Resonant Engine/platform build failure. It is an **external GitHub-hosted runner allocation blocker**. M3.14 remains open until hosted allocation is restored and those jobs actually execute and pass.

No Safari claim is implied by Playwright WebKit.

## Hostile review state

The pre-closure hostile review has found no DSP/architecture blocker. Closure defects H1–H7 have concrete resolutions; H8 is the still-open hosted-runner allocation blocker. A final hostile review is required only after the hosted platform evidence is complete.

## Final gate

M3 cannot be marked FINAL PASS until the canonical completion gate is satisfied, the final exact candidate head is frozen and separately authorised, that exact head merges without undeclared tree changes, post-merge `main` verification passes, and M3 is explicitly frozen.

## Current decision

**The Breath Pipe sound/model and B01–B18 human suite are accepted for M3. Do not add features or retune the synthesis unless a remaining gate demonstrates a specific defect.**
