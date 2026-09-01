# ADR-0023 — M1 First Resonator

Status: **ACCEPTED FOR M1**

## Context

M0 deliberately froze the portable Engine/core contracts without choosing a production resonator algorithm. M1 must provide the first genuinely useful concrete resonator while testing the architecture under continuous excitation, transient excitation, continuously moving tuning, passive loss, regenerative feedback, controlled high-energy operation and deterministic real-time execution.

The first resonator must not become the universal `Resonator` definition and must not overfit the protected future Breath Pipe voice.

## Options considered

### Modal resonator bank

Advantages:

- fixed and predictable CPU/memory;
- straightforward stable pitch control;
- useful for structures, bells and bodies.

Disadvantages for M1:

- does not exercise the open fractional-delay/interpolation question;
- makes returned-state feedback and self-sustaining loop behaviour less direct;
- risks making the first milestone mainly a passive modal synthesis exercise.

### Tuned delay / simple waveguide-lineage resonator

Advantages:

- directly exercises fractional tuning and loop phase;
- naturally supports passive decay, regenerative feedback and self-sustaining regimes;
- simple enough for hostile review;
- continuously excited noise, transient impulses and external audio can all drive the same model;
- fixed-capacity storage is compatible with hard real-time and embedded targets.

Disadvantages:

- interpolation and in-loop damping alter effective pitch;
- one delay is not sufficient to model many structures;
- careless naming could accidentally imply that Resonant Engine is a Karplus–Strong engine.

### Hybrid delay + modal model

Advantages:

- broader timbral range;
- could test multiple physical abstractions immediately.

Disadvantages:

- too many interacting variables for the first algorithm milestone;
- makes failures harder to attribute;
- premature before either primitive has been proven independently.

## Decision

M1 uses a **generic tuned-delay resonator** as its first concrete model.

The implementation is `resonant::TunedDelayResonator`, composed with `resonant::ContinuousNoiseExciter` by `resonant::FirstResonatorVoice`.

This is one M1 implementation of the existing M0 concepts. It does not narrow or replace `Resonator`, `Exciter`, `EngineModel` or the Host/core boundary.

## Fractional delay

M1 uses **linear interpolation** between the two adjacent delay samples.

Reasons:

- bounded two-sample read;
- no allocation or coefficient table;
- deterministic across the existing processing model;
- low implementation risk;
- sufficient to prove continuously variable non-integer tuning.

Rejected for M1, not rejected permanently:

- all-pass fractional delay;
- higher-order Lagrange interpolation;
- windowed-sinc interpolation;
- phase-compensated or dispersion-aware tuning.

Those alternatives become justified only if measured tuning motion, phase error or audible quality shows linear interpolation is inadequate.

## Loss and feedback

The resonator loop contains:

1. fractional-delay read;
2. one-pole frequency-dependent damping;
3. passive loss;
4. controllable active regeneration;
5. optional soft nonlinearity;
6. bounded emergency finite-state containment;
7. write-back into the delay.

Passive loss and active regeneration are deliberately separate concepts. `feedback = 0` still leaves a decaying physical-style loop; increasing feedback can compensate that loss and enter regenerative/self-sustaining regimes.

The one-pole damping stage changes loop phase. Therefore requested `tuning_hz` is a control target, not a promise of phase-compensated laboratory pitch accuracy in M1. Effective-pitch compensation remains a later measured improvement.

## Excitation

`ContinuousNoiseExciter` supports:

- deterministic internal noise from the M0 PCG32 source;
- continuous pressure/drive amount;
- turbulence/noise-colour amount;
- instantaneous trigger input;
- arbitrary external audio;
- a bounded returned-resonator interaction.

This is intentionally **not** a reed, jet, lip or air-column model. The returned-state slot proves bidirectional interaction remains usable without defining Breath Pipe behaviour.

## Memory bound

`TunedDelayResonator` contains exactly 16,384 float32 delay samples: **65,536 bytes** of delay memory per resonator instance.

At the M0 maximum sample rate of 384 kHz this retains a minimum tuning target of approximately 24 Hz while avoiding process-time allocation. This is a reference-model bound, not a final polyphonic memory budget.

## Nonlinearity and oversampling

M1 includes a bounded soft nonlinearity in the resonator loop because nonlinear regenerative behaviour is part of the milestone.

M1 does **not** add a general oversampling framework. Oversampling remains deferred until render/listening evidence shows aliasing is a practical blocker. Adding it pre-emptively would enlarge both API and CPU scope without evidence.

## Consequences

- M1 directly tests the hardest M0 feedback/timing claims with a real model.
- Browser/VST/embedded Hosts still require no synthesis fork.
- later modal, coupled, waveguide and impossible-body models remain legal;
- future interpolation or tuning compensation may change this model without replacing the Engine;
- production Breath Pipe remains a later milestone.

## Supersession rule

A future resonator may replace this implementation as the preferred musical model. Doing so does not supersede M0 invariants. This ADR is superseded only if the rationale for the M1 reference model itself changes.
