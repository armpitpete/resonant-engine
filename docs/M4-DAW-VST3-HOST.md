# M4 — DAW/VST3 Reference Host

Status: **M4.0–M4.5 MERGED AND COMPLETE — M4.6 NEXT**

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

- [x] freeze this milestone contract;
- [x] pin the VST3 SDK revision used by CI/builds;
- [x] document SDK licence/trademark obligations;
- [x] enforce that VST3 headers/types do not enter `resonant_core`.

### M4.1 — VST3 component skeleton

- [x] build a VST3 instrument bundle;
- [x] processor/controller/component registration;
- [x] stereo output bus;
- [x] external-audio input is explicitly deferred to M4.7; no M4.1 input bus is required;
- [x] no custom GUI requirement.

### M4.2 — Processing lifecycle

- [x] map VST3 setup/activation/reset to portable core lifecycle;
- [x] sample rates 44.1/48/96 kHz;
- [x] host blocks larger than the 4096-frame core maximum are chunked into bounded core calls without making host block size feedback delay;
- [x] no added host-visible buffering.

### M4.3 — Audio-buffer translation

- [x] translate VST3 float32 buffers into caller-owned core views;
- [x] safe silence/fail-closed behaviour for malformed host buffers;
- [x] prove no Host DSP duplication.

### M4.4 — Note and expression translation

- [x] NoteOn/NoteOff and velocity;
- [x] stable note identity where supplied;
- [x] pitch/pressure mapping through VST3 event/parameter facilities;
- [x] deterministic sample-offset ordering;
- [x] malformed/overflow policy using bounded event storage.

M4.4 merged as PR #11 at exact authorized head `28d7f67e095e0f7e12474aa695e9b8e35a414c38`. Exact-head native Debug/Release, sanitizers, portability, M3 native/WASM parity and Linux VST3 validation all passed; Steinberg validator reported **47 tests passed, 0 tests failed**. Merge commit `13eb9fe1158d02c754da3a0bae4af9f569379ff4` has the identical tree `244ee39bfe0898c40d322958681c7632370aa2be`.

Full MPE is not required for M4 unless it falls naturally out of the portable event contract without broadening scope.

### M4.5 — Parameter and automation mapping

- [x] stable mapping from core `ParameterId` to VST3 `ParamID`;
- [x] normalized/native conversion via core metadata;
- [x] sample-accurate automation points;
- [x] host-visible names/units/defaults from portable metadata;
- [x] topology/internal parameters remain correctly hidden or non-realtime.

M4.5 merged as PR #12 at exact authorized head `2feaa9863443416eb7d8c2e50defafbfd58b206a` as merge commit `a0228e3853e17e9cc3a2a85ab6dd4e04fba11016`. The merge commit contains no file-level delta from the authorized head, preserving the exact tested tree.

Exact-head acceptance passed native Debug/Release, ASan+UBSan, no-exceptions/no-RTTI portability, M3 native/WASM parity, the dedicated zero-sample VST3 parameter-flush regression, and the Linux VST3 integration gate. Steinberg validator reported **47 tests passed, 0 tests failed**.

The accepted host projection keeps the stable numeric core `ParameterId` as the VST3 `ParamID`, derives host names/units/defaults from portable metadata, translates sample-accurate automation to ordinary core `ParameterChange` events, and handles VST3 zero-sample/no-audio parameter flushes without inventing a DSP block. Flushed values are staged transactionally at the Host boundary and enter the core through the ordinary portable parameter path on the next real block, preserving core-owned smoothing and frozen M3 DSP semantics.

M4.5 is complete. M4.6 proceeds from merged `main` and is limited to portable state recall.

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


## M4.0–M4.3 implementation evidence

The accepted implementation evidence head before this reconciliation is
`61b842d00ec910c4404d4df2b4caf17ceb7358a3`.

Hosted `M4 VST3 Build` run #9 (`33900470367`) passed on that exact head:

- Windows MSVC Debug — PASS;
- Windows MSVC Release — PASS;
- macOS Clang Debug — PASS;
- macOS Clang Release — PASS;
- raw PR-head verification — PASS in every job;
- Steinberg validator — PASS with `0 tests failed` in every job.

The Windows validator is run explicitly rather than as an MSBuild post-build hook. Steinberg intentionally emits error-labelled diagnostics when probing unsupported sample rates even when its aggregate result is PASS; Visual Studio 18 treats those text lines as custom-build errors despite validator exit status 0. The explicit CI step therefore requires both a zero validator exit and the aggregate `0 tests failed` summary.

Routine Oracle CI #155 Release on the same implementation head passed all 35 CTests, including:

- `resonant_vst3_adapter_tests` — PASS;
- `resonant_vst3_dependency_boundary` — PASS.

The adapter proof demonstrates exact reset silence, real output from the same frozen `BreathPipeVoice`, stereo translation and bounded core-block rejection. The hosted validator additionally proves DAW-facing lifecycle and oversized-host-block chunking. This reconciliation adds explicit 44.1/48/96 kHz adapter preparation coverage.

The VST3 SDK is pinned to reproducible GitHub tag `v3.8.0_build_66`, superproject commit `9fad9770f2ae8542ab1a548a68c1ad1ac690abe0`. The Developer Portal lists VST 3.8.1, but no matching reproducible GitHub tag was available when M4.0 was established; M4 deliberately does not follow `master`.

### M4.0–M4.3 hostile review

**PASS at implementation level.**

- no `core/**` source file changed in this slice;
- VST3 SDK types exist only under the Host/build layer;
- the SDK-free `BreathPipeCoreAdapter` composes `Engine<BreathPipeVoice>` instead of copying synthesis code;
- no host-specific smoothing, turbulence, feedback or resonator algorithm was added;
- oversized host blocks are processed as consecutive legal core calls, not buffered or converted into block-rate feedback;
- note/event translation remains deliberately unimplemented until M4.4 and is not falsely claimed by this slice;
- parameters/state/external excitation remain deliberately open for M4.5–M4.7;
- no custom GUI or JUCE dependency was introduced.

### CI cost-control rule

Hosted Windows/macOS validation is a **deliberate milestone gate**, not a per-commit service. A previously green hosted platform result may be carried forward across a later exact head only when hostile diff review proves that no platform-sensitive VST3 implementation, SDK pin, build configuration or validator command changed. Any such relevant change requires a fresh manual hosted matrix.

Routine PR validation remains on the self-hosted Oracle runner. Routine Lab builds record exact-head provenance, deterministic parity and SHA-256 evidence in workflow logs/job summaries; GitHub Actions artifact storage is used only when a later manual test genuinely needs a transferable build. This avoids paying to retain duplicate transient evidence.

For this M4.0–M4.3 reconciliation, hosted run #9 on `61b842d00ec910c4404d4df2b4caf17ceb7358a3` remains valid platform evidence because the later delta changes only workflow triggering, documentation/ADR text and additional SDK-free adapter tests. No VST3 processor, controller, adapter, SDK pin, CMake host build logic or validator command changed. Fresh exact-head Oracle validation is still required.

This slice may leave draft only after that fresh exact-head self-hosted validation and hostile diff review pass. A fresh hosted matrix is required again before M4 final freeze if later M4 work changes any platform-sensitive implementation.


## M4.0–M4.3 merge closure

PR #10 merged at exact authorized head `19d159abb75fa6fa21dc0cefeed92549a0fe1980` as merge commit `e6caa090bf7766dc26bf3e62e3fa88f34c0dae6b`. The authorized head tree and merged tree were identical: `0590f4c06fb0bc080adac36d8047a8bf1795f414`.

M4.0–M4.3 are therefore merged and complete. M4.4 proceeds from that exact merged main state.
