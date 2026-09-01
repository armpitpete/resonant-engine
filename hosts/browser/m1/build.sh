#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
OUT="${1:-$ROOT/build/m1-browser}"
SRC="$ROOT/hosts/browser/m1"

rm -rf "$OUT"
mkdir -p "$OUT"

em++ "$SRC/M1Wasm.cpp" \
  -I"$ROOT/core/include" \
  -std=c++20 -O3 -fno-exceptions -fno-rtti \
  -Wall -Wextra -Wpedantic -Werror \
  -sMODULARIZE=1 \
  -sEXPORT_ES6=1 \
  -sENVIRONMENT=worklet \
  -sSINGLE_FILE=1 \
  -sFILESYSTEM=0 \
  -sALLOW_MEMORY_GROWTH=0 \
  -sINITIAL_MEMORY=4194304 \
  -sSTACK_SIZE=131072 \
  -sEXPORTED_FUNCTIONS="['_re_prepare','_re_reset','_re_set_parameter','_re_set_pitch','_re_trigger','_re_note_on','_re_process','_re_output_ptr']" \
  -sEXPORTED_RUNTIME_METHODS="['HEAPF32']" \
  -o "$OUT/resonant-m1.js"

cp "$SRC/index.html" "$OUT/index.html"
cp "$SRC/app.js" "$OUT/app.js"
cp "$SRC/worklet.js" "$OUT/worklet.js"
cp "$SRC/style.css" "$OUT/style.css"
cp "$SRC/RUN-M1-SYNTH.bat" "$OUT/RUN-M1-SYNTH.bat"
cp "$SRC/run-m1-synth.sh" "$OUT/run-m1-synth.sh"
cp "$SRC/README.md" "$OUT/README.md"

printf 'Built M1 browser play-test harness at %s\n' "$OUT"
