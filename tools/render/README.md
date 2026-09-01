# Offline render host

`resonant_render` is deliberately outside `resonant_core`. It drives the same core API used by future browser, plugin and embedded wrappers and writes deterministic mono PCM16 WAV output.

Canonical fixture:

```sh
resonant_render --sample-rate 48000 --block-size 64 --duration 0.1 --seed 777 --output m0-canonical.wav
```

The seed controls the deterministic noise excitation used by this M0 architectural probe. Live hosts may choose a changing seed outside the core; deterministic/offline hosts should provide a fixed seed.

A zero-excitation placeholder render is also supported:

```sh
resonant_render --silent --duration 0.1 --output silent.wav
```
