# M3 — Breath Pipe Reference Voice

Status: **CLOSURE CANDIDATE — FINAL ACCEPTANCE IN PROGRESS**

M3.0–M3.12 are implemented. The Breath Pipe sound/model is the accepted closure candidate; M3.13 human closure, M3.14 final hosted platform evidence and M3.15 final hostile review/merge/freeze remain gated by the completion rules below.

## Goal

M3 builds Resonant Engine's first fully expressive reference voice: a continuously excited, bidirectionally interacting Breath Pipe model that proves the frozen M0 air/pipe/noise continuum as a playable energetic system.

M3 is not complete when a convincing static pipe preset exists. It is complete when one model can be continuously driven through, recovered from and musically controlled across the protected continuum:

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

The same model must be able to travel back through those regimes without changing architecture, switching to hidden substitute voices or crossfading between unrelated preset engines.

## Frozen boundaries

M0, M1 and M2 remain frozen. M3 may extend the model layer through explicit decisions but may not silently weaken or reinterpret their contracts.

Breath Pipe is a reference model, not the definition of Resonant Engine. Breath-Pipe-specific concepts must not leak into generic `Engine` semantics, Host APIs or unrelated future resonators.

M3 is explicitly:

- not a Steampipe clone;
- not a browser-synth product milestone;
- not a VST/DAW product release;
- not a hardware-panel recreation;
- not a requirement for one acoustically exact real-world pipe;
- not permission to move MIDI, Web Audio, JUCE or device semantics into `resonant_core`;
- not permission to use hidden oscillators, switched presets or output effects to fake the protected continuum.

## M3 sections

| Section | Canonical title | Exit condition |
|---|---|---|
| M3.0 | Milestone Contract, Boundaries & Operating Envelope | contract and measurable envelope frozen |
| M3.1 | Breath Pipe Model / Topology Selection | bounded candidate comparison selects a topology |
| M3.2 | Pressure & Turbulence Excitation | continuous energetic excitation works from true silence |
| M3.3 | Breath-Pipe Resonator | selected resonator supports declared pitch/register range |
| M3.4 | Bidirectional Exciter ↔ Resonator Interaction | returned resonator energy can alter excitation behaviour |
| M3.5 | Regeneration & Active Feedback | passive → regenerative → self-sustaining transitions work |
| M3.6 | Nonlinearity & Feedback Spectral Shaping | nonlinear/spectral behaviour exists inside the energetic loop |
| M3.7 | Musical Macro Controls & Expressive Mapping | playable conceptual controls map nonlinearly to model state |
| M3.8 | Regime Transitions, Hysteresis & Overblow | protected continuum traverses continuously in both directions |
| M3.9 | Pitch & Register Behaviour | tuning/pressure/register behaviour meets the operating envelope |
| M3.10 | Stability, Extremes & Recovery | musical instability stays distinct from numerical failure |
| M3.11 | Polyphony, Voice Independence & CPU Scaling | independent voices and declared polyphony envelope pass |
| M3.12 | External Excitation Proof | external audio can excite/perturb the model through its real interaction path |
| M3.13 | Human B-Series Acceptance Suite | B01–B18 human listening/interaction PASS |
| M3.14 | Portability, Performance & Evidence | native/WASM/platform/performance/evidence gates PASS |
| M3.15 | Hostile Review, Completion & Freeze | blockers resolved, exact-head merge and post-merge proof complete |

## M3.0 — Operating envelope

The initial M3 acceptance envelope is intentionally concrete. M3.1 may tighten these requirements after topology evidence, but weakening one requires an explicit documented decision and justification.

### Sample rates

The reference model must behave finitely and retain the same qualitative regime ordering at:

- 44.1 kHz;
- 48 kHz;
- 96 kHz.

48 kHz is the primary human/listening and browser-Lab reference rate.

### Host block sizes

Native tests must cover at least:

- 32;
- 64;
- 128;
- 256;
- 512;
- 1024 frames.

The model must also remain valid up to the frozen M0 maximum of 4096 frames. Host block size must not become internal feedback delay. Browser evidence continues to use the Web Audio 128-frame render quantum.

### Musical pitch range

Stable-pitch acceptance is C2–C6 (MIDI 36–84). The full air/noise/overblow continuum is not required to sound identical at every note, but stable regimes must remain musically usable across that range.

When a stable fundamental exists and a target note is requested, the initial tuning criterion is:

- median absolute error ≤ 15 cents across C2–C6 at the canonical stable-pipe operating point;
- no accepted measured note > 30 cents error at that operating point.

Noise-dominant, bifurcated, unstable and overblown states are not forced into a single-fundamental tuning metric.

### Polyphony

- 4 simultaneous Breath Pipe voices: required musical acceptance target;
- 8 simultaneous voices: bounded stress/evidence target;
- the M2 eight-voice Lab allocator remains test infrastructure, not a canonical product voice-allocation policy.

### Realtime cost

At 48 kHz / 128 frames in the M2 WASM Lab on the CI reference environment:

- one sustained canonical Breath Pipe voice: smoothed process CPU target < 25%;
- four sustained voices: smoothed process CPU target < 70%;
- any sustained >100% process CPU is a realtime failure/investigation;
- eight voices are a stress gate and must remain finite/protected even if they exceed the four-voice musical budget.

M3.1 must record measured candidate costs before topology selection. A topology that meets the sonic behaviour only by making the declared musical polyphony non-realtime does not pass selection.

### Added latency

The Breath Pipe DSP may not introduce an additional host-visible buffering stage. Algorithmic sample delays inherent to the resonator are part of the model; arbitrary block accumulation used merely to make the algorithm work is prohibited.

### Realtime safety

The frozen M0 requirements remain: no process-time allocation, blocking locks, filesystem/network access or unbounded work in the model path.

## M3.1 — Model / topology selection gate

Do not assume M1's `TunedDelayResonator` is the final Breath Pipe topology merely because it already exists.

Build bounded comparable prototypes for the smallest credible candidate set, including at least:

1. an extension of the M1 tuned-delay/fractional-delay approach;
2. a bidirectional/scattering waveguide-style approach;
3. a modal or hybrid candidate if it remains computationally credible.

Each candidate must be evaluated using the same fixtures for:

- true silence at zero energy;
- faint-air/turbulence behaviour;
- pitch emergence rather than switched oscillator onset;
- stable-pipe tuning over the declared range;
- controllable unstable regions;
- overblow/register reorganisation;
- reverse traversal/recovery;
- CPU and fixed-memory cost;
- portability and deterministic behaviour;
- suitability for external excitation.

Select the minimum topology that provides the strongest expressive transitions within the operating envelope. Record the decision in an ADR. Physical realism alone is not a winning criterion.

## M3.2 — Pressure & turbulence excitation

Air is energy/excitation, not a decorative noise layer.

The exciter must support:

- true zero-energy silence;
- continuously variable pressure/drive;
- continuously variable turbulence amount and character;
- transient injection where useful;
- deterministic stochastic behaviour from explicit seeds;
- external excitation input;
- interaction input from resonator state.

Increasing pressure is not required to map linearly to output gain.

## M3.3 — Breath-pipe resonator

Implement the selected resonant topology with bounded memory/work and continuous parameter movement.

Required behaviour includes:

- continuous tuning through C2–C6;
- damping/loss changes without block discontinuities;
- stable and regenerative resonance;
- support for mode/register reorganisation;
- numerically finite response across the declared sample-rate/block envelope;
- no requirement that every future Resonant Engine model reuse this topology.

## M3.4 — Bidirectional exciter ↔ resonator interaction

The Breath Pipe must not reduce to `noise source → resonant filter`.

Returned resonator energy must be able to change the excitation/flow operating point so that coherent pitch, instability and mode transitions can emerge from interaction.

Conceptually:

```text
excitation → resonator
    ↑           ↓
    └─ interaction ─┘
```

The interaction may be physically inspired or deliberately synthetic, but it must remain coherent, bounded and expressive.

## M3.5 — Regeneration & active feedback

Provide continuous movement through:

```text
passive resonance
→ regenerative resonance
→ self-sustaining activity
→ controlled unstable/high-energy behaviour
```

Feedback amount and polarity must be model-level concepts where required by the topology. Self-oscillation is not automatically a failure.

## M3.6 — Nonlinearity & feedback spectral shaping

Spectral shaping and nonlinear interaction belong inside the energetic loop when they are intended to alter resonant behaviour.

M3 must demonstrate that these elements can change mode dominance, richness, saturation, overblow behaviour or instability. An output distortion effect applied after an otherwise unchanged resonator does not satisfy this section.

## M3.7 — Musical macro controls & expressive mapping

Separate musical controls from low-level DSP parameters.

Minimum conceptual dimensions are:

- pitch;
- pressure/drive;
- turbulence;
- interaction;
- damping/loss;
- regeneration/feedback;
- feedback colour/spectral bias;
- nonlinear drive;
- external excitation amount;
- expressive/timbre dimension where useful.

A macro control may drive several internal parameters nonlinearly. For example, `pressure` may alter exciter energy, interaction bias, feedback operating point and nonlinear response. `pressure = noise_gain` is not assumed.

MIDI semantics remain Host-side. Hosts may map velocity, mod wheel, channel/per-note pressure, MPE or hardware controls onto model concepts, but the Breath Pipe model itself receives model controls rather than MIDI concepts.

## Generic-primitive promotion rule

A Breath-Pipe-specific implementation stays inside the model unless its abstraction is independently useful outside Breath Pipe.

A component may be promoted into generic reusable `resonant_core` infrastructure only when all are true:

1. its API can be named without Breath-Pipe-specific semantics;
2. at least one credible non-Breath-Pipe use is identified;
3. promotion does not narrow the Engine/Host contracts;
4. realtime/portability evidence exists independently of one preset;
5. an ADR or equivalent architecture record explains why promotion is justified.

M3 must not slowly turn `resonant_core` into `BreathPipeEngine`.

## M3.8 — Regime transitions, hysteresis & overblow

The central M3 sonic gate is continuous, controllable traversal of the protected continuum.

Acceptance requires more than proving that each regime exists somewhere. A player must be able to:

- approach a transition;
- enter it through continuous control;
- dwell near or inside it where physically/numerically possible;
- leave it without resetting the model;
- revisit it predictably enough to be musically usable.

Forward and reverse sweeps must be measured separately. Hysteresis is permitted and may be musically desirable, but it must be observable, bounded and repeatable enough to understand.

Overblow must represent a real reorganisation of the interacting resonant system. Switching to a hidden oscillator/register preset does not pass.

## M3.9 — Pitch & register behaviour

Pitch is an explicit musical gate rather than an incidental diagnostic.

Test:

- C2–C6 stable-pipe tracking;
- tuning change with pressure at constant requested pitch;
- tuning change with damping/loss;
- register/mode movement through overblow;
- return from overblow to the requested stable register;
- pitch bend / continuous target movement through model-level pitch control.

If pressure-dependent pitch movement is intrinsic to the selected model, compensation may be model-specific. The acceptance envelope still applies at the canonical stable-pipe operating point.

## M3.10 — Stability, extremes & recovery

Musical instability and numerical failure remain separate.

Exercise:

- bifurcation/irregular oscillation;
- high feedback;
- self-sustain;
- pressure extremes;
- turbulence extremes;
- rapid parameter slams;
- wide pitch sweeps;
- repeated overblow/recovery;
- prolonged high-energy operation;
- multi-voice extremes.

Finite but chaotic/noisy behaviour may pass. NaN/Infinity or defined runaway must enter existing protected failure behaviour and recover through explicit Lab recovery semantics.

## M3.11 — Polyphony, independence & CPU scaling

Prove independent per-voice state, seed, tuning, expression and feedback behaviour.

Required evidence:

- one, two and four musical voices;
- eight-voice stress;
- independent note/control movement;
- deterministic independent seed derivation;
- no cross-voice state leakage;
- bounded voice stealing through Lab test infrastructure;
- CPU scaling evidence against the M3.0 budget.

## M3.12 — External excitation proof

Inject external audio through the actual Breath Pipe excitation/interaction path.

The test must show that external audio can excite, perturb or reorganise the resonant system without bypassing the model or being mixed directly to output as a disguised effect path.

This section is intentionally important for future Geophony/sample/live-input uses while remaining generic Resonant Engine capability.

## M3.13 — Human B-series acceptance suite

The canonical M3 human suite is:

| ID | Test | Human question |
|---|---|---|
| B01 | Silence | Is zero supplied energy genuinely silent? |
| B02 | Faint air | Can weak energy produce audible air without forced pitch? |
| B03 | Turbulence | Is irregular broadband excitation a meaningful controllable regime? |
| B04 | Pitch emergence | Does pitch emerge from interaction rather than switch on? |
| B05 | Stable pipe | Is stable resonance coherent, playable and controllable? |
| B06 | Pressure response | Does pressure reshape behaviour rather than merely scale volume? |
| B07 | Damping response | Does loss continuously alter resonance/decay without stepping? |
| B08 | Regeneration | Can passive resonance build into regenerative behaviour? |
| B09 | Self-sustain | Can the model maintain active resonance safely when energy balances loss? |
| B10 | Overblow | Can continuous drive reorganise register/mode without a hidden substitute voice? |
| B11 | Forward/reverse continuum | Can the full protected continuum be traversed both directions? |
| B12 | Hysteresis & recovery | Are direction-dependent transitions controllable and recoverable? |
| B13 | Expressive performance | Do combined macro controls feel like one interacting instrument? |
| B14 | Pitch & register | Is the declared stable range musically usable and does overblow/register behaviour make sense? |
| B15 | Aggressive/noise regime | Can high energy become aggressive/noise-dominant without mere numerical collapse? |
| B16 | Polyphony | Do simultaneous voices remain distinct, controllable and bounded? |
| B17 | External excitation | Does external audio genuinely excite/perturb the resonant system? |
| B18 | Extreme stability | Can severe conditions expose failures and recover without hiding musical instability? |

A B-series automated measurement PASS does not substitute for the human gate. Evidence must preserve automatic and human verdicts separately as in M2.

## M3.14 — Portability, performance & evidence

Reuse and extend M2 Resonant Engine Lab rather than creating another browser instrument product.

Required final evidence includes:

- Windows MSVC Debug/Release;
- Ubuntu GCC Debug/Release;
- macOS Clang Debug/Release;
- ASan+UBSan;
- no-exceptions/no-RTTI portability probe;
- deterministic native regression fixtures;
- WASM build using the same C++ model;
- native/WASM parity where a deterministic signature is meaningful;
- Chromium realtime smoke;
- Firefox realtime smoke;
- WebKit realtime smoke;
- Microsoft Edge realtime smoke;
- operating-envelope sample-rate/block tests;
- tuning/error evidence;
- CPU/polyphony evidence;
- hard-failure/protected-state evidence;
- exact source-head provenance and tested-merge provenance.

No Safari claim is implied by Playwright WebKit alone.

## M3.15 — Hostile review

The final hostile review must actively search for at least these failure modes:

- Steampipe-specific/proprietary semantic leakage;
- decorative noise pretending to be air;
- a hidden oscillator pretending to be emergent pitch;
- switched presets/crossfades pretending to be a continuous continuum;
- output distortion pretending to be resonant-loop nonlinearity;
- host-specific DSP;
- host-block-dependent feedback timing;
- Breath-Pipe concepts contaminating generic Engine APIs;
- MIDI concepts entering the model boundary;
- generic-primitive promotion without reusable justification;
- numerical runaway being labelled musical instability;
- physically impressive but musically dead behaviour;
- interesting transitions existing only in unusably tiny parameter regions;
- CPU cost invalidating the declared musical polyphony;
- external excitation bypassing the real interaction path.

## Completion gate

M3 may be declared FINAL PASS only when:

1. all M3.0–M3.15 sections are complete;
2. model/topology selection is evidence-backed;
3. the operating envelope is met or any explicit approved amendment is recorded;
4. the protected continuum is traversable continuously in both directions;
5. B01–B18 human acceptance passes;
6. automated native/WASM/platform/performance/stability gates pass;
7. the final hostile review has no unresolved blocker;
8. an exact candidate head is frozen and separately authorised for merge;
9. the authorised head merges without undeclared tree changes;
10. post-merge `main` CI and state reconciliation pass;
11. M3 documentation records FINAL PASS and M3 is frozen.

## Instrument criterion

M3 passes when the Breath Pipe behaves like an energetic instrument that can be pushed, balanced, disturbed and recovered:

> add energy → interaction emerges → pitch organises → resonance fights back → modes reorganise → instability remains playable → the system can be recovered and traversed again.

A technically respectable physical model that is static, unresponsive or dependent on preset tricks does not pass M3.
