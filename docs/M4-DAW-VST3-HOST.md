# M4 — DAW/VST3 Reference Host

Status: **DESIGN CANDIDATE**

## Goal

M4 proves that the frozen Resonant Engine core and frozen M3 Breath Pipe Reference Voice can run as a real DAW instrument through a thin VST3 host wrapper without duplicating or reinterpreting DSP.

M4 is the first external desktop-plugin host proof. It is **not** a new synthesis milestone and it must not retune the accepted M3 Breath Pipe.

## Product boundary

The M4 deliverable is a reference VST3 instrument host around `resonant_core`.

It must:

- use the same Breath Pipe model and parameter semantics as M3;
- translate VST3 buffers, events, automation and lifecycle into the existing portable core boundary;
- keep all VST3 SDK types outside `resonant_core`;
- preserve sample-accurate event timing;
- preserve core-owned smoothing, feedback, turbulence and stability behaviour;
- provide deterministic state save/restore through a host-neutral versioned state contract;
- expose bounded diagnostics without realtime logging or callbacks into the host;
- remain usable without a custom GUI.

## Wrapper technology

M4 uses the Steinberg VST3 SDK directly in `hosts/vst3/`.

Rationale:

- the current VST3 SDK is MIT-licensed;
- the repository already uses CMake and C++20;
- direct integration keeps the host boundary explicit;
- JUCE is unnecessary for the first host proof and therefore is not introduced by M4.

VST3 helper classes may be used in the host wrapper. VSTGUI is not required because M4 does not require a custom editor.

## Frozen invariants

M4 must not change these accepted rules:

1. one canonical DSP core;
2. no VST3, JUCE, DAW or GUI types in public core interfaces;
3. no host-specific copy of Breath Pipe DSP;
4. no allocation or blocking synchronization in the core realtime processing path;
5. host block size must not become feedback delay;
6. parameter smoothing required for musical behaviour remains core/model semantics;
7. M3 sound, operating envelope and B01–B18 acceptance baseline remain frozen unless a demonstrated host-boundary defect requires an explicit superseding decision.

## M4 sections

### M4.0 — Contract and dependency boundary

- [ ] freeze this milestone contract;
- [ ] pin the VST3 SDK revision used by CI/builds;
- [ ] document SDK licence/trademark obligations;
- [ ] enforce that VST3 headers/types do not enter `resonant_core`.

### M4.1 — VST3 component skeleton

- [ ] build a VST3 instrument bundle;
- [ ] processor/controller/component registration;
- [ ] stereo output bus;
- [ ] optional audio-input bus for external excitation where host capabilities permit;
- [ ] no custom GUI requirement.

### M4.2 — Processing lifecycle

- [ ] map VST3 setup/activation/reset to portable core lifecycle;
- [ ] sample rates 44.1/48/96 kHz;
- [ ] bounded variable host block sizes within the M0 contract;
- [ ] no added host-visible buffering.

### M4.3 — Audio-buffer translation

- [ ] translate VST3 float32 buffers into caller-owned core views;
- [ ] safe silence/fail-closed behaviour for malformed host buffers;
- [ ] prove no Host DSP duplication.

### M4.4 — Note and expression translation

- [ ] NoteOn/NoteOff and velocity;
- [ ] stable note identity where supplied;
- [ ] pitch/pressure mapping through VST3 event/parameter facilities;
- [ ] deterministic sample-offset ordering;
- [ ] malformed/overflow policy using bounded event storage.

Full MPE is not required for M4 unless it falls naturally out of the portable event contract without broadening scope.

### M4.5 — Parameter and automation mapping

- [ ] stable mapping from core `ParameterId` to VST3 `ParamID`;
- [ ] normalized/native conversion via core metadata;
- [ ] sample-accurate automation points;
- [ ] host-visible names/units/defaults from portable metadata;
- [ ] topology/internal parameters remain correctly hidden or non-realtime.

### M4.6 — Portable state recall

- [ ] define a versioned host-neutral Breath Pipe state representation;
- [ ] VST3 getState/setState translates to that representation;
- [ ] save/reload reproduces parameter/model state deterministically;
- [ ] malformed/unknown state fails safely;
- [ ] no VST3-specific serialized representation becomes the canonical engine state.

### M4.7 — External excitation

- [ ] when an input bus is available, route caller-owned input audio through the existing core external-excitation path;
- [ ] no device/file/network access in the core;
- [ ] no hidden monitoring or extra synthesis path.

### M4.8 — Realtime and boundedness proof

- [ ] wrapper processing performs no unbounded allocation;
- [ ] bounded event/automation translation;
- [ ] no locks on the audio thread;
- [ ] deterministic reset;
- [ ] finite/protected-state propagation remains observable;
- [ ] CPU scaling recorded for one, four and eight voices where applicable.

### M4.9 — Native-core/VST3 parity

- [ ] deterministic reference sequences rendered directly through the core and through the VST3 processor;
- [ ] compare output signatures within an explicitly documented tolerance;
- [ ] prove automation/event timing parity, not only static-note audio similarity.

### M4.10 — VST3 conformance

- [ ] Steinberg validator PASS;
- [ ] clean plugin scan/load/unload;
- [ ] repeated activate/deactivate/reset;
- [ ] state save/restore;
- [ ] offline/non-realtime processing mode where supplied by host;
- [ ] no crash or stuck voice after malformed or extreme automation.

### M4.11 — Human DAW acceptance

One real DAW on a supported desktop platform must prove:

- [ ] plugin scans and loads;
- [ ] notes play the accepted Breath Pipe sound;
- [ ] velocity and expressive automation are audible;
- [ ] chord/polyphony behaviour is sane;
- [ ] project save/reopen restores state;
- [ ] offline bounce produces valid audio;
- [ ] stop/start/reload does not leave stuck or runaway state;
- [ ] listener confirms the host did not materially change the accepted M3 sound.

The DAW is an acceptance host, not the architectural reference implementation.

### M4.12 — Platform evidence

Final candidate must pass:

- [ ] Windows MSVC VST3 build and validator;
- [ ] macOS Clang VST3 build and validator;
- [ ] routine Linux/core/adapter tests on the existing self-hosted path where applicable;
- [ ] sanitizer and no-exceptions/no-RTTI core proofs remain green;
- [ ] exact-head provenance for distributable test artifacts.

A Linux VST3 package is useful but not required to close the first M4 desktop-host milestone unless implementation evidence makes it low-cost and reliable.

### M4.13 — Hostile review and freeze

- [ ] confirm no core/host dependency inversion;
- [ ] confirm no Breath Pipe retuning by host code;
- [ ] confirm state/automation semantics are portable;
- [ ] confirm no hidden DAW-specific DSP or timing assumptions;
- [ ] final exact-head CI;
- [ ] protected merge authorization;
- [ ] post-merge verification;
- [ ] M4 FINAL PASS and freeze.

## Non-goals

M4 does not require:

- a commercial product name or installer;
- a custom plugin GUI;
- a preset browser;
- AU/AUv3/AAX/CLAP builds;
- embedded firmware;
- a second resonator model;
- new Breath Pipe sound design;
- MIDI-device APIs in the core;
- DAW transport-driven synthesis features;
- host-specific oversampling or latency compensation DSP;
- a 1.0 public API promise.

## Acceptance principle

M4 passes only when a real DAW can host the same accepted Breath Pipe system through a thin VST3 wrapper and automated evidence proves that host translation did not create a second synthesis implementation.

## References

- Steinberg VST3 SDK: https://github.com/steinbergmedia/vst3sdk
- VST3 Developer Portal: https://steinbergmedia.github.io/vst3_dev_portal/
