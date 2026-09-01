# Resonant Engine

Resonant Engine is a portable resonant/physical synthesis platform capable of running from the same core DSP architecture in browsers, DAWs, embedded hardware and other host instruments.

## M0

M0 establishes a portable real-time DSP architecture specifically capable of supporting expressive, continuously excited resonant systems, while remaining general enough for strings, structures, coupled resonators and impossible synthetic bodies later.

**Special design priority:** air / pipe / noise is a primary sonic goal.

## Current execution slice

M0.1–M0.4 are implemented as a foundation candidate on `m0/foundation-breath-pipe-review`, with an early M0.21 Breath Pipe architecture review completed before later architecture hardens.

- [M0.1–M0.4 foundation contract](docs/M0-FOUNDATION.md)
- [M0.21 early Breath Pipe architecture review](docs/M0.21-BREATH-PIPE-ARCHITECTURE-REVIEW.md)

## Core rule

Hosts may supply blocks, but the resonant model owns the sample-by-sample closed loop. This keeps active feedback, continuous excitation and bidirectional exciter/resonator interaction inside the physical model rather than forcing them through host block boundaries.

## Build

```sh
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

The current reference feedback probe is an architectural test fixture, **not** the Breath Pipe voice and not a claim of finished physical modelling.
