# M0.16 — Testing Foundation

Status: **APPROVED**

M0 deliberately chooses a **project-local zero-dependency C++ test harness** rather than importing a unit-test framework. At this size it keeps embedded/WASM portability work free from a third-party test dependency while still producing named failure diagnostics and standard CTest results. Revisit this decision if test discovery/reporting complexity becomes material.

## Layout

- `tests/unit/` — contract/unit tests;
- `tests/property/` — bounded generative/property checks;
- `tests/regression/` — deterministic render and file-format regressions;
- `tests/fixtures/` — versioned seeds/event/render fixtures.

## M0 coverage

The suite covers construction, prepare, silence, reset, invalid lifecycle, repeated prepare/reset, deterministic RNG, independent voice-seed derivation, parameter ranges, NaN handling, smoothing, sample-accurate events, deterministic simultaneous ordering, event overflow, final-sample events, finite output, block-size variation, sample-rate variation, stereo/zero-frame behavior and demonstrated zero allocation inside `Engine::process()`.

`resonant_regression_tests` creates a deterministic floating-point signature from seed 777. `resonant_render_smoke` creates the canonical WAV and `verify_wav.py` validates RIFF/WAVE, PCM16 mono, sample-rate and data size.

## Tolerances and regression policy

Exact integer PRNG sequences and same-platform deterministic fixtures use equality. Floating-point algorithm tests should use an explicit absolute/relative tolerance chosen for the measured quantity. Future audio regressions must record engine version, seed, sample rate, block size, events/inputs and tolerances; do not use unexplained “golden WAV” equality across platforms.

Test names state behavior rather than implementation detail. Failures print a specific assertion or diagnostic before returning nonzero. CTest runs every PR job.

Allocation instrumentation is a strategy/probe, not a universal theorem: each new DSP model that enters the demonstrated real-time path must remain covered.

**M0.16 Testing Foundation: APPROVED.**
