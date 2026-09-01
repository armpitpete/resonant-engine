# Product & Scope Contract — M0.1

Status: **APPROVED FOR M0**

## Product

Resonant Engine is a portable resonant/physical synthesis platform capable of running from the same core DSP architecture in browsers, DAWs, embedded hardware and other host instruments.

M0 establishes a portable real-time DSP architecture specifically capable of supporting expressive, continuously excited resonant systems, while remaining general enough for strings, structures, coupled resonators and impossible synthetic bodies later.

## What Resonant Engine is

- a reusable DSP engine for resonant and physical-style synthesis;
- a common synthesis core intended to be embedded in multiple products and hosts;
- a foundation for passive, regenerative, self-sustaining and deliberately unstable resonant systems;
- a platform where continuous excitation, feedback and interaction can be primary synthesis mechanisms;
- a host-independent core with thin host adapters.

## What Resonant Engine is not

- a single finished synthesizer;
- a browser-only Web Audio instrument;
- a VST-only plugin;
- a JUCE framework project;
- an embedded-device firmware product;
- a fixed emulation of one acoustic instrument;
- a clone of Erica Synths Steampipe;
- a promise that every model will be physically realistic.

## Primary use cases

1. **Browser instruments** using the shared core through WebAssembly/AudioWorklet hosting.
2. **DAW instruments/effects** using the shared core through VST3 or another plugin wrapper.
3. **Embedded hardware** using the same core behind device-specific audio, control and I/O layers.
4. **Imported synthesis engines** inside other synthesizer projects without copying their DSP implementation.
5. **Geophony** as a future host able to use resonant/physical voices without owning or duplicating the core DSP.
6. Future research instruments including strings, structures, coupled resonators and impossible synthetic bodies.

## Host equality

Browser, VST3/DAW and embedded hardware are first-class architectural targets. None may become the hidden reference implementation that forces the others to imitate host-specific assumptions.

## Shared-core rule

One canonical DSP core must serve all hosts.

Host-specific DSP duplication is prohibited. A host wrapper may translate buffers, events, parameters, device state or platform services, but must not maintain a divergent copy of synthesis algorithms that should live in `resonant_core`.

## M0 boundaries

M0 establishes contracts, portability boundaries, lifecycle, timing, parameters, feedback, observability, determinism, build/test/render foundations and architectural proof.

M0 may contain small DSP probes where needed to prove architecture, but it is not the milestone for a finished musical resonator.

## M0 non-goals

M0 does not require:

- a production Breath Pipe voice;
- a finished string, modal or waveguide instrument;
- a user-facing synth interface;
- a production VST3 binary;
- a production browser instrument;
- production embedded firmware;
- final presets or sound design;
- arbitrary resonator graphs;
- perfect physical realism;
- optimized SIMD or platform-specific acceleration;
- compatibility promises associated with a 1.0 API.

## Post-M0 relationship

M0 freezes enough architecture to begin **M1 — First Resonator** without requiring a core rewrite. Later milestones add synthesis algorithms and hosts against the contracts established here. New capability may extend those contracts, but violating an M0 invariant requires an explicit architecture decision and migration plan.

## Licence

Initial project licence: **MIT**.

The licence is intentionally permissive because Resonant Engine is designed to be reused inside multiple hosts and instruments. Third-party components with incompatible licensing must not be introduced into the core without explicit review.

## Versioning

Resonant Engine uses Semantic Versioning.

During `0.x` development:

- patch releases should not intentionally break documented public behavior;
- minor releases may make breaking API changes when needed to improve architecture;
- every intentional breaking change must be documented with a migration note;
- no pre-1.0 public API is treated as permanently frozen merely because it has shipped once.

`1.0.0` will mean the public integration contract is intentionally stable enough for external hosts to depend on it.

## Toolchain baseline

Canonical language baseline: **C++20**.

Initial M0 toolchain baseline:

- CMake 3.20 or newer;
- Visual Studio 2022 / MSVC with C++20 support;
- GCC 11 or newer;
- Clang 14 or newer.

Exact CI and cross-compilation support is proven in M0.14 and M0.17. The language baseline must not be silently lowered or raised by one host adapter.

## Architectural invariants

The normative invariants are recorded in `docs/architecture/INVARIANTS.md`.

## M0 acceptance

M0 is complete only when M0.22 passes. In particular, completion requires one portable core, real-time-safe processing rules, sample-accurate control/event foundations, deterministic randomness, feedback/energy/stability architecture, host portability review and a successful Breath Pipe architecture gate.

## Anti-clone review

Steampipe is useful research evidence that expressive continuously excited resonant synthesis is musically valuable. It is not the Resonant Engine specification.

M0 must not copy Steampipe-specific control names, UI layout, proprietary implementation details or an assumed fixed algorithm. Resonant Engine is broader: it must also support strings, structures, coupled systems and impossible synthetic bodies using the same core.

High-level ideas such as excitation, tuned resonance, feedback, damping, saturation and dispersion are general synthesis concepts and may be researched independently. Any later design decision must be justified by Resonant Engine requirements rather than by fidelity to Steampipe.

## Approval

**M0.1 Product & Scope Contract: APPROVED.**
