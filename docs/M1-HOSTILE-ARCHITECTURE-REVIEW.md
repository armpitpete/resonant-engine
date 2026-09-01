# M1 — First Resonator Hostile Architecture Review

Status: **PASS — NO M0 INVARIANT VIOLATION FOUND**

This review attacks the concrete M1 model rather than assuming that passing M0 contracts proves a real resonator will fit them.

## Candidate under review

- `ContinuousNoiseExciter`
- `TunedDelayResonator`
- `FirstResonatorVoice`
- existing frozen M0 `Engine`

The review does not promote these M1 classes into the universal synthesis architecture.

## Attacks

### Force Host-specific synthesis DSP

**PASS.** All excitation, delay, damping, regeneration, nonlinearity, returned-state interaction, smoothing, deterministic randomness and energy observation remain under `core/`. The M1 renderer is only an offline Host around the same core model.

No Web Audio, JavaScript, JUCE, VST, device, filesystem or GUI dependency was added to `resonant_core`.

### Force Host-block feedback latency

**PASS.** The tuned-delay read, damping, active regeneration, nonlinearity and write-back execute in `TunedDelayResonator::processSample()`. Host block size never becomes loop delay.

A dedicated regression renders the same deterministic M1 sequence with one-sample and 64-sample Host blocks and requires exactly equal signatures on the same target.

### Force NoteOn or oscillator semantics

**PASS.** Zero-energy processing remains silent. A `Pressure` event alone can start persistent deterministic-noise excitation. `Trigger` can excite a passive ring. External audio can excite the same resonator. No oscillator exists in the M1 voice.

`NoteOn` is accepted only as another bounded transient source and is not required acoustically.

### Force integer-only tuning

**PASS.** Tuning in Hz maps to a double-precision delay length and reads adjacent float samples with linear fractional interpolation. At 48 kHz, 440 Hz maps to 109.0909... samples rather than an integer delay.

No Host performs pitch quantisation.

### Force one sample rate

**PASS.** Delay length is computed from the prepared sample rate. Tests cover 44.1, 48, 96 and 192 kHz processing, while direct prepare coverage also includes 384 kHz.

The fixed 16,384-sample storage keeps the low target near 24 Hz even at the M0 maximum sample rate.

### Force process-time allocation

**PASS.** Delay state is an in-object `std::array`. Events, smoothers, exciter state and diagnostics are fixed-memory. A dedicated global-allocation-counting test wraps the demonstrated `Engine::process()` path and requires zero allocations.

### Force unbounded work

**PASS.** Each sample performs a bounded number of arithmetic operations, two delay reads and one write. There is no runtime graph walk, dynamic collection, retry loop, search or queue growth.

### Treat musical instability as automatic failure

**PASS.** Active regeneration is allowed above the passive loop gain, and the aggressive fixture deliberately exercises high feedback plus maximum musical nonlinearity. The long-run test requires finite output rather than requiring passive decay.

Numerical failure remains separate: non-finite internal state is rejected/contained and causes the model process call to fail safely.

### Delegate deterministic noise to Host randomness

**PASS.** `ContinuousNoiseExciter` owns M0 `Pcg32` state seeded from the voice constructor. Reset restores that exact seed. Same-seed/different-seed tests and deterministic render fixtures exercise the rule.

### Leak Breath Pipe or Steampipe semantics into generic APIs

**PASS.** The model uses generic tuning, damping, feedback/regeneration, nonlinearity, excitation, turbulence/noise amount and interaction concepts already allowed by M0. There is no Steam/Pipe/Reverberator panel model, proprietary control layout, preset format or claimed proprietary algorithm.

The M1 exciter explicitly remains a generic noise/external/transient source, not a reed or jet model.

### Promote tuned delay into universal Resonator semantics

**PASS.** The frozen `Resonator` concept remains unchanged. `TunedDelayResonator` is one implementation. ADR-0023 explicitly preserves modal, waveguide, coupled and hybrid alternatives.

No Engine method or common interface gained delay-specific state.

### Make arbitrary external excitation impossible

**PASS.** Caller-owned input audio continues through the frozen Engine and is presented directly to `ContinuousNoiseExciter`. The unit suite proves an input impulse can excite the delay body.

### Make future resonator coupling impossible

**PASS WITH M1 SCOPE LIMIT.** M1 does not implement coupled scheduling. The existing `ResonatorInput::coupling_inputs` contract is unchanged, and the selected model does not alter Engine ownership or timing in a way that prevents a containing future model from using coupling ports.

Ignoring coupling inputs in this single-node reference implementation is a model limitation, not a core narrowing.

### Force final pitch compensation into Engine

**PASS.** The one-pole damping stage and linear interpolation alter loop magnitude/phase. M1 records that effective pitch therefore depends on model-local loop phase. No generic Engine compensation or integer-delay assumption was introduced.

Future pitch compensation can remain model-local.

### Force oversampling into the generic architecture

**PASS.** The bounded nonlinearity is sufficient for the M1 engineering fixture. A general oversampling framework is not introduced without listening/aliasing evidence. If later required, it remains a model-local DSP concern unless evidence proves otherwise.

## Real weakness found: stability labels are operational, not psychoacoustic

The M0 `EnergyMonitor` remains useful for finite-state/RMS/peak observation, but its categorical states are deliberately coarse. In particular, excitation stopping while a passive resonator continues ringing can satisfy its generic `SelfSustaining` heuristic even though the model is only decaying stored energy.

**Decision:** non-blocking for M1. Tests use measured energy, finite-state checks and passive/regenerative comparisons rather than claiming those generic labels are a physical classifier. A later milestone may refine energy-state semantics with model-aware evidence; M1 must not silently reinterpret the frozen M0 enum.

## Real weakness found: M1 tuning is a control target, not calibrated pitch truth

The linear fractional read is continuous, but interpolation and damping phase can detune the loop. At 48 kHz, the theoretical worst-case magnitude loss of two-tap linear interpolation remains tiny at low/mid fundamentals but rises toward the upper tuning range: approximately 0.04% at 440 Hz, 0.21% at 1 kHz, 3.4% at 4 kHz and 13.4% at 8 kHz.

**Decision:** non-blocking for M1. The milestone proves continuously movable tuning and useful resonance, not laboratory-grade phase-compensated pitch. Higher-order/all-pass interpolation and model-local tuning compensation remain later evidence-driven work.

## CI evidence before final documentation pass

Exact implementation head `27f458d0b2530314378666a208064f5f376de4ed` passed the complete GitHub CI workflow:

- Ubuntu Debug — PASS;
- Ubuntu Release + warnings-as-errors — PASS;
- macOS Debug — PASS;
- macOS Release + warnings-as-errors — PASS;
- Windows Debug — PASS;
- Windows Release `/W4 /WX` — PASS;
- ASan+UBSan — PASS;
- `-fno-exceptions -fno-rtti` core compile probe — PASS;
- all M0 and M1 CTest targets in those jobs — PASS.

The Release job also retained the six deterministic M1 listening fixtures.

## Decision

**PASS.** No hostile attack forces Host DSP, block-latency feedback, oscillator semantics, process-time allocation, a delay-specific Engine API, Breath-Pipe-specific core semantics or violation of a frozen M0 invariant.

The remaining milestone gate is perceptual rather than architectural: the retained fixtures still need a human judgement that the first resonator is musically useful and materially more convincing than the M0 `ReferenceFeedbackProbe`.
