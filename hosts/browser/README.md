# Browser host

The browser host is the thin Emscripten/WASM + AudioWorklet adapter around `resonant_core`. It translates Web Audio and browser/MIDI control input into the shared core; synthesis algorithms do not live here.

## M1 acceptance harness

`hosts/browser/m1/` contains the first live host implementation. It exists to play and judge `resonant::FirstResonatorVoice` interactively before M1 can be accepted.

The JavaScript layer owns browser startup, Web MIDI, UI and AudioWorklet transport only. The sound-generating implementation remains the C++ M1 voice in `resonant_core`.
