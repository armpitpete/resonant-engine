# Resonant Engine — Normative Glossary (M0.3)

Status: **APPROVED FOR M0**

These terms are normative for Resonant Engine implementation and documentation. If code or later documentation uses one of these words differently, that difference must be made explicit rather than silently changing the meaning.

| Term | Normative meaning |
|---|---|
| **Engine** | The host-independent Resonant Engine processing system that owns/configures DSP models and processes audio/events through the portable core. It is not a DAW, browser or device wrapper. |
| **Host** | Software or hardware that supplies audio buffers, timing, events, parameters, configuration and/or platform services to the Engine. Examples: browser adapter, VST3 wrapper, embedded firmware, another synth. |
| **Voice** | One independently stateful playable or continuously driven synthesis instance. A Voice need not correspond to one MIDI note and need not contain an oscillator. |
| **Exciter** | A component/process that injects energy into a resonator or resonant system. |
| **Excitation** | The signal or energy injected by an Exciter, whether transient, continuous, external, deterministic or stochastic. |
| **Pressure** | A continuous driving-force control/concept representing available excitation energy or flow potential. It is not synonymous with amplitude or MIDI channel pressure. |
| **Turbulence** | Irregular noise-like excitation associated with flow/interaction. It may be broadband, shaped, deterministic-stochastic or model-generated. |
| **Resonator** | A stateful system that selectively stores, transforms and releases energy over time according to tuning, loss and other behavior. |
| **Active Resonator** | A Resonator whose topology may add or regenerate energy through active feedback/excitation. |
| **Passive Resonator** | A Resonator that cannot sustain net energy indefinitely without external excitation. |
| **Regenerative Resonator** | An active resonator in a regime where returned energy reinforces/extends resonance but is not necessarily indefinitely self-sustaining. |
| **Feedback** | Routing of a system's current/prior state or output back into an earlier interaction point so it influences subsequent processing. |
| **Feedback Path** | The ordered signal/state path carrying returned energy, including gain/loss, filtering, nonlinearity, polarity, delay/phase and later dispersion or coupling elements. |
| **Loss** | Net removal of energy from a resonant system. Loss may represent radiation, friction, damping or synthetic attenuation. |
| **Damping** | Frequency/time-dependent or broad reduction of resonant persistence. Damping is one mechanism of Loss; the terms are not interchangeable in all models. |
| **Dispersion** | Frequency-dependent propagation/phase behavior that causes different spectral components or modes to travel/evolve differently. |
| **Nonlinearity** | A transformation whose output is not proportional to its input and may depend on magnitude, sign or state. It may be structural to the model, not merely an effect. |
| **Saturation** | A bounded or progressively compressive nonlinearity used to shape/limit increasing signal magnitude. Saturation is one kind of Nonlinearity. |
| **Energy** | An operational DSP measure of signal/state magnitude used to reason about excitation, stored resonance, feedback, output and stability. It is not automatically a physically calibrated joule value. |
| **Stability** | A regime in which state/output remains bounded and behaves within defined operational limits. Musical stability and numerical validity are related but distinct. |
| **Instability** | A regime where amplitude, spectrum, pitch, mode dominance or behavior becomes irregular or changes state. Musical instability may be intentional; numerical runaway is not. |
| **Coupling** | Bidirectional or directional transfer of energy/state between resonant or exciting systems. |
| **Resonator Graph** | A future topology containing multiple resonant/exciting processing elements connected by directed or bidirectional relationships. M0 preserves this possibility without implementing arbitrary graphs. |
| **Node** | One processing/state element in a Resonator Graph, such as resonator, exciter, coupling element or feedback stage. |
| **Edge** | A connection carrying signal/state/control/energy between Nodes in a Resonator Graph. |
| **Movement** | Perceptible evolution of synthesis behavior over time caused by changing control, interaction, state or modulation. Movement is a musical/design concept, not a specific modulation source. |
| **Modulation** | Intentional variation of a Parameter or internal quantity over time from a defined source/routing. |
| **Parameter** | A named controllable value with defined identity, range, representation and update semantics. |
| **Event** | A timestamped discrete instruction/change delivered to processing, such as note-on, note-off, trigger or sample-accurate parameter change. |
| **State** | Information retained across processing calls that influences future output. State may be DSP-ephemeral or serializable configuration/preset state. |
| **Preset** | Serializable user/model configuration intended to reproduce a defined setup. A Preset is not identical to live DSP State. |
| **Processing Block** | A finite contiguous group of audio frames supplied by a Host in one process call. Internal model feedback may still be evaluated sample by sample. |
| **Sample** | One discrete audio value per channel at one Sample Rate instant. |
| **Sample Rate** | Number of audio samples/frames per second used by processing, expressed in Hz. |
| **Control Rate** | Update rate of a control process slower or otherwise distinct from Audio Rate. Control-rate use must not prevent sample-accurate paths where required. |
| **Audio Rate** | Processing/update at the per-sample rate of the audio stream. |
| **Determinism** | Property that defined initial state, inputs, events, configuration and Seed produce reproducible behavior within the documented cross-platform guarantees. |
| **Seed** | Explicit integer/state input used to initialize deterministic pseudo-random behavior. |
| **Render** | Processing the Engine outside a live audio callback to generate a reproducible finite audio result, normally through the offline render host. |
| **Conformance** | Demonstrated compliance of an implementation/host/model with a specified Resonant Engine contract or acceptance gate. |

## Commonly confused distinctions

### Engine vs Host

The **Engine** owns portable DSP behavior. The **Host** adapts platform-specific audio, events, controls and services. If a synthesis rule changes between browser and VST implementations, that is evidence the boundary has been violated.

### Exciter vs Excitation

An **Exciter** is the process/component. **Excitation** is the energy/signal it produces or injects.

### Pressure vs expression event

**Pressure** is a synthesis concept/control. A MIDI/poly-pressure event may be mapped to Pressure, but the two are not synonymous.

### Turbulence vs noise generator

**Turbulence** is the sonic/interaction concept. A deterministic noise generator may contribute to it, but white noise by itself does not define turbulence.

### Resonator vs oscillator

A **Resonator** stores/selects/evolves excitation energy. It may oscillate, self-sustain or produce pitch, but the core must not assume an oscillator is the source of pitch.

### Loss vs damping

**Loss** is general net energy removal. **Damping** describes one temporal/spectral mechanism of loss.

### Nonlinearity vs saturation

**Nonlinearity** is the broad category. **Saturation** is one bounded/compressive nonlinear behavior.

### Stability vs numerical validity

A voice may be musically **unstable** while remaining numerically finite and safe. NaN/Infinity propagation or uncontrolled unbounded state is a numerical failure, not a desirable unstable timbre.

### Movement vs modulation

**Movement** is the audible evolution. **Modulation** is one mechanism that can create movement. Continuous physical interaction may create movement without a conventional LFO/modulation route.

### State vs preset

**State** includes transient runtime information. A **Preset** contains serializable configuration. Delay-line contents, current resonant energy or PRNG cursor are not automatically user preset data.

### Processing block vs feedback interval

A **Processing Block** is a host delivery unit. It does not define the timing of internal feedback; a model may run a closed loop sample by sample inside a block.

### Control rate vs sample accuracy

A control may be generated at **Control Rate**, but important changes may be interpolated or delivered as sample-offset events. `Control rate` must never be used to justify unavoidable block stepping.

### Determinism vs bit identity

**Determinism** means reproducibility under the documented contract. It does not automatically promise bit-identical floating-point output across every compiler/architecture unless a test explicitly requires that.

## Approval

The glossary has been reviewed for ambiguous overlaps and the distinctions above are normative.

**M0.3 Core Terminology & Concept Model: APPROVED.**
