#!/usr/bin/env python3
import pathlib
import struct
import sys

p = pathlib.Path(sys.argv[1])
data = p.read_bytes()
if len(data) < 44 or data[:4] != b"RIFF" or data[8:12] != b"WAVE":
    raise SystemExit("invalid WAV header")
fmt = struct.unpack_from("<HHIIHH", data, 20)
format_tag, channels, sample_rate, byte_rate, block_align, bits = fmt
if (format_tag, channels, bits) != (1, 1, 16):
    raise SystemExit(f"unexpected WAV format: {fmt}")
if sample_rate != 48000 or byte_rate != 96000 or block_align != 2:
    raise SystemExit(f"unexpected WAV rate fields: {fmt}")
size = struct.unpack_from("<I", data, 40)[0]
if size != len(data) - 44 or size != 4800 * 2:
    raise SystemExit(f"unexpected data size {size} file={len(data)}")
print("PASS: WAV structure")

if len(sys.argv) > 2 and sys.argv[2] == "silent":
    if any(data[44:]):
        raise SystemExit("silent WAV contains non-zero PCM bytes")
    print("PASS: silent WAV payload")
