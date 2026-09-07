# Resonant Engine

Resonant Engine is a portable resonant/physical synthesis platform capable of running from the same core DSP architecture in browsers, DAWs, embedded hardware and other host instruments.

## M0 — Portable Real-Time Foundation

**Status: COMPLETE AND FROZEN.**

M0 establishes a portable real-time DSP architecture specifically capable of supporting expressive, continuously excited resonant systems, while remaining general enough for strings, structures, coupled resonators and impossible synthetic bodies later.

**Primary sonic design priority:** air / pipe / noise.

M0 is architecturally centered on one C++20 `resonant_core`. Browser/WASM, VST3/JUCE, embedded hardware and other synths are thin Hosts around that shared core; host-specific DSP duplication is prohibited.

The `ReferenceFeedbackProbe` remains an **M0 architectural fixture**, not the Breath Pipe voice and not the preferred musical model.

M0 is frozen as the baseline for later milestones. Later work may extend it through explicit architecture decisions, but must not silently reinterpret the M0 invariants or Host/core boundary.

## M1 — First Resonator

**Status: COMPLETE AND FROZEN.**

M1 adds the first concrete musical resonator without changing the frozen Engine/Host architecture:

- `ContinuousNoiseExciter` — deterministic continuous noise, transient and arbitrary external-audio excitation;
- `TunedDelayResonator` — fixed-memory fractional-delay resonance with damping, passive loss, active regeneration and bounded in-loop nonlinearity;
- `FirstResonatorVoice` — sample-accurate, smoothed composition of those primitives with energy/stability observation;
- deterministic offline render scenes used only for regression evidence;
- a browser AudioWorklet/WASM acceptance harness that runs the same C++ `FirstResonatorVoice` live.

M1 exact head `910616d0396ab516fa0b3272fe3067c23bffacb6` passed CI #60 and hostile review, then passed the human live gate through the later M2 Lab using the identical `FirstResonatorVoice` core. PR #4 merged to `main` as `54ebc45e4a0b2c96a7733ce99f49b3855acad9a7`; the merged tree is identical to the green exact-head tree.

The M1 reference model is deliberately generic and replaceable. It is **not Breath Pipe**, not a Steampipe clone and not a rule that future models must use tuned delays.

## M2 — Resonant Engine Lab

**Status: COMPLETE AND FROZEN.**

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

The realtime AudioWorklet path was repaired after a browser-scope timing crash was isolated, and A01 Pluck carries a measurable decay-audibility regression gate. Human A01–A09 acceptance is **PASS**.

M2 exact candidate `c3db8578ae1afabe23140e4449be6b6f73667f61` passed CI #96, hostile review and the human suite, then PR #5 merged to `main` as `460dcebe60babf360ee5b4c72ab8bb97d4a21dd8`. Post-merge CI #97 passed the native/platform matrix, sanitizers, portability probe, native/WASM parity, and Chromium/Firefox/WebKit/Microsoft Edge realtime browser smoke gates on the exact merged commit.

The Lab deliberately does **not** provide a performance piano, computer-keyboard instrument, Web MIDI performance workflow, patch designer or browser-specific synthesis algorithm.

### Build the Lab

With Emscripten active:

```sh
./hosts/browser/lab/build.sh
python3 -m http.server 8000 --directory build/resonant-lab
```

Open `http://127.0.0.1:8000/` and press **Start audio**.

Canonical test/preset contracts live in `lab/contracts/`. Detailed definitions and evidence rules are in `docs/M2-RESONANT-ENGINE-LAB.md`.

## M3 — Breath Pipe Reference Voice

**Status: COMPLETE AND FROZEN.**

M3 turns the frozen architecture, first resonator and Lab into the first fully expressive Resonant Engine reference voice.

The implemented candidate is a bounded three-mode interacting modal model. It was selected over the M1 tuned-delay extension and a bounded scattering-waveguide prototype because it provides the required compact continuous pitch/register reorganisation with much smaller fixed resonator state. The selection remains Breath-Pipe-specific and does not make modal synthesis a generic Engine requirement.

The Breath Pipe behaves as one continuously interacting energetic system rather than a set of pipe presets. The protected acceptance continuum is:

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

The candidate includes pressure/turbulence excitation, bidirectional returned-state interaction, active regeneration/self-sustain, in-loop nonlinearity, feedback spectral shaping, continuous modal overblow, model-level expressive controls, external-audio excitation, deterministic fixed-state processing, bounded Lab polyphony, B01–B18 acceptance programs and a separate Breath Pipe WASM Lab projection.

The initial M3 operating envelope requires 44.1/48/96 kHz behaviour, native block-size coverage from 32–1024 frames plus validity at the M0 4096-frame maximum, C2–C6 stable-pitch acceptance, four-voice musical polyphony with eight-voice stress, no added host-visible buffering, explicit tuning/CPU gates, and an external-audio excitation proof.

Routine native, sanitizer, portability and native/WASM validation is routed to the repository-scoped `oracle-resonant-engine-01` runner. Windows/macOS and four-browser realtime smoke remain deliberate final platform gates rather than per-commit hosted jobs.

Human acceptance is B01–B18 covering silence, air, turbulence, pitch emergence, stable pipe, pressure/damping response, regeneration, self-sustain, overblow, forward/reverse continuum, hysteresis/recovery, expressive performance, pitch/register, aggressive/noise behaviour, polyphony, external excitation and extreme stability.

Implementation head `fb0375bf6e8899c3b3d4f6c8221d8ce3d150d936` passed the exact-head Linux/WASM suite, sanitizers and portability probe, Windows/macOS Debug+Release, and Chromium/Firefox/Playwright WebKit/Microsoft Edge realtime smoke + CPU gates. Final closure head `f38d0d7bce6b1fa8471dd1e96a1ac91783b0d2fa` then passed CI #142, PR Exact Head #7 and M3 Final Platform #4 before explicit merge authorisation. PR #7 merged to `main` as `a0bf3540f164c3466eee9592a493fb4bc2902d5d` with a tree identical to the authorised head, and post-merge CI #143 passed native Debug/Release, sanitizers, portability and M2/M3 native/WASM parity on the exact merged commit. M3 is complete and frozen.

M3 remains **not a Steampipe clone, not a browser synth product, and not a requirement that future Resonant Engine models use the Breath Pipe topology**.

Canonical M3 contract: `docs/M3-BREATH-PIPE.md`.


## M4 — DAW/VST3 Reference Host

**Status: M4.0–M4.8 MERGED AND COMPLETE — M4.9 NATIVE-CORE/VST3 PARITY ACCEPTED; FINAL-HEAD VALIDATION PENDING.**

M4 is the first real DAW-host proof for the frozen Resonant Engine. M4.0–M4.3 now provide a thin VST3 instrument component/controller around the same `resonant_core` and M3 Breath Pipe Reference Voice, using a pinned Steinberg VST3 SDK directly while keeping VST3/JUCE/DAW types outside the core. Oversized DAW blocks are split into bounded core calls without extra buffering or changing feedback timing.

M4 focuses on plugin lifecycle, audio/event translation, sample-accurate automation, portable state recall, optional external excitation, realtime boundedness, native-core/VST3 parity, Steinberg validator evidence and one real-DAW human acceptance gate. It does not add a custom GUI, a new synthesis model, a preset browser, or retune the frozen M3 voice.

Canonical M4 contract: `docs/M4-DAW-VST3-HOST.md`.

## Build and test

```sh
cmake -S . -B build -DRESONANT_ENGINE_BUILD_TESTS=ON -DRESONANT_ENGINE_BUILD_RENDER=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Offline render tools are deterministic regression evidence. They are not substitutes for human listening gates.

## Documentation

M3:

- `docs/M3-BREATH-PIPE.md`
- `docs/M3-COMPLETION.md`
- `docs/decisions/ADR-0025-M3-BREATH-PIPE-REFERENCE-VOICE.md`
- `docs/decisions/ADR-0026-M3-BREATH-PIPE-TOPOLOGY.md`

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
