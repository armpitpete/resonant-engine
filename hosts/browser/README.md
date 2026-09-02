# Browser hosts

Browser code is a thin Emscripten/WASM + Web Audio adapter around shared C++ Resonant Engine code. Synthesis algorithms do not live in JavaScript or Web Audio nodes.

## M2 Resonant Engine Lab

`hosts/browser/lab/` is the canonical browser diagnostic and human-acceptance host.

It is **not a browser synth**. It provides exact test controls, measurements, scripted acceptance scenarios and evidence capture around the C++ core/Lab adapter.

Build with:

```sh
./hosts/browser/lab/build.sh
```

Then serve `build/resonant-lab/` over localhost HTTP.

## M1 historical play-test harness

`hosts/browser/m1/` is the temporary M1 live acceptance harness retained with M1 history. Its keyboard/MIDI surface was created only to make the first resonator playable for the M1 human gate. It is not the direction of the Resonant Engine product and is not used by the M2 Lab CI gate.
