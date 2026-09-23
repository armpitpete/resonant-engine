# M3 — Breath Pipe Reference Voice

Status: **ACCEPTED BASELINE FROZEN ON `main` — M3.7 EXPRESSIVE REPAIR OPEN IN PR #20**

M3 established the accepted Breath Pipe reference voice and B01–B18 human baseline. During M4.11 real-DAW acceptance, direct Cubase listening exposed one bounded weakness that the original M3 gates did not measure adequately: four primary musical controls could be mathematically effective yet insufficiently obvious to a player. PR #20 therefore explicitly reopens M3.7 only; it does not reopen the topology or generic Engine architecture.

## Protected baseline

The accepted M3 topology remains a bounded three-mode interacting modal Breath Pipe. Its protected continuum remains:

```text
silence
→ faint air
→ turbulence
→ emerging pitch
→ unstable resonance
→ stable pipe
→ rich pipe
→ overblow
→ aggressive resonance
→ noise
```

The following remain protected unless separately superseded: one canonical DSP core; continuous excitation/interaction; regeneration; in-loop nonlinearity; feedback spectral shaping; deterministic state; external excitation; C2–C6 operating envelope; 44.1/48/96 kHz support; bounded realtime work; B01–B18 identity; no hidden substitute oscillators or preset-switched engines.

## M3.7 — Musical macro controls & expressive mapping

MIDI semantics remain Host-side. Hosts may map velocity, pressure, MPE or hardware controls onto model concepts, but the Breath Pipe receives model controls rather than MIDI concepts.

M4.11 direct DAW listening exposed a weakness that the original automated gates did not measure: controls could be numerically different while remaining too weak as musical controls. M3.7 is therefore reopened narrowly for **Pressure, Turbulence, Damping and Nonlinear Drive**. The topology and canonical Stable Pipe identity remain protected.

### Stable Pipe reference scene vs host defaults

The canonical Stable Pipe point is the zero-motion anchor for the reopened macro mapping:

- Pressure `0.55`
- Turbulence `0.18`
- Interaction `0.72`
- Damping `0.08`
- Regeneration `0.38`
- Feedback colour `0.25`
- Nonlinear Drive `0.10`
- External excitation `0.0`
- Timbre `0.25`

These are **reference-scene values, not host parameter defaults**. The portable `ParameterSpec::default_value` values remain neutral initialization/state defaults exposed to hosts. M4.11 must not silently rewrite host defaults to the Stable Pipe scene merely to improve a DAW audition. An acceptance session that requires Stable Pipe must set the documented scene values explicitly.

The macro layer must reproduce the underlying exciter/resonator path sample-for-sample at the Stable Pipe zero-motion anchor.

### Primary perceptual-macro contract

The four reopened controls have distinct deterministic preflight contracts:

- **Pressure:** between `0.25` and `0.85`, sustained RMS rises by at least `12 dB` and overblow increases by at least `0.40`.
- **Turbulence:** between `0.05` and `0.85`, sustained RMS rises by at least `9 dB` and first-difference roughness rises by at least `10%`.
- **Damping:** between `0.03` and `0.65`, sustained RMS falls by at least `9 dB` and mode-0 radius falls by at least `0.001`.
- **Nonlinear Drive:** between `0.05` and `0.90`, sustained RMS rises by at least `3 dB`, roughness rises by at least `15%`, and overblow increases by at least `0.10`.

For each primary macro, five representative control positions must have no dead zone: every adjacent render pair must produce normalized waveform separation of at least `0.10`.

These thresholds are engineering guards, not psychoacoustic proof. Direct M4.11 H04 listening remains authoritative for whether movement is clearly audible, coherent and musically useful in a real DAW.

Interaction, Regeneration, Feedback colour and Timbre retain their accepted model semantics and regression coverage; M3.7 does not claim every exposed control has equal perceptual leverage.

## M4.11 diagnostic result

At PR #20 head `054044f230dd7b555d2348ddb6aa5e646ce540db`, direct-core and actual VST3 `Processor` diagnostics both showed strong full-range effects for the four reopened controls. This rules out a weak normalized→native conversion or generally insensitive processor path as the explanation for the Cubase H04 failure.

Therefore **no further blind DSP widening is permitted**. The next diagnostic is explicit Cubase endpoint/native-value automation. Only new evidence demonstrating a production defect may justify another DSP change.

## Re-freeze rule

The original M3 acceptance on `main` remains the historical accepted baseline while PR #20 is open. The repaired M3.7 contract may be declared FINAL/FROZEN only when:

1. deterministic M3.7 regression passes at one exact head;
2. M3 native/WASM parity and sanitizer/portability gates pass;
3. M4.11 H04 passes directly in the real DAW;
4. H09 confirms no material loss of the accepted Breath Pipe identity;
5. final hostile/no-drift review finds no unrelated M3 change;
6. the exact accepted head is merged and post-merge verification passes.

Until then the repair is a candidate, not a replacement for the accepted baseline.
