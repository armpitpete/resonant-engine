# M0 Foundation Execution — M0.1 to M0.4

## Canonical M0 statement

M0 establishes a portable real-time DSP architecture specifically capable of supporting expressive, continuously excited resonant systems, while remaining general enough for strings, structures, coupled resonators and impossible synthetic bodies later.

Special design priority: **air / pipe / noise is a primary sonic goal.**

## M0.1 — Product and architecture contract

Status: **candidate complete**

Frozen for M0:

- one host-independent DSP core;
- browser, DAW, embedded hardware and other instruments are adapters around that core;
- no host framework types in the DSP contract;
- the architecture must support continuously excited systems rather than only note-triggered oscillator voices;
- future physical models may contain bidirectional interaction and active feedback;
- pipe-specific behavior must not become a generic-core assumption.

Acceptance evidence:

- `README.md` carries the canonical product and M0 statements;
- `include/resonant/Engine.hpp` exposes only standard C++ types;
- the reference model is injected into the engine as a compile-time model type.

## M0.2 — Portable DSP core contract

Status: **candidate complete**

Current contract:

- C++17 baseline;
- explicit sample rate and maximum host block size;
- model-owned state;
- deterministic `prepare`, `reset`, `tick` lifecycle;
- raw/fixed caller-owned buffers for block adaptation;
- no required heap allocation in the process path;
- no required threads, locks, file I/O, UI APIs or host callbacks in the process path.

Reason for C++17: it is broadly supportable across native hosts, WebAssembly toolchains and embedded toolchains without making a newer language runtime a hard dependency.

## M0.3 — Real-time processing contract

Status: **candidate complete**

The core semantic unit is one sample:

`model.tick(input) -> output`

Hosts may deliver blocks, but `Engine::processBlock` is only an adapter that repeatedly calls the same sample operation.

This is intentional. Feedback and exciter/resonator interaction must be expressible inside the model at sample resolution. A host block boundary must never become the feedback boundary.

Real-time invariants:

- process calls are `noexcept`;
- no allocation is required by the engine process loop;
- controls may change every sample;
- state remains owned by the model instance;
- reset must return the model to a deterministic zero-energy state;
- invalid host preparation data is rejected before processing.

## M0.4 — Closed-loop architecture probe

Status: **candidate complete**

`ReferenceFeedbackProbe` is deliberately not a finished resonator or Breath Pipe voice. It exists only to prove that one model can contain:

1. continuous excitation;
2. returned resonant energy;
3. active feedback amount;
4. feedback filtering;
5. nonlinearity inside the loop;
6. bounded output behavior;
7. sample-by-sample control movement.

The smoke test drives all of these continuously over 256 samples and checks finite, bounded output plus deterministic reset.

## Architectural boundary established by M0.1–M0.4

The generic engine owns lifecycle and host adaptation. A concrete physical/resonant model owns its internal signal topology.

That boundary is required for future systems such as:

- Breath Pipe;
- strings and bowed/plucked structures;
- coupled resonator networks;
- feedback structures;
- deliberately impossible synthetic bodies.

## Not claimed yet

M0.1–M0.4 do **not** yet claim:

- a perceptually convincing pipe;
- waveguide accuracy;
- modal-bank accuracy;
- overblow behavior;
- alias-safe nonlinear feedback at all gains;
- browser/DAW/embedded adapter completion;
- production CPU or memory budgets.

Those remain later M0 work and must be proven rather than inferred from this foundation.
