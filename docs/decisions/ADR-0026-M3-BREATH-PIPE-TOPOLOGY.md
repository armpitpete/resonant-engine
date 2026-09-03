# ADR-0026 — M3 Breath Pipe Topology Selection

Status: **ACCEPTED FOR M3**

## Context

ADR-0025 requires M3 to choose the Breath Pipe resonator topology from bounded credible candidates rather than inheriting M1's tuned-delay implementation by convenience.

The M3.1 comparison evaluates three candidate families:

1. the existing M1 tuned-delay/fractional-delay family;
2. a bounded bidirectional/scattering-waveguide prototype;
3. a bounded three-mode interacting modal resonator.

The selection criteria are not physical realism alone. The winning topology must preserve true silence and deterministic bounded work while providing the strongest combination of continuous pitch, returned-state interaction, reversible high-energy behaviour, register/overblow reorganisation, external excitation suitability and realtime/fixed-memory economy.

## Decision

Select the **bounded three-mode interacting modal resonator** for the M3 Breath Pipe reference voice.

The selected model uses three damped resonant pole pairs. Pressure, regeneration, interaction and nonlinear drive continuously alter modal loss and returned-state behaviour. At high drive, loss is redistributed from the fundamental toward upper modes so overblow is a continuous reorganisation of one resonant system rather than a switch to a hidden oscillator or preset.

The modal resonator remains Breath-Pipe-specific. This decision does **not** promote it into a universal Resonant Engine primitive and does not prohibit tuned-delay, waveguide, modal-bank or hybrid models in later voices.

## Evidence

`tests/unit/test_m3_topology.cpp` is the executable selection record. It probes candidate capabilities from actual implementations rather than assigning positive capability flags by declaration. It records:

- exact zero-energy silence;
- response to supplied excitation;
- returned-state/bidirectional interaction suitability;
- continuous pitch and stable tuning where implemented;
- overblow/register reorganisation where implemented;
- reverse/recovery behaviour;
- deterministic replay;
- fixed-memory cost;
- measured per-sample probe cost as non-normative diagnostic evidence.

`tests/unit/test_breath_pipe_contract.cpp` separately verifies the selected topology at the M3 stable-pipe operating point, across the declared sample-rate/block envelope, and proves that feedback colour and nonlinear drive alter resonator state through/in the energetic loop.

The executable selection and contract tests passed with sanitizers and native/WASM parity on exact-head run #3 at `39a8708dda67d7ff3720dfc13ba307d93f780a6e` and again on post-monitor exact-head run #5 at `b7d251aa9082ae2a4226f0dfde44ce8855e47101`. The topology-selection acceptance condition is therefore satisfied.

## Alternatives

### Extend M1 tuned delay

Advantages: already mature, continuously tunable, deterministic, and proven portable. Rejected for M3 because the single tuned-delay mode does not natively provide the required continuous register-energy redistribution; adding that machinery would remove much of the simplicity advantage while retaining substantially larger fixed delay memory.

### Scattering/bidirectional waveguide

Advantages: naturally represents travelling-wave return paths and is a credible future resonator family. The bounded M3 prototype proves silence, returned energy and recovery but does not yet provide the required continuously tunable/register-reorganising behaviour. Building those features would materially increase M3 scope before the expressive reference voice is proven.

### Three-mode interacting modal resonator

Advantages: very small fixed state, direct continuous tuning, explicit observable modal energy, simple returned-state interaction and a compact continuous overblow mechanism. It satisfies the M3 reference-voice goals with the least additional topology machinery.

## Consequences

- Breath Pipe pitch is organised by damped resonant modes, not free-running oscillators.
- Overblow may be inspected directly through modal energy/radius telemetry.
- The model can remain exactly silent from reset until energy is supplied.
- M3 can keep fixed memory extremely small while retaining four-voice/eight-voice bounded operation.
- Future work may still introduce waveguide or tuned-delay voices without superseding this ADR.
- Any attempt to promote this topology into generic engine architecture requires a separate generic-primitive promotion decision under ADR-0025.

## Acceptance

**ACCEPTED FOR M3.** The topology-selection gate is closed by exact-head executable evidence. This does not by itself declare M3 FINAL PASS; M3 still requires final platform/browser evidence, final hostile review, protected exact-head merge authorisation and post-merge reconciliation.
