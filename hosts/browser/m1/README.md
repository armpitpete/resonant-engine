# M1 browser play-test harness

This is the human acceptance instrument for **M1 — First Resonator**.

It compiles the real C++ `resonant::FirstResonatorVoice` to WebAssembly and runs it in an `AudioWorklet`. No synthesis algorithm is reimplemented in JavaScript.

## Build

Requires Emscripten 6.0.6 or compatible:

```sh
./hosts/browser/m1/build.sh
```

The output is written to `build/m1-browser/`.

## Run

Browsers require the AudioWorklet files to be served from a secure origin; `localhost` qualifies.

Windows: open `RUN-M1-SYNTH.bat` from the built artifact.

Other platforms:

```sh
cd build/m1-browser
./run-m1-synth.sh
```

Then open `http://127.0.0.1:8000/`.

## Playing

- Click **Start audio**.
- Computer keys: `A W S E D F T G Y H U J K`.
- `Z` / `X`: octave down/up.
- **Strike / trigger** directly excites the resonator.
- **Excitation** adds continuous deterministic air/noise without requiring NoteOn.
- **Turbulence**, **Damping**, **Regeneration**, **Nonlinearity**, and **Interaction** exercise the M1 continuous controls.
- **Enable MIDI** accepts note input. CC1/mod wheel and channel pressure map to continuous excitation for the M1 play test.

The harness is deliberately monophonic. Polyphony, production UI and a finished Breath Pipe exciter are not M1 requirements.

## Acceptance

The human M1 gate is interactive, not a WAV-listening exercise. Pass only if playing the synth demonstrates a musically useful resonator with useful movement across passive, continuously excited and regenerative states.

Offline WAV fixtures remain deterministic regression evidence only.
