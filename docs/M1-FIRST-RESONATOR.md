# M1 — First Resonator

Status: **IMPLEMENTATION CANDIDATE**

M1 turns the frozen M0 portable foundation into the first genuinely musical concrete resonant model.

M1 does **not** build Breath Pipe. It proves that the M0 Engine/core contracts can host a useful continuously excitable, tunable, passive-to-regenerative resonator without Host-specific DSP or architectural rewrites.

## Sonic objective

A single voice must move meaningfully through:

```text
silence
→ transient ring
→ continuously excited resonance
→ stronger regenerative resonance
→ finite nonlinear/high-energy resonance
→ silence/decay
```

The voice must be useful enough to **play as an instrument primitive**, not merely pass numerical tests or produce acceptable offline renders.

## Selected model

M1 selects the bounded tuned-delay implementation recorded in `docs/decisions/ADR-0023-M1-FIRST-RESONATOR.md`.

Signal path:

```text
external audio ─┐
trigger ────────┼→ ContinuousNoiseExciter ─→ TunedDelayResonator ─→ output
pressure/noise ─┘             ↑                       │
                              └──── bounded return ───┘

TunedDelayResonator loop:

fractional delay
→ frequency-dependent damping
→ passive loss
→ active regeneration
→ optional nonlinearity
→ bounded write-back
↺
```

The selected model is one implementation of the generic M0 `Exciter` and `Resonator` concepts. Delay-based synthesis is not made universal.

## Native M1 controls

`FirstResonatorVoice` exposes these core parameter identifiers:

| ID | Control | Native range | Default | Meaning |
|---:|---|---:|---:|---|
| 101 | tuning_hz | model-clamped | 220 Hz | target resonant tuning |
| 102 | damping | 0..1 | 0.30 | high-frequency loss + passive loss |
| 103 | feedback | 0..1.5 | 0 | active regeneration above passive loop |
| 104 | nonlinearity | 0..1 | 0.20 | blend from linear loop to soft saturation |
| 105 | excitation | 0..1 | 0 | continuous internal-noise drive |
| 106 | turbulence | 0..1 | 0.65 | noise brightness/directness |
| 107 | interaction | 0..1 | 0 | amount of resonator return presented to exciter |

`Pitch` events target tuning in Hz for this model. `Pressure` targets continuous excitation. `Trigger` and `NoteOn` create a bounded instantaneous impulse; sound does not require `NoteOn`. `NoteOff` is not required to terminate a continuously excited model.

## Tuning range and memory

The delay storage is fixed at 16,384 float32 samples = 65,536 bytes per resonator.

The minimum tuning is the greater of 24 Hz and the frequency supported by the fixed delay at the active sample rate. The upper target is the lesser of 8 kHz and 45% of sample rate.

Linear fractional-delay interpolation permits continuous non-integer delay lengths. In-loop damping/interpolation change loop phase, so M1 does not claim final phase-compensated acoustic pitch accuracy.

## Determinism

The M1 voice owns an explicit 64-bit seed supplied at construction. `ContinuousNoiseExciter` uses the M0 PCG32 generator. Reset returns the RNG, delay, feedback, smoother and diagnostic state to their initial deterministic state.

Required property:

```text
same configuration + same seed + same event sequence
→ same sample sequence on the same floating-point target
```

Cross-platform results remain tolerance-conformant rather than promised bit-identical.

## Real-time rules

M1 inherits every M0 hard real-time invariant.

During `Engine::process()` the M1 path must perform:

- no allocation/deallocation;
- no locks;
- no filesystem/network access;
- no blocking I/O;
- no unbounded queues or graph traversal;
- bounded sample work only.

Delay storage is fixed-capacity in the model object. Parameter movement is performed by the existing sample-rate-aware core smoothers.

## Stability model

M1 explicitly permits:

- passive decay;
- regenerative behaviour;
- near/self-sustaining behaviour;
- loud or unstable-sounding finite states.

It does not permit NaN/Infinity propagation. Musical nonlinearity and emergency finite-state containment are separate mechanisms. Existing `EnergyMonitor` diagnostics observe excitation, resonator, output and returned-feedback energy.

## Offline regression scenes

`resonant_m1_render` provides six canonical deterministic scenes: `passive-pluck`, `continuous`, `feedback`, `nonlinear`, `sweep`, and `silent`.

Their verifier checks WAV structure, silence/non-silence, gross level/clipping and passive-decay behaviour. These renders are **machine regression evidence only**. They are not the M1 human acceptance method.

## Interactive play-test host

`hosts/browser/m1/` compiles the real C++ `FirstResonatorVoice` to WebAssembly and runs it in an AudioWorklet. The JavaScript layer handles browser transport, UI and MIDI only; it does not reimplement synthesis.

The M1 human gate is performed by playing this live host with:

- computer-keyboard or MIDI pitch input;
- transient trigger/pluck excitation;
- continuous excitation without NoteOn;
- turbulence movement;
- damping movement;
- active regeneration movement;
- in-loop nonlinearity;
- resonator-to-exciter interaction;
- reset/recovery.

The play test must establish that the voice responds as a useful resonant instrument primitive and is materially beyond the M0 `ReferenceFeedbackProbe` in actual interaction.

## Test matrix

M1 requires evidence for:

- `Exciter`, `Resonator` and `EngineModel` concept compatibility;
- prepare/reset/lifecycle;
- silence;
- trigger excitation;
- arbitrary external-audio excitation;
- continuous excitation without NoteOn;
- fractional tuning at multiple sample rates;
- deterministic same-seed repeatability;
- seed variation;
- deterministic reset;
- passive versus regenerative late energy;
- aggressive finite nonlinear operation;
- block-size variation;
- 44.1/48/96/192 kHz processing;
- randomized parameter/event stress;
- zero dynamic allocation in the demonstrated real-time path;
- deterministic offline regression fixtures;
- M0 regression suite preservation;
- Debug/Release and warnings-as-errors;
- GCC/Clang/MSVC CI;
- ASan+UBSan;
- no-exceptions/no-RTTI core compile probe;
- Emscripten build of the live AudioWorklet/WASM acceptance host.

## M1 non-goals

M1 does not include:

- production Breath Pipe;
- a reed/jet/lip physical exciter;
- arbitrary resonator graphs;
- coupled-resonator scheduling;
- production browser UI;
- production VST3/JUCE wrapper;
- production embedded firmware;
- full MIDI/MPE mapping;
- polyphony;
- preset serialization;
- SIMD optimization;
- a general oversampling framework;
- a final 1.0 ABI;
- Geophony integration;
- a claim that linear interpolation is the final tuning solution.

## Completion rule

`docs/M1-COMPLETION.md` is the authoritative evidence gate. M1 may be declared FINAL PASS only after exact-head CI, hostile architecture review, deterministic regression evidence and the **interactive human play-test gate** are complete, followed by the protected merge decision.
