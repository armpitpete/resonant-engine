# ADR-0024 — Resonant Engine Lab is a diagnostic host, not a browser synth

Status: **Accepted for M2 candidate**

## Context

M2 needs humans to hear, measure and stress the headless Resonant Engine. M1 temporarily used a browser play-test surface with keyboard/MIDI controls. Continuing that surface as a product would create pressure for browser-specific instrument behaviour and duplicate product decisions that belong to future hosts.

## Decision

M2 introduces **Resonant Engine Lab** as a bounded diagnostic and human-acceptance host.

1. The sound-generating implementation remains the shared C++ core.
2. The browser compiles that implementation to WASM and runs it in an AudioWorklet.
3. Test-only voice banking, scenario scheduling, measurement and evidence capture may live outside `resonant_core`.
4. The Lab may expose exact note, velocity, parameter and preset controls required to reproduce tests.
5. The Lab must not grow a performance keyboard, Web MIDI instrument workflow, patch-design workflow or browser-only synthesis algorithm.
6. Canonical acceptance scenarios are machine-readable and scheduled against audio frames rather than UI timers.
7. Human listening verdicts and automatic numerical/realtime results are recorded separately.
8. Browser monitor gain, visualisation and capture are host concerns and cannot alter core synthesis semantics.
9. Native/WASM parity is a CI gate so the browser cannot silently become a separate DSP implementation.

## Consequences

- humans can inspect and stress the engine without a debugger;
- future native/plugin/embedded hosts remain free to choose their own UI and voice-allocation policy;
- the Lab's fixed eight-voice allocator is explicitly test infrastructure, not canonical product polyphony;
- browser compatibility defects can be separated from core DSP failures;
- M2 can retain quantitative and listening evidence tied to an exact commit;
- a future browser instrument, if ever desired, would require a separate product decision rather than emerging accidentally from the test harness.

## Alternatives rejected

### Evolve the M1 play-test into a browser synth

Rejected because it conflates engine validation with a new product and encourages browser-specific feature creep.

### Reimplement the engine in JavaScript/Web Audio nodes

Rejected because it violates ADR-0002/0008/0020 and destroys parity evidence.

### Use only offline WAV renders

Rejected because M2 specifically needs live observability, parameter stress, CPU behaviour and human interaction.

### Put all diagnostics in `resonant_core`

Rejected because UI-oriented aggregation, browser CPU timing, plotting and evidence transport are host/test responsibilities. Cheap model energy/finite diagnostics remain in core where already required by M0.
