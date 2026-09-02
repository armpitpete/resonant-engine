# Resonant Engine Lab browser host

This directory contains the M2 browser diagnostic/test host.

**It is not a browser synth.**

## Build

Activate Emscripten, then:

```sh
./hosts/browser/lab/build.sh
python3 -m http.server 8000 --directory build/resonant-lab
```

Open `http://127.0.0.1:8000/` and choose **Start audio**.

## Responsibilities

Browser host code may:

- start/suspend Web Audio;
- transport exact test note/parameter commands;
- run the canonical scripted suite;
- display waveform/spectrum and output measurements;
- show C++ Lab telemetry;
- capture optional evidence WAV/JSON/PNG;
- report browser-host failures.

It must not implement a resonator, exciter, feedback loop or browser-only synthesis behaviour.

## Files

- `LabWasm.cpp` — exported C ABI around `resonant_lab::LabEngine`;
- `worklet.js` — realtime WASM transport and audio-frame scenario scheduler;
- `capture-worklet.js` — downstream optional evidence capture;
- `app.js` — controls, measurement, scenario compiler and evidence export;
- `index.html` / `style.css` — diagnostic UI;
- `build.sh` — deterministic browser artifact build;
- `parity.mjs` — CI-only Node/WASM signature runner.

Canonical presets/tests live in `lab/contracts/`, outside the browser host.
