# M2 — Resonant Engine Lab

Status: **ENGINEERING CANDIDATE**

## Purpose

M2 makes the headless Resonant Engine observable, measurable and testable by humans.

The Lab is **not a browser synth**. It is an engineering and human-acceptance harness around the same C++ synthesis implementation used by native hosts.

The boundary is:

```text
resonant_core
    │
    ├── native/offline hosts
    │
    └── resonant_lab adapter
             │
             └── WASM + AudioWorklet browser Lab
                     ├── deterministic test control
                     ├── measurements
                     └── evidence capture
```

The browser contains no replacement resonator, exciter, feedback loop or synthesis algorithm.

## Lab adapter

`lab/include/resonant_lab/LabEngine.hpp` is test-host orchestration, not a new synthesis core. It owns a fixed bank of eight real `Engine<FirstResonatorVoice>` instances so M2 can exercise chords and bounded voice pressure before a future product host defines its own voice-allocation policy.

Properties:

- fixed capacity: 8 voices;
- deterministic per-voice seeds from the frozen core seed derivation;
- free voice first, then oldest released voice, then oldest held voice;
- observable voice stealing;
- no dynamic allocation in the audio process path;
- deterministic reset;
- parameter metadata used by the browser Lab;
- bounded monitor mix while retaining pre-monitor core peak/energy diagnostics.

The Lab voice allocator is **not** a universal Resonant Engine polyphony policy.

## Browser test harness

`hosts/browser/lab/` contains the M2 host.

It provides only test-oriented controls:

- start and suspend audio;
- reset and panic;
- exact MIDI note number;
- exact velocity;
- note-on/note-off;
- canonical test preset loading;
- canonical parameter controls;
- selected or all-suite execution.

It deliberately has no piano keyboard, computer performance keyboard, Web MIDI instrument surface or patch-design workflow.

## WASM

`LabWasm.cpp` compiles the Lab adapter and the shared C++ core with Emscripten. The AudioWorklet wrapper translates browser messages/audio quanta only.

The normal browser build uses:

- C++20;
- no exceptions;
- no RTTI;
- fixed WASM memory;
- 128-frame Web Audio render quantum;
- one mono diagnostic output before browser monitor gain.

A separate Node-targeted WASM build exists only for CI native/WASM parity comparison.

## Measurements

### Waveform

The rolling output waveform uses an 8192-sample `AnalyserNode` time-domain window. It observes Lab output before the host-only monitor gain.

### Spectrum

The spectrum uses the browser analyser's 8192-point FFT and a logarithmic 20 Hz–20 kHz display. The current display range is -100 dB to 0 dB.

### Fundamental frequency

F0 uses a decimated autocorrelation estimator over the analyser window and reports 50–2000 Hz. It updates at approximately 4 Hz. The C2–C6 test stores the target frequency and calculates cents error at deterministic audio-thread markers.

F0 is diagnostic evidence, not a tuning authority. Strongly inharmonic/noisy/self-oscillating states may legitimately produce no stable estimate.

### RMS, peak and DC

RMS, peak and DC offset are calculated from the same rolling pre-monitor waveform window.

### Resonator energy

The core already exposes per-voice `EnergyDiagnostics`. Lab resonator energy is the mean of squared per-voice resonator RMS values for currently processed voices. It is an operational energy proxy, not a physical joule measurement.

### Stability state

M2 maps core diagnostics to:

```text
QUIET
ACTIVE
HIGH_ENERGY
SELF_OSCILLATING
NEAR_LIMIT
UNSTABLE
PROTECTED
```

Current classification rules:

- core NaN/Infinity, `Unstable` or `NumericalRunaway` -> `UNSTABLE`;
- peak >= 1.35 or resonator RMS >= 1.2 -> `NEAR_LIMIT`;
- core `SelfSustaining` -> `SELF_OSCILLATING`;
- output RMS >= 0.25 or resonator RMS >= 0.35 -> `HIGH_ENERGY`;
- core `Silent` -> `QUIET`;
- otherwise -> `ACTIVE`;
- a failed child-engine process moves the Lab to `PROTECTED` and silences the monitor mix until reset.

These thresholds are diagnostic policy. They do not redefine core musical stability.

### Active voices

The Lab reports active voices, held voices, maximum active voices, maximum configured polyphony and voice steals.

### CPU load

CPU is measured around the C++/WASM Lab process call itself:

```text
100 * process_elapsed_time / audio_quantum_budget
```

The Lab reports instantaneous, smoothed and maximum values. The smoothed value uses a 0.94/0.06 previous/current blend.

Interpretation:

- <50%: healthy;
- 50–80%: elevated;
- 80–100%: realtime risk;
- >100%: realtime failure/investigation.

The metric intentionally excludes canvas rendering and optional evidence encoding. It is a browser/WASM realtime diagnostic and is not directly interchangeable with a native-host CPU meter.

## Canonical presets

`lab/contracts/presets.json` is the canonical M2 preset document. Presets contain only named test states and canonical parameter IDs/values.

Current presets:

- Neutral;
- Pluck;
- Sustained pipe;
- Feedback sweep;
- Extreme stability.

The label “Sustained pipe” describes the M2 listening scenario; `FirstResonatorVoice` is still a generic M1 resonator, not the future Breath Pipe model.

## Canonical human acceptance suite

`lab/contracts/acceptance-tests.json` is the machine-readable suite.

### A01 — Pluck

Tests transient quality, resonant decay, finite output and click/runaway behaviour.

### A02 — Sustained pipe

Tests continuously driven sustained resonance, pitch/timbre continuity, energy and release behaviour.

### A03 — Damping sweep

Sweeps damping upward and downward while sustained. Tests continuity, energy/decay response and smoothing.

### A04 — Feedback sweep

Sweeps regeneration from passive through high feedback and back. Tests state transitions and recovery.

### A05 — C2–C6 pitch run

Runs MIDI 36 through 84 chromatically. Each note has a deterministic measurement marker with target frequency and cents-error capture.

### A06 — Velocity response

Runs velocities 16/32/48/64/80/96/112/127 and records a marked measurement snapshot for every step.

### A07 — Chord and polyphony

Runs 1, 2, 4 and 8 concurrent voices and records allocation, peak/energy and CPU scaling.

### A08 — Self-oscillation

Deliberately raises regeneration, removes active excitation and observes whether self-sustaining behaviour emerges, remains bounded and recovers when regeneration is reduced.

Self-oscillation itself is not a failure.

### A09 — Extreme stability

Combines eight voices, extreme parameters, opposing parameter sweeps, rapid retriggers, panic and reset. It is intended to expose NaN/Infinity, runaway, stuck voice, realtime and recovery defects.

## Deterministic runner

The main thread expands the declarative suite into primitive actions. The action list is sent once to the AudioWorklet. The AudioWorklet schedules those actions against processed audio frames and applies them on deterministic render-quantum boundaries.

The UI thread does not time note events with `setTimeout`.

`Run all nine` sequences the nine independently reset scenarios. Human listening verdicts remain separate from automatic measurement results.

## Evidence

Canonical evidence schema: `resonant-engine-lab-evidence/v1`.

Each run records:

```text
TEST
ENGINE_COMMIT
BUILD
BROWSER
SAMPLE_RATE
BLOCK_SIZE
POLYPHONY
PRESET
PARAMETERS
ACTION
EXPECTED
OBSERVED
MEASUREMENTS
AUTOMATED_RESULT
LISTENER_NOTES
RESULT
STARTED_AT
COMPLETED_AT
```

Evidence can be exported as JSON. Optional scenario capture can be exported as mono 16-bit PCM WAV. The current waveform and spectrum can be exported together as PNG.

The build writes the exact Git commit into `build-info.js`, allowing exported evidence to identify its source candidate.

## Recovery and failure domains

- `Panic` clears voices immediately while retaining parameter state.
- `Reset` restores canonical Lab parameter defaults, deterministic voice seeds, voice state and CPU history.
- core numerical/process failure moves the Lab to `PROTECTED` and silences output;
- the browser host reports Worklet/module errors separately from core protected-state telemetry;
- optional WAV capture is downstream of the DSP timing measurement;
- monitor gain is browser-only and is not a synthesis parameter.

## Browser behaviour

Audio starts only after explicit user interaction, satisfying normal autoplay policy. `Stop audio` suspends the `AudioContext`; `Start audio` resumes the same context and engine state. Reset is available without page reload.

The UI is responsive and DSP state does not depend on viewport dimensions. Browser tab suspension is a Host scheduling condition; returning to a suspended context requires explicit start/resume rather than silently pretending realtime continuity.

Automated CI exercises Chromium, Firefox, WebKit and Microsoft Edge. WebKit is a portability proxy; it is not evidence that a particular Safari release has been manually listened to.

## Build

With Emscripten active:

```sh
./hosts/browser/lab/build.sh
python3 -m http.server 8000 --directory build/resonant-lab
```

Open `http://127.0.0.1:8000/` and press **Start audio**.

## Completion rule

M2 engineering implementation is complete when native tests, WASM build, parity comparison and browser smoke gates pass at the exact candidate head.

M2 as a human acceptance milestone additionally requires a person to run/listen to all nine canonical tests and record the resulting evidence. A protected merge is a separate final authority boundary.
