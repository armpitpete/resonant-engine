# M0.19 — Research & Reference Record

Status: **APPROVED**

This directory records observations and research prompts. It is **not** the product specification. Normative decisions live in `docs/architecture/` and `docs/decisions/`.

## Erica Synths Steampipe — reference, not specification

Primary references:

- Erica Synths product page: https://www.ericasynths.lv/steampipe-3153/
- Erica Synths July 2025 manual: https://www.ericasynths.lv/media/Steampipe_manual_July_2025.pdf
- Erica Synths development/product description: https://www.ericasynths.lv/news/post/subscribe-steampipe

Observed broad ideas useful to research:

- oscillator-free physical-model synthesis centered on excitation + resonant feedback;
- a “steam” excitation section described in terms of airflow/force and DC/noise, with external audio able to replace the normal excitation source;
- a tuned delay/resonator section whose returned signal can push/pull the generator and sustain oscillation;
- feedback filtering and saturation as structural loop controls;
- saturation symmetry/polarity as harmonic-shaping mechanisms;
- “stiffness”/dispersion-like control that spreads harmonics in the delay body;
- a split resonator/diffuser-delay concept that can create more two-dimensional/bell-like responses;
- extensive modulation/performance control rather than one static acoustic emulation;
- tuning is affected by multiple resonator/feedback parameters: the manual explicitly provides a tuning function because editing can detune the instrument. This is evidence that loop filtering/phase and resonator parameters interact with effective pitch.

The manufacturer describes three user-facing sections (Steam, Pipe, Reverberator), eight-voice polyphony and a physical-model engine. Those observations are useful evidence that continuously excited resonant synthesis is musically viable, but Resonant Engine must not copy the panel, parameter naming, proprietary implementation, preset system or assumed internal algorithm.

**Design distinction:** Resonant Engine’s architecture is intentionally broader. The same core must support strings, passive structures, coupled resonators, active/self-sustaining systems and impossible synthetic bodies. The protected Breath Pipe voice is our architecture test, not a request to reproduce Steampipe.

## Karplus–Strong / tuned-delay lineage

- Kevin Karplus & Alex Strong, “Digital Synthesis of Plucked-String and Drum Timbres,” *Computer Music Journal* 7(2), 1983, pp. 43–55. DOI: https://doi.org/10.2307/3680062
- David A. Jaffe & Julius O. Smith, “Extensions of the Karplus-Strong Plucked-String Algorithm,” *Computer Music Journal* 7(2), 1983, pp. 56–69. DOI: https://doi.org/10.2307/3680063
- “Plucked-String Models: From the Karplus-Strong Algorithm to Digital Waveguides and Beyond,” *Computer Music Journal* 22(3), 1998 (survey/reference lineage).

Research relevance: delay length, loop filtering, fractional tuning, decay control and dispersion are reusable concepts, but M0 does not freeze a Karplus–Strong resonator as the universal model.

## Digital waveguides / wind excitation

Reference lineage:

- Julius O. Smith III, *Physical Audio Signal Processing* / digital-waveguide literature (online text and related publications).
- Perry R. Cook, *Real Sound Synthesis for Interactive Applications* and wind-instrument physical-model literature.
- Gary P. Scavone and related digital-waveguide wind-instrument work (reed/jet nonlinear excitation and bore interaction).

Research relevance: wave variables, traveling-wave delay lines, scattering, fractional delay tuning, frequency-dependent loss and nonlinear excitation/resonator interaction. Breath Pipe may use these ideas later, but M0 keeps the interface compatible with modal or hybrid alternatives.

## Nonlinear resonators

Relevant questions from physical-model synthesis literature:

- nonlinear excitation can transfer energy between modes/registers;
- saturating or nonlinear loop elements can control/bifurcate self-oscillation;
- oversampling or other anti-alias strategies may be needed when nonlinearities sit inside feedback;
- “musically unstable” and “numerically unstable” are distinct engineering categories.

M0 deliberately leaves the exact nonlinearity and anti-alias strategy to later algorithm milestones while preserving an insertion point and observability.

## Feedback-delay / network references

- Jean-Marc Jot & Antoine Chaigne, “Digital Delay Networks for Designing Artificial Reverberators,” AES 90th Convention, 1991, preprint 3030.
- Feedback-delay-network literature is relevant because it demonstrates bounded networks of delays, mixing, feedback and frequency-dependent loss. It is **not** evidence that Resonant Engine should become a reverberator architecture.

Research relevance: multi-stage feedback, lossless/passive structures, coupled delays, matrix mixing, stability conditions and future resonator graphs.

## Open research questions after M0

1. Which first resonator best tests expressive continuous excitation without overfitting to Breath Pipe?
2. Which fractional-delay/interpolation family gives useful tuning motion at acceptable CPU/phase error?
3. How should pressure and turbulence interact with returned resonator state for reed/jet-like behavior?
4. Which nonlinearities remain musically useful and controllable near self-oscillation?
5. When does internal oversampling become necessary for nonlinear feedback?
6. Which energy metrics correlate with musically useful transition regimes?
7. How should coupled resonator ports represent wave/force/velocity-like quantities without freezing one physical formalism?
8. How much floating-point determinism is practical across native/WASM/embedded targets?

## Intentionally deferred beyond M0

- production Breath Pipe implementation;
- production waveguide/modal algorithms;
- arbitrary graph editor/runtime graph mutation;
- complete MPE/MIDI mapping;
- oversampling implementation;
- SIMD optimization;
- psychoacoustic conformance metrics;
- final preset serialization;
- production browser/VST3/embedded wrappers.

**M0.19 Research & Reference Record: APPROVED.**
