# M2 — Resonant Engine Lab Completion

Status: **ENGINEERING CANDIDATE — exact-head CI and human listening evidence pending**

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
- [x] known signal/path validation is covered by native/WASM signature and existing M1 deterministic regression; browser measurement sanity remains part of human Lab use.

### M2.3 Human acceptance suite

- [x] A01 Pluck defined and runnable.
- [x] A02 Sustained pipe defined and runnable.
- [x] A03 Damping sweep defined and runnable.
- [x] A04 Feedback sweep defined and runnable.
- [x] A05 C2-C6 pitch run defined and runnable.
- [x] A06 Velocity response defined and runnable.
- [x] A07 Chord/polyphony defined and runnable.
- [x] A08 Self-oscillation defined and runnable.
- [x] A09 Extreme stability defined and runnable.
- [ ] all nine listened to by a human at the exact M2 candidate and verdicts recorded.

### M2.4 Automated runner

- [x] machine-readable acceptance definitions.
- [x] declarative programs compiled to primitive actions.
- [x] audio-frame/quantum deterministic scheduler.
- [x] selected-test execution.
- [x] run-all-nine execution.
- [x] abort -> panic recovery.
- [x] marked measurement capture.
- [x] automatic and human verdicts separated.

### M2.5 Evidence capture

- [x] canonical JSON evidence schema.
- [x] exact Git commit recorded in browser build.
- [x] browser/user-agent, sample rate, block size, polyphony and preset captured.
- [x] expected/actions/observed measurements captured.
- [x] stability transitions and CPU maxima captured.
- [x] listener notes and PASS/FAIL/INVESTIGATE field.
- [x] JSON export.
- [x] optional WAV capture/export.
- [x] waveform+spectrum PNG export.

### M2.6 Failure detection and recovery

- [x] core NaN/Infinity/numerical-process failure reaches protected state.
- [x] pre-monitor core peak and energy expose runaway growth.
- [x] active voice telemetry exposes stuck voice conditions.
- [x] CPU >100% is classified as realtime failure/investigation.
- [x] invalid UI values are clamped to C++ metadata ranges.
- [x] panic clears voices.
- [x] deterministic reset restores known defaults/seeds.
- [x] protected output is silenced until reset.
- [x] evidence preserves hard-failure telemetry.

### M2.7 Browser/platform verification

- [x] explicit user-start handles autoplay restrictions.
- [x] responsive UI has no DSP dependency on viewport size.
- [x] suspend/resume lifecycle implemented.
- [x] Chromium automated smoke gate implemented.
- [x] Firefox automated smoke gate implemented.
- [x] WebKit automated smoke gate implemented.
- [x] Microsoft Edge automated smoke gate implemented on Windows.
- [x] host errors remain distinct from core protected-state failures.
- [ ] exact-head browser matrix CI evidence recorded.
- [ ] manual Safari listening, if required for a release claim; Playwright WebKit alone is not labelled Safari evidence.

### M2.8 CI and regression

- [x] Lab native unit/stress/determinism tests added.
- [x] WASM Lab compilation gate added.
- [x] JSON/JavaScript artifact validation added.
- [x] native/WASM deterministic signature comparison added.
- [x] four-engine browser smoke matrix added.
- [x] existing compiler/Debug/Release/sanitizer/core-portability gates retained.
- [ ] exact M2 candidate CI pass recorded.

### M2.9 Documentation

- [x] M2 architecture and measurement definitions documented.
- [x] browser-Lab boundary documented.
- [x] stability states documented.
- [x] CPU calculation documented.
- [x] canonical tests documented.
- [x] evidence format documented.
- [x] known browser interpretation limits documented.
- [x] ADR-0024 records the no-browser-synth decision.

## Final gate

M2 cannot truthfully be marked final until all of the following evidence exists:

1. exact-head CI PASS for native tests, WASM build, native/WASM parity and browser matrix;
2. a human has run/listened to A01-A09 at that candidate and stored verdict/evidence;
3. no blocking defect remains;
4. the candidate passes the protected merge gate;
5. post-merge `main` verification records the merged commit.

Until then, the implementation is an engineering candidate rather than a completed human-acceptance milestone.
