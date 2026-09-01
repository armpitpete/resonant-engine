# M0.6 — Audio Processing Model

Status: **APPROVED**

## Numeric and buffer model

- Canonical audio sample type: IEEE-style `float32` represented by `resonant::Sample` (`float`).
- `double`/`Accumulator` is permitted and preferred for accumulated energy, coefficient derivation, sample-rate/time conversion and other calculations where extra precision reduces drift. Audio buffers remain float32.
- Host-facing core buffers are **non-interleaved** arrays of channel pointers.
- M0 supports 0–2 input channels and 1–2 output channels. Zero input channels means no external audio. Mono and stereo inputs are accepted; concrete models decide how channels influence their topology. The reference probe currently consumes the first external input and duplicates its mono result to all output channels.
- Mono output is one channel. Stereo output is two non-interleaved channels. More than two channels is a post-M0 extension, not a hidden assumption in physical-model topology.

## Sample rate and blocks

`ProcessSpec` stores sample rate, maximum block size and channel capacities. Supported M0 sample-rate range is 8 kHz–384 kHz. Maximum host block size is 4096 frames. A concrete host may advertise a smaller prepared maximum.

A zero-frame block is legal and performs no work. Events are not legal in a zero-frame block because no sample offset exists in that block.

Buffers are caller-owned and must remain valid for the call. `resonant_core` does not retain audio-buffer pointers. In-place input/output aliasing is not guaranteed in M0; hosts should provide distinct buffers unless a model explicitly documents safe aliasing.

Silence is represented by zeros and must remain a valid stable state. `reset()` returns model DSP state to deterministic zero-energy/default-control state.

## Feedback and granularity

Host blocks are only transport. The engine iterates sample by sample and model feedback remains inside `processSample()`, so host block size never becomes the feedback delay. Events can update controls at an exact sample offset before that sample is processed.

Internal oversampling is compatible with the architecture: a model may oversample its own nonlinear/feedback subgraph while preserving the external float32 block contract. Any resulting fixed latency must be exposed through a future host-facing latency query; M0 establishes the concept but the reference probe reports no added latency.

## Preparation and reset

`prepare()` validates configuration, permits non-real-time model setup and then resets the model. A successful re-prepare replaces the active processing configuration. A failed prepare invalidates the demonstrated engine until a valid prepare succeeds.

## Evidence

Tests cover block sizes 1, 2, 7, 16, 31 and 64; sample rates 8 kHz, 44.1 kHz, 48 kHz and 192 kHz; mono/stereo configuration; zero-frame calls; silence; reset; exact-sample events and finite output.

**M0.6 Audio Processing Model: APPROVED.**
