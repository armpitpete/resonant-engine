# M0.15 — Offline Render Host

Status: **APPROVED**

`resonant_render` is a deterministic command-line Host outside `resonant_core`. It instantiates the same Engine/model path intended for future Hosts, configures sample rate and maximum block size, accepts render duration and an explicit 64-bit seed, processes blocks offline, captures the output, and writes a mono PCM16 WAV file.

## Contract

- CLI options: `--sample-rate`, `--block-size`, `--duration`, `--seed`, `--output`, `--silent`, `--help`;
- invalid ranges/options fail before rendering;
- output path is owned by the renderer and no filesystem API enters the DSP core;
- deterministic mode uses the supplied seed and core `Pcg32` for noise excitation;
- `--silent` exercises the initial zero-excitation placeholder path;
- normal M0 fixture drives the feedback probe with deterministic noise and fixed sample-zero parameter events;
- output baseline is mono 16-bit PCM RIFF/WAVE;
- the renderer allocates/captures buffers because it is an offline Host, not an audio-thread component.

## Canonical fixture

```sh
resonant_render --sample-rate 48000 --block-size 64 --duration 0.1 --seed 777 --output m0-canonical.wav
```

CTest runs both normal and silent render smoke tests. `verify_wav.py` checks RIFF/WAVE structure, PCM format, channel count, sample rate, byte rate, payload size, and verifies that the silent placeholder contains only zero PCM bytes.

Usage details are in `tools/render/README.md`.

**M0.15 Offline Render Host: APPROVED.**
