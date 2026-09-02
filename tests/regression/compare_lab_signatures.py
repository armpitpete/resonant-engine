#!/usr/bin/env python3
import json
import math
import pathlib
import sys

if len(sys.argv) != 3:
    raise SystemExit("usage: compare_lab_signatures.py NATIVE_JSON WASM_JSON")

native = json.loads(pathlib.Path(sys.argv[1]).read_text())
wasm = json.loads(pathlib.Path(sys.argv[2]).read_text())

for key in ("maxVoices", "steals"):
    if native[key] != wasm[key]:
        raise SystemExit(f"parity mismatch {key}: native={native[key]} wasm={wasm[key]}")

for key in ("sum", "sumSquares", "peak"):
    a = float(native[key])
    b = float(wasm[key])
    if not math.isfinite(a) or not math.isfinite(b):
        raise SystemExit(f"non-finite parity metric {key}")
    tolerance = 1.0e-3 + 2.0e-3 * max(abs(a), abs(b))
    if abs(a - b) > tolerance:
        raise SystemExit(
            f"parity mismatch {key}: native={a:.12g} wasm={b:.12g} tolerance={tolerance:.12g}"
        )

print("Native/WASM Lab parity PASS")
