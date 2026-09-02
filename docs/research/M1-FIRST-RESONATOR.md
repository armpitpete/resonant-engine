# M1 — First Resonator Research Findings

Status: **RECORDED FROM M1 CANDIDATE EVIDENCE**

This document extends the research record without modifying the frozen M0.19 record. Normative M1 decisions live in ADR-0023 and `docs/M1-FIRST-RESONATOR.md`.

## First-model selection finding

A tuned-delay model is the strongest first implementation test for the M0 architecture because it simultaneously exercises:

- sample-level feedback independent of Host block size;
- continuously variable non-integer tuning;
- frequency-dependent loop loss;
- passive and active/regenerative operation;
- nonlinear in-loop processing;
- deterministic continuous and transient excitation;
- fixed-memory real-time constraints.

A modal bank remains a strong later model family, especially for structures and multi-mode bodies, but would leave more of the fractional-delay/feedback-phase architecture untested in M1.

## Linear fractional-delay finding

M1 uses the two-tap linear interpolator

```text
y = x[n] + fraction * (x[n+1] - x[n])
```

because it gives continuous delay motion with two reads, no table and no dynamic state.

For a 48 kHz target, theoretical worst-case interpolation magnitude loss over all fractional positions is approximately:

| Frequency | Worst-case magnitude loss |
|---:|---:|
| 110 Hz | 0.0026% |
| 220 Hz | 0.0104% |
| 440 Hz | 0.0415% |
| 1 kHz | 0.214% |
| 4 kHz | 3.41% |
| 8 kHz | 13.40% |

The corresponding worst-case phase error is very small at low/mid fundamentals and becomes more relevant toward the top of the M1 tuning range.

**Interpretation:** linear interpolation is a defensible M1 proof primitive, but not a final high-frequency fidelity decision. Higher-order Lagrange/all-pass or other interpolation remains justified research if listening or tuning measurements expose a real limitation.

## Damping/tuning finding

The in-loop one-pole damping stage changes both magnitude and phase. Therefore a nominal `tuning_hz` maps to a geometric delay target, not an exact acoustic fundamental after every damping/feedback setting.

This confirms an M0 design decision: pitch compensation belongs to the resonator model, not to the generic Engine.

A later tuning-quality milestone should measure fundamental error across:

- tuning frequency;
- damping;
- active regeneration;
- nonlinearity;
- sample rate;
- interpolation family.

## Oversampling finding

M1 needs a bounded nonlinearity to exercise high-energy feedback, but current engineering evidence does not justify making oversampling a generic requirement.

The correct next step is evidence-driven:

1. listen for objectionable aliasing in aggressive/high-pitch states;
2. measure spectra if a problem is heard;
3. compare local oversampling strategies inside the affected model;
4. only then decide whether a reusable core primitive is warranted.

This avoids turning a possible model implementation detail into premature architecture.

## Fixed-memory finding

The M1 delay holds 16,384 float32 samples = **65,536 bytes** per resonator instance.

This is intentionally generous for one reference voice because it retains approximately a 24 Hz minimum target at the M0 maximum 384 kHz sample rate.

It is **not** a final polyphonic memory budget. A future multi-voice/embedded milestone should evaluate:

- maximum required low frequency per model;
- host-supported sample rates;
- voice count;
- shared versus per-voice state;
- memory-bandwidth cost;
- alternative capacity policies configured outside the real-time path.

## Deterministic fixture findings

Exact-head CI produced and machine-verified six one-second, mono, 48 kHz PCM16 fixtures at seed 777.

Objective inspection of the retained Release artifact found:

| Fixture | Peak (FS) | RMS (FS) | Gross observation |
|---|---:|---:|---|
| silent | 0 | 0 | exact zero-energy control |
| passive-pluck | 0.552 | 0.00467 | strong transient followed by clear decay |
| continuous | 0.214 | 0.0506 | sustained finite excitation/resonance |
| feedback | 0.202 | 0.0613 | stronger sustained regenerative energy |
| nonlinear | 0.191 | 0.0593 | aggressive bounded nonlinear state |
| sweep | 0.185 | 0.0515 | sustained energy through tuning movement |

No retained fixture reached PCM full scale.

The passive-pluck 100 ms RMS envelope fell from approximately 0.0141 FS in the first 100 ms to 0.0000165 FS in the final 100 ms, providing a clear passive-decay control case.

Spectral inspection also shows comb/resonant peaks near intended tuning families: the passive 220 Hz fixture has peaks close to 220, 440, 660 and 880 Hz; the continuous 330 Hz fixture has strong peaks near 330 Hz and its harmonics. These are engineering checks, not psychoacoustic acceptance.

## Energy-state finding

The generic M0 `EnergyMonitor` metrics remain useful, but categorical `EnergyState` values should not be interpreted as a physical classifier. For example, stored passive energy ringing after excitation stops may meet the generic heuristic used for `SelfSustaining`.

M1 therefore relies on:

- excitation/resonator/output/feedback RMS;
- peak and finite-state checks;
- long-run stress;
- controlled comparisons such as passive versus regenerative late energy;
- human listening for musical-state interpretation.

Refining state classification remains possible without changing the M0 principle that numerical failure and musical instability are different things.

## Remaining research question after M1 engineering pass

The principal unresolved M1 question is perceptual:

> Does this bounded tuned-delay/noise-excited reference model sound sufficiently useful and expressive to deserve the title “First Resonator,” or is another algorithm iteration required before the milestone is frozen?

That question cannot be truthfully replaced by CI. The retained listening fixtures are the acceptance evidence for the human gate.
