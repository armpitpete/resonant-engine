# M2 — Resonant Engine Lab Completion

Status: **FINAL CANDIDATE — HUMAN ACCEPTANCE PASS; FRESH EXACT-HEAD CI / MERGE GATE REMAIN**

## Scope

M2 makes the canonical headless engine observable and testable by humans without creating a browser synth.

## Implemented

### M2.0 Boundary and architecture

- [x] Lab defined as diagnostic/test harness, not product synth.
- [x] shared C++ core remains the sound implementation.
- [x] WASM/AudioWorklet adapter boundary defined.
- [x] diagnostics/telemetry contract defined.
- [x] deterministic reset and fixed browser quantum defined.
- [x] architecture recorded in ADR-0024 and M2 documentation.

### M2.1 Browser test harness

- [x] canonical engine/Lab adapter compiles to WASM.
- [x] AudioWorklet host established.
- [x] start/suspend/reset/panic lifecycle.
- [x] exact note-on, note-off and velocity control.
- [x] deterministic scripted note sequences.
- [x] parameter controls use C++ min/max/default metadata.
- [x] canonical M2 preset document and loader.
- [x] active/held/max voice and steal telemetry.
- [x] fixed eight-voice bounded test bank.
- [x] native/WASM parity signature gate implemented.
- [x] realtime AudioWorklet crash isolated to unsupported `performance.now()` use in AudioWorklet scope and repaired with an AudioWorklet-safe clock bridge.

### M2.2 Measurement tools

- [x] waveform.
- [x] spectrum.
- [x] fundamental frequency.
- [x] C2-C6 cents-error markers.
- [x] RMS.
- [x] peak.
- [x] DC offset.
- [x] resonator energy.
- [x] stability state.
- [x] active voices.
- [x] instantaneous/smoothed/max WASM process CPU load.

### M2.3 Human acceptance suite

- [x] A01 Pluck.
- [x] A02 Sustained pipe.
- [x] A03 Damping sweep.
- [x] A04 Feedback sweep.
- [x] A05 C2-C6 pitch run.
- [x] A06 Velocity response.
- [x] A07 Chord/polyphony.
- [x] A08 Self-oscillation.
- [x] A09 Extreme stability.
- [x] all nine listened to by a human and accepted.

A01 initially failed because the pluck was too quiet. The defect was treated as a Lab acceptance-stimulus/listening-calibration problem rather than hidden with global engine gain. The final A01 has stronger bounded excitation/decay calibration and browser CI enforces a measurable decay RMS floor at the 700 ms mark. The final human A01 retest passed.

### M2.4 Automated runner and evidence

- [x] machine-readable acceptance definitions.
- [x] declarative programs compiled to primitive actions.
- [x] audio-frame/quantum deterministic scheduler.
- [x] selected-test and run-all-nine execution.
- [x] abort -> panic recovery.
- [x] marked measurement capture.
- [x] automatic and human verdicts separated.
- [x] canonical JSON evidence schema.
- [x] browser/user-agent, sample rate, block size, polyphony and preset captured.
- [x] stability transitions and CPU maxima captured.
- [x] listener notes and PASS/FAIL/INVESTIGATE field.
- [x] JSON export, optional WAV capture/export and waveform+spectrum PNG export.
- [x] `ENGINE_COMMIT` is stamped from the exact PR source head in pull-request CI rather than GitHub's synthetic merge SHA.
- [x] the synthetic commit actually tested by CI is retained separately as `BUILD_INFO.testedCommit` and in the artifact/workflow identity.

### M2.5 Failure detection and recovery

- [x] core NaN/Infinity/numerical-process failure reaches protected state.
- [x] pre-monitor core peak and energy expose runaway growth.
- [x] active voice telemetry exposes stuck voice conditions.
- [x] CPU >100% is classified as realtime failure/investigation.
- [x] invalid UI values are clamped to C++ metadata ranges.
- [x] protected output is silenced until an explicit Lab recovery action.
- [x] `Panic` is an explicit recovery action: it clears/resets child voices while retaining parameter state.
- [x] `Reset` is the full deterministic recovery action: it restores canonical defaults/seeds, voices and CPU history.
- [x] evidence retains any hard-failure telemetry observed before recovery.

This matches the actual Lab implementation and does not redefine the frozen M0 lifecycle contract.

### M2.6 Browser/platform verification

- [x] explicit user-start handles autoplay restrictions.
- [x] responsive UI has no DSP dependency on viewport size.
- [x] suspend/resume lifecycle implemented.
- [x] Chromium automated smoke gate.
- [x] Firefox automated smoke gate.
- [x] WebKit automated smoke gate.
- [x] Microsoft Edge automated smoke gate.
- [x] realtime smoke proves active audio processing rather than constructor readiness alone.
- [x] A01 browser regression enforces non-trivial decay audibility.
- [x] host errors remain distinct from core protected-state failures.

Playwright WebKit is not represented as manual Safari evidence; no Safari-specific release claim is made by M2.

### M2.7 CI evidence

Earlier implementation head `b9fdd3fe964756b18ad9899288714700d8f963a0` passed CI #90 (`33687382606`), including:

- [x] Ubuntu GCC Debug/Release.
- [x] macOS Clang Debug/Release.
- [x] Windows MSVC Debug/Release.
- [x] ASan+UBSan.
- [x] no-exceptions/no-RTTI portability probe.
- [x] WASM Lab build.
- [x] native/WASM deterministic signature parity.
- [x] Chromium realtime browser smoke.
- [x] Firefox realtime browser smoke.
- [x] WebKit realtime browser smoke.
- [x] Microsoft Edge realtime browser smoke.

Final documentation, M1 closure reconciliation and evidence-provenance hardening deliberately changed the M2 head, so one final exact-head CI PASS is required before PR #5 can leave Draft.

### M2.8 Hostile review

Final diff review against merged M1 `main` found and resolved two blockers:

1. recovery documentation incorrectly implied only `Reset` could leave Lab protection even though `Panic` explicitly resets child voices and recovers while retaining parameters;
2. pull-request builds used GitHub's synthetic merge SHA as `ENGINE_COMMIT`, obscuring the exact candidate head.

Both are resolved. No remaining blocking architecture, realtime, evidence, browser-product-boundary or M0-invariant defect is known.

### M2.9 Documentation

- [x] M2 architecture and measurement definitions documented.
- [x] browser-Lab boundary documented.
- [x] stability states documented.
- [x] CPU calculation documented.
- [x] canonical tests documented.
- [x] evidence format documented.
- [x] known browser interpretation limits documented.
- [x] ADR-0024 records the no-browser-synth decision.
- [x] M1 completion status reconciled to merged/frozen state.
- [x] README reconciled to M1 complete and M2 human PASS state.

## Final gate

M2 cannot be marked FINAL PASS until all of the following are true:

1. [ ] fresh exact-head CI PASS after final hostile-review repairs;
2. [x] final hostile diff review against `main` completed with blockers repaired;
3. [x] human A01-A09 acceptance PASS;
4. [x] no currently known blocking defect remains;
5. [ ] PR #5 is moved from Draft to Ready after exact-head CI is green;
6. [ ] protected exact-head merge is separately authorised and completed;
7. [ ] post-merge `main` verification records the merged tree/commit;
8. [ ] M2 is declared FINAL PASS and frozen.

## Current decision

**M2 FINAL CANDIDATE. Human acceptance and hostile review are complete; final exact-head CI and protected merge closure remain.**
