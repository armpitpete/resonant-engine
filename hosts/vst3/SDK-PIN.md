# M4 VST3 SDK Pin

Status: **PINNED FOR M4.0**

## Source

Repository: `steinbergmedia/vst3sdk`

Pinned GitHub release tag:

`v3.8.0_build_66`

Pinned superproject commit:

`9fad9770f2ae8542ab1a548a68c1ad1ac690abe0`

The SDK is fetched recursively because the VST3 SDK superproject pins its implementation repositories as Git submodules.

## Why 3.8.0

The Steinberg Developer Portal lists VST 3.8.1 dated 11 August 2026, but the public GitHub tag listing available during M4.0 does not expose a matching reproducible `v3.8.1` source tag. M4 therefore pins the latest reproducible tagged GitHub SDK state, VST 3.8.0 build 66, rather than following `master` or guessing an unpublished source revision.

A later SDK update requires an explicit pin change and fresh M4 platform/validator evidence.

## Licence and boundary

The pinned VST3 SDK repository is MIT licensed. The SDK remains an external Host-layer dependency and is not copied into `resonant_core`.

The VST trademark and SDK usage guidelines remain Steinberg-controlled. Resonant Engine must describe compatibility accurately and must not imply Steinberg endorsement.

M4 does not introduce JUCE or VSTGUI. Host generic parameter UI is sufficient for the first DAW proof.
