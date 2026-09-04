# ADR-0027 — M4 Direct VST3 Reference Host

Status: **ACCEPTED FOR M4**

## Context

Resonant Engine has now proved the shared C++ core in native tests and in the browser Lab through WASM/AudioWorklet. The product contract gives browser, DAW/plugin and embedded hosts equal architectural standing.

The next unproven first-class host family is the DAW/plugin target.

M0 ADR-0021 already requires VST3 integration to remain a thin Host wrapper around `resonant_core`. M4 must implement that promise without turning a plugin framework into a new architectural dependency.

The current Steinberg VST3 SDK is MIT-licensed and provides CMake-based plugin support, helper classes and validator tooling. A separate framework is therefore not required for the first reference-host proof.

## Decision

M4 will implement the first DAW reference host using the **Steinberg VST3 SDK directly** under `hosts/vst3/`.

JUCE is not introduced by M4.

The VST3 wrapper may depend on Steinberg SDK interfaces/helper classes. `resonant_core` may not.

The plugin will initially rely on host-provided generic parameter UI rather than a custom editor. This keeps the milestone focused on lifecycle, audio/event translation, automation, state, realtime safety and parity.

## Consequences

- the MIT core remains free of plugin-framework types and licensing coupling;
- the first DAW proof has a small, inspectable dependency boundary;
- VST3 parameter/event semantics are translated into existing portable structures rather than becoming core semantics;
- a custom GUI can be considered later without contaminating the audio-host proof;
- JUCE, CLAP, AU and other wrappers remain valid future options but need separate evidence/decisions;
- M4 must add a host-neutral versioned state contract rather than making VST3 chunk serialization the engine's canonical state format.

## Rejected for M4

### JUCE wrapper

Not rejected forever. It is unnecessary for the first host proof and would add a broader framework/licensing surface before there is evidence that M4 needs its GUI, cross-format or utility layers.

### Browser-as-plugin bridge

Rejected because it would make the already-proven browser host an intermediary for DAW integration and would not prove the native DAW boundary.

### Plugin-specific DSP fork

Forbidden by ADR-0002 and ADR-0021.

### Custom GUI first

Rejected because UI does not prove the host/core translation contract and would materially increase scope before plugin lifecycle and audio correctness are established.

## Acceptance

**ACCEPTED FOR M4.**

The acceptance condition is satisfied on implementation head `61b842d00ec910c4404d4df2b4caf17ceb7358a3`: Windows/macOS Debug+Release direct-SDK builds and Steinberg validator pass in M4 VST3 Build #9, while Oracle CI #155 Release passes the SDK-free adapter and VST3/core dependency-boundary tests. No `core/**` implementation file changed in the slice.
