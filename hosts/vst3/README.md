# VST3 reference host

M4 implements a thin native VST3 instrument wrapper around the frozen Resonant Engine core.

## Boundary

Allowed here:

- Steinberg VST3 SDK types;
- plugin component/controller registration;
- host audio/event/automation translation;
- plugin lifecycle and state translation;
- host-specific diagnostics and packaging.

Forbidden here:

- a copied Breath Pipe synthesis implementation;
- host-specific smoothing or feedback semantics that duplicate the core;
- VST3/JUCE types in `resonant_core`.

`CoreAdapter.hpp` is deliberately SDK-free. It exposes the smallest host seam around `Engine<BreathPipeVoice>` so routine Oracle tests can verify lifecycle and audio-buffer translation without downloading the SDK.

## SDK

See `SDK-PIN.md`.

M4 pins VST3 SDK `v3.8.0_build_66` at superproject commit
`9fad9770f2ae8542ab1a548a68c1ad1ac690abe0`.

## Build

The plugin build is opt-in so routine core CI does not fetch/build the SDK:

```sh
cmake -S . -B build-vst3 \
  -DRESONANT_ENGINE_BUILD_VST3=ON \
  -DRESONANT_ENGINE_BUILD_TESTS=OFF \
  -DRESONANT_ENGINE_BUILD_RENDER=OFF \
  -DSMTG_CREATE_PLUGIN_LINK=OFF
cmake --build build-vst3 --config Release --target ResonantEngineBreathPipe
```

M4.0–M4.3 intentionally expose no plugin parameters or note translation yet; those begin in M4.4–M4.5.
