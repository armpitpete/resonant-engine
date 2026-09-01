# M0.12 — Determinism & Randomness Contract

Status: **APPROVED**

## PRNG

M0 selects **PCG-XSH-RR 32-bit output** (`Pcg32`) with 64-bit internal state/stream derivation. `Seed` is `uint64_t`. The default deterministic seed is a fixed constant; a Host may supply any explicit 64-bit seed.

No `std::random_device`, OS entropy API or heap allocation is used by core random generation. Live/randomized mode means the Host chooses a changing seed outside `resonant_core`; deterministic/offline mode supplies a recorded fixed seed.

## Engine and voice seeds

An engine/model receives a base seed through its own configuration. Independent voice seeds are derived by a fixed SplitMix64-based `deriveVoiceSeed(engine_seed, voice_index)`. Voice index is part of deterministic state. Future voice stealing must define how logical voice identity maps to index before promising deterministic polyphonic renders.

`Pcg32::reset()` returns to its initial sequence. Serializable preset/state may store the selected base seed where reproducibility is intended; ephemeral current PRNG state is DSP state, not necessarily a user preset field unless exact continuation is requested by a later state format.

## Guarantees

For the same build semantics, seed and event/input sequence, the integer PRNG sequence is exactly deterministic. Tests also require the demonstrated offline floating-point render to repeat identically on the same platform/process.

M0 does **not** promise bit-identical complete DSP output across every compiler, CPU, WebAssembly runtime or floating-point mode. Cross-platform conformance is defined using finite behavior and numeric/audio tolerances unless a particular integer/fixture component explicitly has stronger guarantees.

Canonical test seed: `777`.

**M0.12 Determinism & Randomness Contract: APPROVED.**
