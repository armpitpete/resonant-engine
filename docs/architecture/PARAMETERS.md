# M0.8 — Parameter Contract

Status: **APPROVED**

## Identity and representation

`ParameterId` is a stable unsigned 32-bit identifier. ID 0 is reserved/invalid. Public models must assign IDs explicitly and preserve them across compatible preset/state versions.

Host automation may use normalized `[0,1]` values; the model works in native/physical units. `ParameterSpec` records minimum, maximum, default, name, unit, kind, smoothing mode/duration and host visibility. Conversion and clamping are finite and deterministic.

Kinds:

- **Continuous** — normal smoothly varying value;
- **Discrete** — stepped/enumerated value;
- **Topology** — may change object topology/storage and therefore is applied off the real-time path;
- **Internal** — model state/control not normally host-visible.

A Trigger is an Event because it has temporal occurrence rather than persistent value.

## Smoothing and automation

M0 provides None, Linear and OnePole smoothing. Smoothing duration is specified in seconds and converted using the prepared sample rate. Reset places current and target at the same deterministic value. Sample-accurate parameter events set a target at their sample offset; subsequent interpolation occurs inside the sample loop.

The Host may deliver block automation points, but the core/model owns interpolation required for physical behavior. Essential pressure/tuning/feedback dynamics must not be relegated to a host-specific smoother.

## Invalid data and thread boundary

Native values are clamped to declared range. Non-finite values use the declared default when passed through `ParameterSpec`, while a smoother ignores a non-finite target and keeps its prior finite target. Parameter metadata/configuration is non-real-time state; real-time changes are immutable Event values or other fixed-size prepared controls.

## Evidence

Tests cover low/high clamp, normalization, denormalization, NaN fallback, linear smoothing duration and deterministic smoothing reset.

**M0.8 Parameter Contract: APPROVED.**
