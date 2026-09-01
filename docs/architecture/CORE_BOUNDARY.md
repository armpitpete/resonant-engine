# Portable DSP Core Boundary — M0.4

Status: **APPROVED FOR M0**

`resonant_core` is the single portable DSP dependency shared by all Resonant Engine hosts.

## Responsibilities of `resonant_core`

The core may own:

- engine/model lifecycle state;
- portable DSP algorithms and primitives;
- exciters, resonators, feedback paths and coupling abstractions;
- sample/block processing logic;
- sample-accurate event consumption;
- parameter representation/smoothing required for DSP correctness;
- deterministic pseudo-random state;
- energy/stability diagnostics;
- portable preset/state structures once defined;
- model-independent processing configuration;
- bounded real-time-safe utility code needed by DSP.

## Responsibilities outside `resonant_core`

Hosts/adapters own:

- browser and JavaScript integration;
- Web Audio and AudioWorklet lifecycle;
- VST3 SDK integration;
- JUCE integration if a wrapper chooses JUCE;
- DAW transport/plugin buses/host parameter discovery;
- USB, MIDI-device, audio-device and GPIO APIs;
- operating-system services;
- GUI/UI rendering;
- filesystem/network access;
- user file pickers and preset storage;
- platform threads and callback registration;
- device-driver or firmware peripheral code;
- host-specific logging/telemetry;
- conversion from platform events/buffers into core-facing structures.

## Forbidden dependencies inside the core

The portable core must not depend directly on:

- browser APIs;
- Web Audio;
- JavaScript/WebAssembly host glue;
- JUCE;
- VST SDK headers/types;
- DAW-specific APIs;
- USB APIs;
- MIDI-device APIs;
- GUI frameworks;
- OS-specific APIs unless contained behind an explicitly portable abstraction outside the audio-processing dependency path;
- filesystem access during DSP processing.

No public core interface may expose a host-framework type.

## Core-facing host abstraction

The core-facing boundary is deliberately small:

```text
Host adapter
    │
    ├─ configure / prepare / reset
    ├─ audio buffers
    ├─ sample-offset events
    ├─ parameter changes
    ├─ optional external audio excitation
    └─ reads bounded diagnostics
    │
    ▼
resonant_core
    │
    └─ portable state + DSP model topology
```

The exact structures are refined in M0.6–M0.8. M0.4 fixes the dependency direction and conceptual interfaces without prematurely locking channel layout, event encoding or parameter metadata.

## Audio-buffer interface

The Host supplies caller-owned audio buffers for a finite frame count. The core neither owns the Host audio device nor performs device I/O.

M0.4 requirements:

- buffer memory is provided by the caller;
- processing is bounded by an explicit frame count;
- host block delivery must permit internal sample-level feedback;
- channel/interleaving/in-place details are finalized in M0.6;
- no platform audio-buffer class appears in the core API.

## Event-input interface

The Host supplies a bounded sequence of portable events associated with positions inside the current processing block.

M0.4 requirements:

- events are data, not callbacks into the Host;
- sample-offset timing must remain possible;
- event consumption requires no host API calls;
- exact event types/order/capacity are finalized in M0.7.

## Parameter interface

The Host controls DSP through portable parameter identifiers/values or parameter events rather than through wrapper-specific objects.

M0.4 requirements:

- no VST/JUCE/Web Audio parameter object enters the core;
- sample-accurate updates must remain possible;
- smoothing needed for DSP behavior occurs in/under the core contract rather than being required from each host;
- exact normalized/native representation is finalized in M0.8.

## Optional external-audio input interface

The architecture reserves caller-owned audio input that may later excite models from microphones, samples, another synth, live streams or other resonators.

External audio is an input signal to the core, not permission for the core to open devices/files/network sources itself.

## Diagnostics-output interface

The core may expose bounded, passive diagnostic data such as:

- finite/non-finite state;
- observed energy metrics;
- clipping/runaway indicators;
- current stability classification;
- counters or flags useful for conformance testing.

Diagnostics must not require real-time logging, strings, heap allocation or callbacks into the Host.

## Configuration and lifecycle interface

The core owns portable lifecycle operations:

- construct;
- configure/prepare with portable processing specification;
- reset;
- process;
- safe non-real-time reconfiguration;
- destroy.

Exact legal transitions are finalized in M0.13. The existing `Engine` probe already demonstrates `prepare`, `reset`, sample processing and block adaptation without host types.

## Dependency direction

Allowed:

```text
browser host ─┐
vst3 host    ├──> resonant_core
embedded host┘
```

Forbidden:

```text
resonant_core -> browser / Web Audio / JUCE / VST3 / OS / device APIs
resonant_core -> a host-specific DSP implementation
```

Host wrappers may depend on `resonant_core`. `resonant_core` must never depend on a host wrapper.

## Thin-wrapper criterion

A new Host should mainly perform translation and lifecycle wiring. If implementing a Host requires recreating synthesis feedback, smoothing rules, turbulence/random behavior or physical-model state outside the core, the boundary has failed.

## Target reviews

### Browser

PASS at M0.4 level. The interface requires only memory buffers, numeric configuration and portable event/parameter data. Web Audio/AudioWorklet glue can remain outside the core and later call a WASM build of the same code.

### VST3/DAW

PASS at M0.4 level. A plugin wrapper can translate process buffers/events/parameters into the portable boundary without introducing VST SDK types into core DSP.

### Embedded

PASS at M0.4 level. Firmware may provide statically/caller-owned buffers and numeric control data. The core requires no OS, GUI, filesystem, network or device API.

These are architecture reviews, not M0.17 portability proofs.

## Dependency enforcement

CMake defines `resonant_core` without third-party link dependencies. A boundary check scans current core/public-header source for known forbidden host/framework dependencies. This is defense-in-depth; code review remains authoritative.

## Approval

The portable boundary supports all three first-class host families without requiring host-specific DSP.

**M0.4 Portable DSP Core Boundary: APPROVED.**
