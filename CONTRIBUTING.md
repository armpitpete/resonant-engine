# Contributing to Resonant Engine

## Build and test

```sh
cmake -S . -B build -DRESONANT_ENGINE_BUILD_TESTS=ON -DRESONANT_ENGINE_BUILD_RENDER=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Use C++20. Public portable headers live in `core/include/resonant`; private core sources live in `core/src`. Host-specific code belongs under `hosts/` and must not be pulled into the core.

## Real-time contribution contract

Anything reachable from `Engine::process()` must have bounded work and bounded memory and must not:

- allocate or deallocate dynamically;
- acquire mutexes or block on synchronization;
- call filesystem/network/console/GUI/device/Host APIs;
- throw across the processing boundary;
- use unbounded loops or unpredictable callbacks;
- obtain entropy from `std::random_device` or OS APIs.

Preparation/configuration/preset/graph mutation happens outside the audio thread. Sample-accurate values arrive through fixed-size events/controls. New DSP paths need tests demonstrating finite output and, where practical, zero allocation in the exercised process path.

## Architecture discipline

Normative terminology and contracts live in `docs/architecture`. Accepted architectural decisions live in `docs/decisions`. Research observations are non-normative. Do not introduce a host-specific DSP fork or a special-case Breath Pipe/Steampipe API to solve one model quickly.

A breaking pre-1.0 API change is allowed when justified, but update contracts/ADRs and provide a migration note.
