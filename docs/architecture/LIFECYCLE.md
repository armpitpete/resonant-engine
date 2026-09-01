# M0.13 — State & Lifecycle Contract

Status: **APPROVED**

## Lifecycle

```text
Constructed (unprepared)
    │ valid prepare
    ▼
Prepared/Reset ──process──→ Prepared/Running state
    │    ▲                    │
    │ reset ──────────────────┘
    │
    ├─ valid re-prepare → Prepared/Reset with new configuration
    └─ failed prepare → Unprepared/fail-safe

Destruction is legal from any state off the audio thread.
```

Construction creates no active audio configuration. `prepare()` validates `ProcessSpec`, permits model setup outside the audio thread and ends in deterministic reset state. `process()` is legal only after successful preparation. `reset()` before prepare is rejected; after prepare it clears ephemeral DSP/feedback/energy state while retaining the prepared configuration.

Sample-rate or maximum-block-size changes use re-prepare rather than mutating coefficients/buffers unpredictably in process. A failed reconfiguration invalidates the demonstrated Engine until a valid prepare succeeds.

## State ownership

- **parameter state**: stable semantic control values/metadata owned by model/configuration;
- **voice state**: per-voice DSP state owned by the containing engine/model;
- **DSP state**: delay/filter/feedback/phase/PRNG continuation/energy state; ephemeral unless a later exact-resume format chooses to serialize it;
- **preset state**: portable user-intent values such as model selection, public parameters and optionally seed;
- **configuration/topology state**: non-real-time objects that may be built/replaced safely outside processing.

A future graph replacement is prepared separately and handed over at a Host-defined safe boundary. M0 does not require in-place graph mutation on the audio thread.

State formats will carry an explicit schema/version. Pre-1.0 migration may be breaking but must be documented. The M0 code establishes the concept rather than freezing a serialization format prematurely.

Tests cover construction, process-before-prepare, prepare, reset, repeated prepare/reset, invalid prepare and configuration changes.

**M0.13 State & Lifecycle Contract: APPROVED.**
