# M0.5 — Real-Time Processing Contract

Status: **APPROVED**

The audio thread is a bounded computation domain. Anything that can block, allocate unpredictably, call a host service, or throw across the processing boundary belongs outside `resonant_core::process()`.

## Audio-thread rules

Inside `process()` and every function transitively called from it:

- no dynamic allocation or deallocation;
- no mutex acquisition, condition variables, blocking atomics, sleeps or waits;
- no filesystem, network, console/logging, GUI, host callbacks, USB or device APIs;
- no exceptions may cross the real-time boundary; core processing APIs are `noexcept`;
- no unbounded loops or data-dependent searches without a documented hard bound;
- no unpredictable callbacks into caller code;
- memory touched by processing must be pre-existing and bounded by the prepared configuration;
- events and parameters are consumed from fixed-size/caller-owned storage;
- work per block is bounded by `frames * (model work + event work)` with frames and event count capped.

Preparation, preset loading, graph construction, allocation, file I/O and state replacement happen off the audio thread. A host may prepare a replacement state and atomically/safely hand it over at a non-real-time boundary; M0 does not require lock-free graph mutation while audio is running.

## Parameter and configuration mutation

Continuous parameters arrive as validated sample-offset events. Values are clamped/sanitized in bounded code. Topology-changing parameters are configuration requests and are not applied by mutating heap-owned structures inside `process()`.

Malformed events, invalid ranges and non-finite parameter data never trigger allocation or exceptions. They are rejected or sanitized according to the Event and Parameter contracts.

## Numeric policy

- NaN/Infinity on external audio input is converted to zero at the core boundary.
- NaN/Infinity generated internally is treated as numerical failure: the demonstrated `Engine` zeros the affected output path and returns `NumericalFailure`.
- Denormals must not become an architectural dependency. Models should use state decay/zeroing thresholds where needed; hosts may additionally enable FTZ/DAZ modes outside the portable core.
- Musical instability is allowed. Numerical runaway is not.

## Invalid processing state

Processing before `prepare()` returns `NotPrepared` and emits silence. Invalid buffer/event context is rejected and output is cleared when safely possible. A failed re-prepare leaves the engine unprepared rather than continuing with ambiguous state.

## Evidence

`resonant_tests` wraps the demonstrated process call with a global allocation counter and requires zero allocations. The dependency/RT boundary test scans core sources for forbidden host APIs, `std::random_device`, console output and basic file/network calls. CI runs the suite in Debug/Release and under ASan/UBSan where supported.

Allocation instrumentation is a demonstrated-path guard, not a proof that every future model is allocation-free. New models must add equivalent evidence.

**M0.5 Real-Time Processing Contract: APPROVED.**
