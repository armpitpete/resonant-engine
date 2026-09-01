# M0.10 — Feedback Path Contract

Status: **APPROVED**

Feedback is part of resonant topology, not a generic post-effect and not a Host graph round trip.

```text
excitation/interacting exciter
        ↓
     resonator ─────→ output/diagnostics
        │
        └→ feedback tap
             ↓ gain/loss
             ↓ filter / future dispersion
             ↓ nonlinearity
             ↓ polarity/envelope/time variation
             └────────────↺ returned inside the next sample interaction
```

`FeedbackStage` is an M0 probe with gain, loss, one-pole filtering, polarity, envelope and bounded nonlinearity. It demonstrates the insertion model; it is not the only legal future feedback stage.

## Rules

- feedback is computed sample by sample inside a model; Host block boundaries may not insert one-block latency;
- gain, loss, filtering, nonlinear processing, polarity and envelope can be time varying;
- multiple stages may be composed later because feedback input/output are ordinary sample-domain signals plus state;
- future dispersion may be inserted in the path;
- active excitation may depend on the returned signal before the next resonator update;
- reset clears feedback state; initialization is finite and zero-energy by default;
- diagnostics/runaway hooks observe the returned path without requiring I/O or allocation;
- a future resonator graph can represent feedback edges using the same sample/port semantics.

## Phase and tuning consequence

Any filter, interpolator, nonlinear state or delay in a feedback path changes phase and therefore can change effective resonant tuning and stability. A model that exposes tuning in Hz must account for its own loop phase; the generic Engine must not assume that delay length alone determines pitch. This explicitly preserves future fractional-delay tuning and compensation strategies required by Breath Pipe-like systems.

## Breath Pipe review

The protected signal path fits directly: returned resonator energy can be filtered, made nonlinear and fed back into the interaction without Host latency or a Steampipe-specific API.

**M0.10 Feedback Path Contract: APPROVED.**
