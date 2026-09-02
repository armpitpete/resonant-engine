#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
OUT="${1:-$ROOT/build/resonant-lab}"
SRC="$ROOT/hosts/browser/lab"
SOURCE_COMMIT="${RESONANT_LAB_SOURCE_SHA:-${GITHUB_SHA:-$(git -C "$ROOT" rev-parse HEAD)}}"
TESTED_COMMIT="${GITHUB_SHA:-$SOURCE_COMMIT}"
EXPORTED="['_re_prepare','_re_reset','_re_panic','_re_set_parameter','_re_parameter_value','_re_note_on','_re_note_off','_re_process','_re_output_ptr','_re_parameter_count','_re_parameter_id','_re_parameter_min','_re_parameter_max','_re_parameter_default','_re_resonator_energy','_re_core_output_rms','_re_core_peak','_re_stability_state','_re_active_voices','_re_held_voices','_re_max_active_voices','_re_maximum_polyphony','_re_voice_steals','_re_protected_state','_re_cpu_load','_re_cpu_load_smoothed','_re_cpu_load_max']"

rm -rf "$OUT"
mkdir -p "$OUT"

COMMON=(
  "$SRC/LabWasm.cpp"
  -I"$ROOT/core/include"
  -I"$ROOT/lab/include"
  -std=c++20 -O3 -fno-exceptions -fno-rtti
  -Wall -Wextra -Wpedantic -Werror
  -sMODULARIZE=1
  -sEXPORT_ES6=1
  -sSINGLE_FILE=1
  -sFILESYSTEM=0
  -sALLOW_MEMORY_GROWTH=0
  -sINITIAL_MEMORY=16777216
  -sSTACK_SIZE=262144
  -sEXPORTED_FUNCTIONS="$EXPORTED"
  -sEXPORTED_RUNTIME_METHODS="['HEAPF32']"
)

em++ "${COMMON[@]}" -sENVIRONMENT=worklet -o "$OUT/resonant-lab.js"

if [[ "${RESONANT_LAB_BUILD_NODE_PARITY:-0}" == "1" ]]; then
  em++ "${COMMON[@]}" -sENVIRONMENT=node -o "$OUT/resonant-lab-node.mjs"
fi

for file in index.html app.js worklet.js capture-worklet.js style.css; do
  cp "$SRC/$file" "$OUT/$file"
done
cp "$ROOT/lab/contracts/presets.json" "$OUT/presets.json"
cp "$ROOT/lab/contracts/acceptance-tests.json" "$OUT/acceptance-tests.json"
cat > "$OUT/build-info.js" <<EOF
export const BUILD_INFO = Object.freeze({ commit: '${SOURCE_COMMIT}', testedCommit: '${TESTED_COMMIT}' });
EOF

printf 'Built Resonant Engine Lab at %s for source %s (tested %s)\n' "$OUT" "$SOURCE_COMMIT" "$TESTED_COMMIT"
