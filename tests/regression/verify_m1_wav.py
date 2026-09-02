#!/usr/bin/env python3
import math
import pathlib
import struct
import sys

if len(sys.argv) != 3:
    raise SystemExit("usage: verify_m1_wav.py FILE SCENE")

path = pathlib.Path(sys.argv[1])
scene = sys.argv[2]
data = path.read_bytes()
if len(data) < 44 or data[:4] != b"RIFF" or data[8:12] != b"WAVE":
    raise SystemExit("invalid WAV header")

fmt = struct.unpack_from("<HHIIHH", data, 20)
format_tag, channels, sample_rate, byte_rate, block_align, bits = fmt
if (format_tag, channels, bits) != (1, 1, 16):
    raise SystemExit(f"unexpected WAV format: {fmt}")
if sample_rate != 48000 or byte_rate != 96000 or block_align != 2:
    raise SystemExit(f"unexpected WAV rate fields: {fmt}")

size = struct.unpack_from("<I", data, 40)[0]
if size != len(data) - 44 or size != 48000 * 2:
    raise SystemExit(f"unexpected M1 data size {size} file={len(data)}")

samples = struct.unpack_from(f"<{size // 2}h", data, 44)
peak = max(abs(value) for value in samples)
rms = math.sqrt(sum(float(value) * float(value) for value in samples) / len(samples))

if scene == "silent":
    if peak != 0:
        raise SystemExit("M1 silent fixture contains non-zero PCM")
    print("PASS: M1 silent render")
    raise SystemExit(0)

if peak == 0 or rms < 2.0:
    raise SystemExit(f"M1 {scene} fixture is effectively silent: peak={peak} rms={rms}")

full_scale_count = sum(1 for value in samples if abs(value) == 32767)
if full_scale_count > len(samples) // 100:
    raise SystemExit(
        f"M1 {scene} fixture clips too often: {full_scale_count}/{len(samples)}")

if scene == "passive-pluck":
    middle = samples[4_000:16_000]
    tail = samples[-8_000:]
    middle_rms = math.sqrt(
        sum(float(value) * float(value) for value in middle) / len(middle))
    tail_rms = math.sqrt(
        sum(float(value) * float(value) for value in tail) / len(tail))
    if middle_rms <= 2.0 or tail_rms >= middle_rms * 0.65:
        raise SystemExit(
            f"passive pluck did not decay enough: middle={middle_rms} tail={tail_rms}")

if scene == "nonlinear" and peak < 500:
    raise SystemExit(f"nonlinear fixture lacks useful level: peak={peak}")

print(f"PASS: M1 {scene} WAV peak={peak} rms={rms:.3f}")
