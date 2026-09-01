# Resonant Engine — Architectural Invariants

These rules are normative for M0 and later work unless superseded by an explicit accepted architecture decision.

1. **One DSP core serves all hosts.** Browser, DAW/plugin, embedded and instrument hosts must not carry divergent copies of core synthesis DSP.
2. **The core is host-independent.** Browser APIs, Web Audio, JavaScript, JUCE, VST SDKs, device APIs, GUI frameworks and OS services remain outside `resonant_core`.
3. **C++20 is the core language baseline.**
4. **Hosts may process blocks; resonant feedback may operate sample by sample inside the core/model.** Host block size must not become feedback-loop latency.
5. **Continuous excitation is first-class.** A resonant model must not require note-on or a conventional oscillator to exist acoustically.
6. **Passive and active resonators are both first-class.** The architecture must represent passive decay, regenerative behavior, self-sustaining behavior and controlled musical instability.
7. **Feedback is topology, not merely an effect.** Filtering, nonlinearity and later dispersion may live inside feedback paths.
8. **Exciters and resonators may interact.** Future models must be able to let returned resonator state affect excitation behavior.
9. **Air / pipe / noise is a primary sonic priority.** No generic abstraction may prevent the Breath Pipe Reference Voice or its required continuum.
10. **Pipe-specific semantics do not belong in the generic engine.** Pressure/turbulence may become reusable concepts, but no Steampipe-specific API or fixed pipe algorithm is allowed to define the core.
11. **Sample-accurate movement must remain possible.** Events, pressure, tuning, feedback and expressive controls may need sample-level change.
12. **The real-time path must be bounded.** No required allocation, deallocation, locks, blocking I/O, filesystem/network access or unbounded work may occur during processing.
13. **Numerical failure is distinct from musical instability.** Aggressive, unstable or noisy sound is allowed; NaN/Infinity propagation and uncontrolled numerical runaway are not.
14. **Deterministic operation must be possible.** Random/turbulent processes must support explicit deterministic state/seed behavior independent of host randomness.
15. **Hosts remain thin adapters.** Essential physical-model dynamics, smoothing required by the model, feedback behavior and random generation must not migrate into host code.
16. **Future resonator coupling must remain possible.** Core interfaces must not make another resonator-as-exciter, bidirectional coupling or later resonator graphs require a wholesale rewrite.
17. **Arbitrary external audio excitation must remain possible.** Microphones, samples, streams or another synth may later provide excitation without changing the core architecture.
18. **Observability is part of the architecture.** Energy, finite-state health and runaway/stability diagnostics must be exposable to hosts without owning the DSP.
19. **M0 does not freeze algorithms that have not been proven.** Waveguide, modal, tuned-delay, coupled-network and hybrid implementations remain model choices.
20. **A host target may add an adapter, never a second synthesis architecture.**
