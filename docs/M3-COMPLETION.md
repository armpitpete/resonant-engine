# M3 — Breath Pipe Reference Voice Completion

Status: **FINAL PASS — MERGED AND FROZEN**

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
- [x] M3.14 — Portability, Performance & Evidence — PASS
- [x] M3.15 — Hostile Review, Completion & Freeze — PASS

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

The final implementation candidate head `fb0375bf6e8899c3b3d4f6c8221d8ce3d150d936` then passed:

- `PR Exact Head` run #6 (`33791246658`) — all five raw-head jobs PASS;
- CI #141 (`33791246636`) — PASS;
- native Debug and Release;
- ASan + UBSan;
- no-exceptions/no-RTTI portability probe;
- M3 WASM build, exact artifact provenance and native/WASM parity.

This establishes `fb0375bf6e8899c3b3d4f6c8221d8ce3d150d936` as the accepted exact implementation head before documentation-only closure reconciliation.

The final documentation-only closure head `f38d0d7bce6b1fa8471dd1e96a1ac91783b0d2fa` then passed all required exact-head validation:

- CI #142 (`33881473150`) — PASS;
- `PR Exact Head` run #7 (`33881473178`) — PASS;
- `M3 Final Platform` run #4 (`33881473140`) — PASS;
- Windows MSVC Debug/Release — PASS;
- macOS Clang Debug/Release — PASS;
- Chromium, Firefox, Playwright WebKit and Microsoft Edge realtime smoke + CPU gates — PASS;
- native Debug/Release, ASan+UBSan, no-exceptions/no-RTTI and native/WASM parity — PASS.

The diff from the accepted implementation head to the final closure head changed only `README.md`, `docs/M3-COMPLETION.md` and `docs/M3-HOSTILE-REVIEW.md`.

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

**PASS.**

The earlier zero-step hosted failures were runner-allocation failures rather than Resonant Engine failures. After hosted Actions capacity was restored, `M3 Final Platform` run #3 (`33791246542`) was rerun against exact implementation head `fb0375bf6e8899c3b3d4f6c8221d8ce3d150d936` and completed successfully.

Final hosted evidence:

- Windows MSVC Debug — PASS;
- Windows MSVC Release — PASS;
- macOS Clang Debug — PASS;
- macOS Clang Release — PASS;
- exact-head M3 browser build, artifact provenance and native/WASM parity — PASS;
- Chromium realtime M3 Lab smoke and CPU budget — PASS;
- Firefox realtime M3 Lab smoke and CPU budget — PASS;
- Playwright WebKit realtime M3 Lab smoke and CPU budget — PASS;
- Microsoft Edge realtime M3 Lab smoke and CPU budget — PASS.

No Safari claim is implied by Playwright WebKit.

## Hostile review state

**FINAL PRE-MERGE HOSTILE REVIEW: PASS** on implementation head `fb0375bf6e8899c3b3d4f6c8221d8ce3d150d936`.

No DSP, architecture, realtime, provenance, human-acceptance or platform blocker remains. H1–H8 are resolved. The subsequent closure changes were documentation-only and the exact closure head passed fresh CI, exact-head and final-platform validation before merge authorisation.

## Merge and post-merge verification

PR #7 was merged at the exact authorised head:

`f38d0d7bce6b1fa8471dd1e96a1ac91783b0d2fa`

Merge commit on `main`:

`a0bf3540f164c3466eee9592a493fb4bc2902d5d`

Merged tree SHA:

`594f62b5a6ba15725745d00d9b7cd32384549f82`

Post-merge verification established:

- [x] `main` points at merge commit `a0bf3540f164c3466eee9592a493fb4bc2902d5d`;
- [x] comparison from the authorised head to the merge commit contains no changed files; merge history changed, tree contents did not;
- [x] GitHub Actions CI #143 (`33889135839`) ran on the exact merged `main` commit and completed **PASS**;
- [x] post-merge native Debug and Release passed;
- [x] post-merge ASan+UBSan passed;
- [x] post-merge no-exceptions/no-RTTI portability probe passed;
- [x] post-merge M2/M3 browser Lab builds and native/WASM parity passed;
- [x] Windows/macOS and Chromium/Firefox/WebKit/Edge final-platform evidence remains valid because it passed on the exact authorised head and the merge tree is identical;
- [x] the normal `main` push correctly skipped manual hosted cross-platform/browser-smoke jobs by workflow design;
- [x] no blocking defect remains.

## Final gate

M3 final requirements are all satisfied:

1. [x] M3.0–M3.14 implementation and acceptance gates complete;
2. [x] B01–B18 human acceptance PASS;
3. [x] final hostile review PASS with H1–H8 resolved;
4. [x] exact final closure head passed CI, raw-head, hosted platform and browser gates;
5. [x] protected exact-head merge was explicitly authorised and completed;
6. [x] authorised head and merge commit have identical tree contents;
7. [x] post-merge `main` CI #143 completed PASS on the exact merge commit;
8. [x] M3 is declared FINAL PASS and frozen.

## Final decision

**M3 FINAL PASS. M3 is complete and frozen.**

The Breath Pipe reference voice, its B01–B18 acceptance contract and the M3 evidence baseline are frozen. Future milestones may extend the engine through explicit decisions, but must not silently retune or reinterpret the accepted M3 model and contracts.
