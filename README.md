# Resonant Engine

Resonant Engine is a portable resonant/physical synthesis platform capable of running from the same core DSP architecture in browsers, DAWs, embedded hardware and other host instruments.

## M0

M0 establishes a portable real-time DSP architecture specifically capable of supporting expressive, continuously excited resonant systems, while remaining general enough for strings, structures, coupled resonators and impossible synthetic bodies later.

**Special design priority:** air / pipe / noise is a primary sonic goal.

## Current M0 slice

The canonical M0 roadmap contains 22 sections. The current bounded slice is:

- **M0.1 — Product & Scope Contract** — approved on branch;
- **M0.2 — Sonic Design Contract** — approved on branch;
- **M0.3 — Core Terminology & Concept Model** — approved on branch;
- **M0.4 — Portable DSP Core Boundary** — approved on branch;
- **early M0.21 — Breath Pipe Architecture Review** — provisional pass before later core contracts harden.

Canonical documents:

- [Product & Scope Contract](docs/architecture/PRODUCT_SCOPE.md)
- [Air / Pipe / Noise Sonic Design Contract](docs/architecture/AIR_PIPE_NOISE.md)
- [Normative Glossary](docs/architecture/GLOSSARY.md)
- [Portable DSP Core Boundary](docs/architecture/CORE_BOUNDARY.md)
- [Architectural Invariants](docs/architecture/INVARIANTS.md)
- [M0 foundation status](docs/M0-FOUNDATION.md)
- [Early Breath Pipe architecture review](docs/M0.21-BREATH-PIPE-ARCHITECTURE-REVIEW.md)

## Core rule

One C++20 `resonant_core` must serve browser, DAW/plugin, embedded and other synth hosts. Hosts may supply blocks, but the resonant model must remain able to own sample-by-sample closed-loop interaction. Host-specific DSP duplication is prohibited.

## Build

```sh
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

The current reference feedback probe is an architectural test fixture, **not** the Breath Pipe voice and not a claim of finished physical modelling.

## Licence

MIT. See [LICENSE](LICENSE).
