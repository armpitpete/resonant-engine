# Resonant Engine

Resonant Engine is a portable resonant/physical synthesis platform capable of running from the same core DSP architecture in browsers, DAWs, embedded hardware and other host instruments.

## M0 — Portable Real-Time Foundation

**Status: COMPLETE AND FROZEN.**

M0 establishes a portable real-time DSP architecture specifically capable of supporting expressive, continuously excited resonant systems, while remaining general enough for strings, structures, coupled resonators and impossible synthetic bodies later.

**Primary sonic design priority:** air / pipe / noise.

M0 is architecturally centered on one C++20 `resonant_core`. Browser/WASM, VST3/JUCE, embedded hardware and other synths are thin Hosts around that shared core; host-specific DSP duplication is prohibited.

## M0 result

The merged `main` branch contains the full M0 contract/runtime foundation:

- normative product/scope, sonic, terminology and core-boundary contracts;
- hard real-time processing rules;
- float32 non-interleaved block transport with sample-level model feedback;
- sample-accurate deterministic events;
- native/normalized parameters with sample-rate-aware smoothing;
- generic exciter/resonator compatibility contracts;
- feedback, energy/stability and lifecycle contracts;
- deterministic PCG32 randomness/voice seed derivation;
- Debug/Release, MSVC/GCC/Clang and sanitizer CI;
- deterministic offline WAV render host;
- unit/property/regression testing foundation;
- WASM/VST3/embedded portability proof for M0 scope;
- research/reference record and accepted ADR register;
- final hostile Breath Pipe architecture review and M0 completion gate.

The `ReferenceFeedbackProbe` remains an **M0 architectural fixture**, not the Breath Pipe voice and not the preferred musical model.

M0 is frozen as the baseline for later milestones. Later work may extend it through explicit architecture decisions, but must not silently reinterpret the M0 invariants or Host/core boundary.

## M1 — First Resonator

**Status: CANDIDATE — engineering gates pass; human live play-test gate remains on the M1 branch.**

M1 adds the first concrete musical resonator without changing the frozen Engine/Host architecture:

- `ContinuousNoiseExciter` — deterministic continuous noise, transient and arbitrary external-audio excitation;
- `TunedDelayResonator` — fixed-memory fractional-delay resonance with damping, passive loss, active regeneration and bounded in-loop nonlinearity;
- `FirstResonatorVoice` — sample-accurate, smoothed composition of those primitives with energy/stability observation;
- deterministic offline render scenes used only for regression evidence;
- a temporary browser AudioWorklet/WASM acceptance harness that runs the same C++ `FirstResonatorVoice` live.

The M1 reference model is deliberately generic and replaceable. It is **not Breath Pipe**, not a Steampipe clone and not a rule that future Resonant Engine models must use tuned delays.

The temporary M1 keyboard/MIDI surface exists only to judge the first resonator. It is not a Resonant Engine browser product and is not the UI direction for M2.

## M2 — Resonant Engine Lab

**Status: ENGINEERING CANDIDATE — exact-head CI and human A01–A09 evidence remain final gates.**

M2 makes the headless engine observable, measurable and reproducibly testable by humans.

**Resonant Engine Lab is not a browser synth.** It is a bounded diagnostic/test host around the shared C++ engine.

The Lab provides:

- the same `FirstResonatorVoice` implementation compiled to WASM;
- a fixed eight-voice test bank outside `resonant_core` for chord/polyphony stress;
- exact note, velocity, parameter and canonical preset controls;
- start/suspend/reset/panic lifecycle;
- waveform and spectrum;
- fundamental frequency and C2–C6 cents-error capture;
- RMS, peak and DC offset;
- resonator energy and stability state;
- active/held/max voices and voice steals;
- instantaneous/smoothed/max WASM process CPU load;
- canonical A01–A09 scripted human acceptance tests;
- deterministic audio-quantum scenario scheduling;
- JSON evidence, optional WAV capture and plot PNG export;
- native/WASM deterministic signature comparison;
- automated Chromium, Firefox, WebKit and Microsoft Edge browser smoke gates.

The Lab deliberately does **not** provide a performance piano, computer-keyboard instrument, Web MIDI performance workflow, patch designer or browser-specific synthesis algorithm.

### Build the Lab

With Emscripten active:

```sh
./hosts/browser/lab/build.sh
python3 -m http.server 8000 --directory build/resonant-lab
```

Open `http://127.0.0.1:8000/` and press **Start audio**.

Canonical test/preset contracts live in `lab/contracts/`. Detailed definitions and evidence rules are in `docs/M2-RESONANT-ENGINE-LAB.md`.

## Protected future Breath Pipe reference

```text
Pressure + Turbulence
        ↓
Exciter interaction
        ↕
Tuned resonator
        ↺
Active feedback
        ↓
Feedback filtering
        ↓
Nonlinearity
        ↺
```

Required future continuum:

```text
silence
→ faint air
→ turbulence
→ emerging pitch
→ unstable resonance
→ stable pipe
→ rich pipe
→ overblow
→ aggressive resonance
→ noise
```

## Build and test

```sh
cmake -S . -B build -DRESONANT_ENGINE_BUILD_TESTS=ON -DRESONANT_ENGINE_BUILD_RENDER=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Offline render tools are deterministic regression evidence. They are not substitutes for the M1 or M2 human listening gates.

## Documentation

M2:

- `docs/M2-RESONANT-ENGINE-LAB.md`
- `docs/M2-COMPLETION.md`
- `docs/decisions/ADR-0024-M2-RESONANT-ENGINE-LAB.md`
- `hosts/browser/lab/README.md`
- `lab/contracts/presets.json`
- `lab/contracts/acceptance-tests.json`

M1:

- `docs/M1-FIRST-RESONATOR.md`
- `docs/M1-COMPLETION.md`
- `docs/M1-HOSTILE-ARCHITECTURE-REVIEW.md`
- `docs/decisions/ADR-0023-M1-FIRST-RESONATOR.md`
- `docs/research/M1-FIRST-RESONATOR.md`
- `hosts/browser/m1/README.md`

Frozen M0 baseline:

- `docs/M0-COMPLETION.md`
- `docs/architecture/PRODUCT_SCOPE.md`
- `docs/architecture/AIR_PIPE_NOISE.md`
- `docs/architecture/GLOSSARY.md`
- `docs/architecture/CORE_BOUNDARY.md`
- `docs/architecture/REALTIME.md`
- `docs/architecture/PROCESSING.md`
- `docs/architecture/EVENTS.md`
- `docs/architecture/PARAMETERS.md`
- `docs/architecture/EXCITER_RESONATOR.md`
- `docs/architecture/FEEDBACK.md`
- `docs/architecture/ENERGY_STABILITY.md`
- `docs/architecture/DETERMINISM.md`
- `docs/architecture/LIFECYCLE.md`
- `docs/architecture/PORTABILITY.md`
- `docs/research/INDEX.md`
- `docs/decisions/ADR_INDEX.md`

## Licence

MIT. See `LICENSE`.
