# M0.7 — Event & Timing Contract

Status: **APPROVED**

## Canonical event

`resonant::Event` is a fixed-size, allocation-free value containing:

- `sample_offset`: zero-based sample timestamp within the current block;
- `type`;
- generic `target` identifier;
- optional `note_id` reserved for per-note/MPE-style expression;
- two finite float payload values.

Canonical types are NoteOff, Pitch, Pressure, ParameterChange, PerNoteExpression, NoteOn and Trigger. Note velocity is the NoteOn value. Pitch and pressure are continuous expression events. Trigger is an instantaneous event, not a latched parameter.

## Ordering

Events are ordered by:

1. sample offset;
2. deterministic type priority: NoteOff → Pitch → Pressure → ParameterChange → PerNoteExpression → NoteOn → Trigger;
3. note id;
4. target id.

This makes simultaneous-event behavior deterministic without depending on host container order. A host may pre-sort; `FixedEventBuffer` can insert into order with fixed capacity and no allocation.

## Sample boundaries

- offset 0 applies before sample 0;
- `frames - 1` applies before the final sample;
- offset `>= frames` is not an event in the current block and is rejected by the core call;
- future/deferred events remain owned by the Host and are re-timestamped into a later block;
- cross-block scheduling therefore never requires the core to allocate or retain an unbounded queue.

## Capacity and malformed data

M0 maximum is 1024 events per block. Overflow is explicit: `FixedEventBuffer::push()` returns false and exposes `overflowed()`. A host must choose a policy before calling the core (drop with diagnostics, split scheduling, or fail safely); silent heap growth is prohibited.

Non-finite payloads, out-of-range offsets or out-of-order caller spans are malformed and rejected. Model-specific range clamping happens under the Parameter contract.

## MPE/per-note reservation

`note_id` and `PerNoteExpression` reserve a stable route for future per-note pitch, pressure and timbral controls without making MIDI or MPE device APIs part of the core.

## Evidence

Unit tests prove deterministic insertion order, overflow reporting, events at sample 0/final sample, malformed-order rejection and a control change that is silent through sample 7 and audible exactly at sample 8.

**M0.7 Event & Timing Contract: APPROVED.**
