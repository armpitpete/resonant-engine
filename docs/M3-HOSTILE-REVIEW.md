# M3 — Pre-Closure Hostile Review

Status: **FINAL PRE-MERGE HOSTILE REVIEW — PASS**

Final implementation candidate reviewed: PR #7 head `fb0375bf6e8899c3b3d4f6c8221d8ce3d150d936`. Earlier findings are retained below as the audit trail that led to this final review.

This review is deliberately adversarial. Human B01–B18 acceptance and the final platform matrix have independently passed; this review tests whether any remaining implementation, architecture, evidence or closure defect blocks the protected merge gate.

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

## Closure defects found

### H1 — CI provenance was not strict raw-head evidence

CI run #135 was associated with PR head `714df0b758ff1f92f1590c69169c293b3d30bcea`, but GitHub's default `pull_request` checkout built synthetic merge commit `cac23e55fac5b9e8b1375ec1333f380d669b849e`.

That is useful tested-merge evidence, but the M3 contract explicitly requires an exact candidate head. It must not be described as raw-head proof.

Resolution: add a dedicated `PR Exact Head` workflow that checks out `github.event.pull_request.head.sha`, asserts `git rev-parse HEAD` equals that SHA, and reruns the native, sanitizer, portability and M3 native/WASM parity gates.

### H2 — completion ledger materially contradicted repository state

`docs/M3-COMPLETION.md` still said `PLANNED — 0/16 SECTIONS COMPLETE` and `implementation has not begun` even though the PR contains the Breath Pipe implementation, tests, Lab adapter and acceptance suite.

Resolution: reconcile the ledger to implementation-candidate state while leaving M3.13–M3.15 open.

### H3 — M3 human-suite projection retained stale M2 count text

The built M3 Lab loaded all 18 B-series scenarios and `Run all` queued the complete `state.tests` array, but the inherited button label still said `Run all nine` and the heading still presented the artifact as `M2 — Resonant Engine Lab — M3 Breath Pipe`.

This does not remove tests, but it is unacceptable at the human acceptance gate because the interface contradicts the canonical B01–B18 suite.

Resolution: keep frozen M2 source files unchanged and fix only the `build-m3.sh` projection so the artifact is titled `M3 — Breath Pipe Reference Voice Lab` and the batch control is count-neutral `Run all`.

### H4 — exact-head artifact retained synthetic tested-commit provenance

The first raw-head validation artifact correctly recorded `commit: 9a52d3dc08fa7be5de77191612d33b5b3e8142dd`, but `testedCommit` still came from the pull-request event's `GITHUB_SHA` and therefore recorded synthetic merge commit `cbae7b79a60456f1b8adf08db5c4d2d7f147347d` even though the workflow had explicitly checked out and tested the raw head.

The DSP result was not invalid, but the artifact provenance was internally contradictory and could not be accepted as final evidence.

Resolution: add an explicit `RESONANT_LAB_TESTED_SHA` override to the M3 build, set both source and tested SHA to the verified raw head in `PR Exact Head`, and assert both fields in the generated `build-info.js`. Normal PR integration CI continues to record the synthetic merge commit as the tested commit, preserving the distinction between raw-head and tested-merge evidence.

### H5 — the human Lab attenuated the accepted sound

The first B01–B18 human run produced a positive sonic verdict — the sounds were good — but the listener reported that the volume was low. Inspection showed the inherited M2 monitor control defaulted to `0.6x` gain and was capped at `0.8x`, so the Lab was attenuating an already conservative core signal.

Resolution: change only the built M3 projection to a clearly labelled audition-only monitor control with `2x` default and `4x` maximum. The analyser, WAV capture, core telemetry and Breath Pipe DSP remain before that gain, so this does not alter the accepted synthesis or measurement evidence.

### H6 — a 128-frame block mean was labelled as DC offset

The B08 regeneration run recorded `scenarioMaxAbsDc = 0.05116955156699987` and `scenarioExcessiveDc = true`, while its 8192-frame analyser measurement at the regenerative mark was only about `0.00055`, the recovery mark was about `0.00003`, and the run recovered to near-zero final DC. At 48 kHz a 128-frame mean spans only 2.67 ms, so a low-frequency musical oscillation can produce a large block mean without representing a real DC component.

The old telemetry vocabulary therefore overstated what the measurement proved. It was not a demonstrated numerical/DC failure.

Resolution: keep frozen M2 source semantics unchanged, but in the built M3 projection rename the telemetry evidence to `outputBlockMean`, `scenarioMaxAbsBlockMean` and `scenarioLargeBlockMean`. The long-window analyser remains the Lab's DC-offset diagnostic. The M3 evidence metadata explicitly states this distinction.

### H7 — browser autocorrelation aliased the low B14 registers

The B14 browser evidence reported the autocorrelation estimator at its 2 kHz ceiling for C2 and C3 even though the same run was sonically accepted and higher registers were coherent. This is a diagnostic estimator failure, not valid evidence that the model was tuned to 2 kHz.

The canonical M3 tuning gate already exists independently in `tests/unit/test_breath_pipe_contract.cpp`: it directly measures C2, C3, C4, C5 and C6 at 44.1, 48 and 96 kHz, requires median absolute error at or below 15 cents and rejects any measured note above 30 cents error.

Resolution: retain the browser autocorrelator as a diagnostic display but label it explicitly as diagnostic in the M3 projection and name the native M3 contract as authoritative for the C2–C6 tuning criterion. Do not alter the approved Breath Pipe DSP to satisfy a faulty browser estimator.

### H8 — hosted platform jobs failed before runner allocation

Two final-platform attempts requested `ubuntu-latest`, `windows-latest` and `macos-latest`. Every runnable hosted job failed with no workflow steps, no runner name and `runner_id: 0`; the browser-smoke matrix then skipped because its hosted build prerequisite never ran.

This is not evidence of a CMake, compiler, Emscripten, browser or DSP failure because none of those steps executed. It is a real M3.14 closure blocker until GitHub-hosted runner allocation is restored and the required Windows/macOS/browser jobs actually run.

Resolution state: **RESOLVED.** Hosted Actions capacity was restored and `M3 Final Platform` run #3 (`33791246542`) completed successfully on exact implementation head `fb0375bf6e8899c3b3d4f6c8221d8ce3d150d936`, including Windows Debug/Release, macOS Debug/Release, exact-head browser build/parity, and Chromium/Firefox/WebKit/Edge realtime smoke + CPU gates.

## Final hostile-review result

**PASS — no blocking defect found.**

At `fb0375bf6e8899c3b3d4f6c8221d8ce3d150d936`:

- B01–B18 human acceptance is PASS, including the corrected audition-monitor confirmation;
- `PR Exact Head` run #6 (`33791246658`) is PASS;
- CI #141 (`33791246636`) is PASS;
- `M3 Final Platform` run #3 (`33791246542`) is PASS;
- Windows and macOS native Debug/Release all pass;
- Chromium, Firefox, Playwright WebKit and Microsoft Edge realtime smoke/CPU gates all pass;
- native/WASM parity, sanitizers and no-exceptions/no-RTTI portability all pass;
- no architecture-boundary, energetic-model, boundedness, recovery, provenance or acceptance contradiction remains.

The implementation tree is therefore accepted for the M3 protected merge gate. Do not add features or retune the Breath Pipe during closure.

## Gates intentionally still open

- fresh exact-head validation of the documentation-only closure reconciliation commit;
- exact final-head merge authorisation;
- merge and post-merge `main` proof;
- FINAL PASS documentation and freeze.

## Final-review rule

This is the final hostile review for implementation head `fb0375bf6e8899c3b3d4f6c8221d8ce3d150d936`. The following closure commit is restricted to `docs/M3-COMPLETION.md`, `docs/M3-HOSTILE-REVIEW.md` and `README.md`. Any DSP, contract, test, workflow or other implementation change after this point invalidates the review and reopens M3.15.
