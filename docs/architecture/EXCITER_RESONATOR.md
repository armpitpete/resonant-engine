# M0.9 — Exciter & Resonator Interfaces

Status: **APPROVED**

M0 defines **compatibility contracts**, not one mandatory acoustic algorithm. C++20 concepts in `Interfaces.hpp` prove that concrete types can meet the lifecycle and sample interfaces without virtual allocation or host dependencies.

## Exciter contract

An Exciter supports `prepare(spec, seed)`, `reset()`, event handling and `processSample(ExciterInput) -> ExciterOutput`.

`ExciterInput` provides bounded slots for:

- arbitrary external audio;
- continuous pressure;
- turbulence amount;
- transient trigger;
- returned resonator signal/state proxy.

This allows continuous excitation, transient strikes/plucks, deterministic noise/turbulence, future sampled exciters, microphone/audio streams and an exciter that reacts to resonator state. A resonator output may be routed as another exciter input by a containing model; the common core does not privilege one direction.

The exciter reports sample and an operational output-energy value. Seed is supplied at prepare time when randomness is relevant; exciters that do not use randomness may ignore it.

## Resonator contract

A Resonator supports `prepare(spec)`, `reset()`, event handling and `processSample(ResonatorInput) -> ResonatorOutput`.

Inputs include audio excitation, continuous `tuning_hz`, damping, feedback and nonlinearity controls, plus a span of coupling-port values. Output includes main sample, a feedback tap and energy estimate. State remains owned by the resonator instance.

The contract does not prescribe oscillator semantics, integer delays, waveguides or modes. A tuning implementation may use fractional delays/interpolation, modal coefficient updates, waveguide length changes or a hybrid. Feedback and nonlinearity can be external composable stages or internal model topology. Diagnostics are compatible through output energy and model-specific observation APIs.

## Compatibility review

- passive resonator: feedback/control can be zero and loss positive;
- active/regenerative/self-sustaining: feedback/active interaction is permitted;
- modal resonator: no delay-line requirement;
- waveguide: sample loop and continuous tuning are permitted;
- coupled resonators: coupling ports and containing-model routing preserve bidirectionality;
- resonator-as-exciter: output/feedback taps are plain samples and can be routed onward;
- Breath Pipe: pressure+turbulence drive an interactive exciter, returned resonator energy can affect it, tuning/damping/feedback/nonlinearity remain sample-continuous.

The `ReferenceFeedbackProbe` deliberately does not claim to implement these interfaces as a final instrument; it is a topology test fixture.

**M0.9 Exciter & Resonator Interfaces: APPROVED.**
