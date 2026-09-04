# ADR-0025 — M3 Breath Pipe Reference Voice

Status: **PROPOSED FOR M3**

## Context

M0 froze Resonant Engine as a reusable resonant/physical synthesis platform and protected the future Breath Pipe continuum. M1 proved a concrete resonator. M2 proved that the shared C++ implementation can be observed, measured and human-tested through the Lab.

The next milestone must prove that those foundations can become a genuinely expressive interacting instrument without turning Resonant Engine into one specific pipe synth.

## Decision

M3 is the **Breath Pipe Reference Voice** milestone.

The Breath Pipe is a reference model used to prove continuous energetic interaction across:

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

M3 must prove continuous controllable traversal in both directions, not merely provide static presets representing those labels.

## Model-selection gate

The final Breath Pipe topology is not predetermined by M1. M3.1 must compare bounded credible candidates, including the existing tuned-delay family and at least one bidirectional/waveguide-style alternative. A modal or hybrid candidate must also be evaluated when computationally credible.

Topology selection is based on expressive transitions, controllable instability, tuning, overblow/register behaviour, CPU/fixed-memory cost, portability, determinism and external-excitation suitability. Physical realism alone is insufficient.

## Musical controls vs DSP parameters

The model exposes conceptual controls such as pitch, pressure, turbulence, interaction, damping, regeneration and nonlinear drive. These controls may map nonlinearly to multiple low-level state variables.

No requirement exists for one macro control to equal one DSP coefficient.

MIDI, MPE, hardware and automation semantics remain Host-side. The model does not receive MIDI-specific concepts.

## Generic-primitive promotion

Breath-Pipe-specific code stays model-specific unless its abstraction is independently reusable. Promotion into generic `resonant_core` infrastructure requires a non-Breath-Pipe use case, generic naming/semantics, preserved Engine/Host boundaries and realtime/portability evidence.

## External excitation

M3 explicitly proves that external audio can excite or perturb the Breath Pipe through the real excitation/interaction path. Direct dry mixing or bypass effects do not satisfy this proof.

## Operating envelope

The initial M3 contract defines concrete sample-rate, block-size, pitch, tuning, polyphony, CPU, latency and realtime-safety targets in `docs/M3-BREATH-PIPE.md`. Those requirements may be tightened by topology evidence. Weakening them requires an explicit recorded decision rather than an undocumented implementation convenience.

## Human acceptance

M3 adds the B01–B18 human acceptance suite. Automated measurements and human verdicts remain separate evidence dimensions as established by M2.

## Non-decisions

This ADR does not:

- select the final resonator topology;
- define a product UI;
- define a VST or hardware release;
- copy Erica Synths Steampipe implementation or proprietary panel semantics;
- require acoustically exact simulation of one physical pipe;
- make Breath Pipe semantics part of generic Engine APIs;
- make the M2 Lab's eight-voice allocator a product polyphony policy.

## Consequences

- M3 begins with topology evidence rather than implementation lock-in.
- The sonic continuum becomes a measurable and human-testable milestone gate.
- Expressive macro controls can represent interacting systems instead of subtractive-style one-knob/one-coefficient mappings.
- Resonant Engine remains reusable for Geophony and other future synths.
- External excitation is proven against a real expressive model.
- M3 may introduce new reusable primitives, but only through explicit justified promotion.

## Acceptance

ADR-0025 becomes **ACCEPTED FOR M3** when the M3 contract-definition PR passes exact-head CI/review and is merged through the normal protected-authority route.
