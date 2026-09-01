# M0.11 — Energy & Stability Model

Status: **APPROVED**

“Energy” in M0 is an **operational diagnostic**, not a promise of calibrated physical joules. It is based on sample-square/RMS and peak observations that let models/hosts reason about silence, excitation, resonator activity, feedback growth and numerical failure cheaply.

## Metrics

`EnergyMonitor` tracks an explicit bounded observation window (default 256 samples):

- excitation RMS;
- resonator RMS;
- output RMS;
- feedback RMS;
- output peak;
- NaN/Infinity flags;
- runaway flag and coarse state classification.

The classification vocabulary is Silent, Passive, Regenerative, SelfSustaining, Unstable and NumericalRunaway. Thresholds are diagnostics, not universal physical truth, and models may supply better measurements later.

```text
zero energy
   ↓ excitation
Passive ──feedback contribution──→ Regenerative
   ↑                                 ↓ sustained without input
   └──────────── loss ───────── SelfSustaining
                                     ↓ excessive growth
                                  Unstable
                                     ↓ NaN/Inf
                              NumericalRunaway
```

Musically unstable/noise-like/bifurcating behavior is legal. Numerical runaway is separate: NaN/Infinity is always failure, while large finite energy is reported so a model/host can decide whether containment is needed.

`EnergyMonitor::contain()` is an optional emergency containment primitive that converts non-finite values to zero and clamps to a supplied ceiling. It is not required as a permanent timbral limiter and must not hide a model bug during testing.

Observation is O(1) per sample, fixed-memory and can be omitted by future compile-time model choices when a deeply constrained target does not need it. Hosts observe diagnostics by reading fixed-size snapshots outside or alongside the real-time call; no logger callback is required.

Tests require finite output across rates/blocks and the reference probe exposes its diagnostics.

**M0.11 Energy & Stability Model: APPROVED.**
