#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
OUT="${1:-$ROOT/build/resonant-m3-lab}"
SRC="$ROOT/hosts/browser/lab"
SOURCE_COMMIT="${RESONANT_LAB_SOURCE_SHA:-${GITHUB_SHA:-$(git -C "$ROOT" rev-parse HEAD)}}"
TESTED_COMMIT="${RESONANT_LAB_TESTED_SHA:-${GITHUB_SHA:-$SOURCE_COMMIT}}"
EXPORTED="['_re_prepare','_re_reset','_re_panic','_re_set_parameter','_re_parameter_value','_re_note_on','_re_note_off','_re_process','_re_output_ptr','_re_parameter_count','_re_parameter_id','_re_parameter_min','_re_parameter_max','_re_parameter_default','_re_resonator_energy','_re_core_output_rms','_re_core_peak','_re_stability_state','_re_active_voices','_re_held_voices','_re_max_active_voices','_re_maximum_polyphony','_re_voice_steals','_re_protected_state','_re_overblow_amount','_re_mode_energy','_re_cpu_load','_re_cpu_load_smoothed','_re_cpu_load_max']"

rm -rf "$OUT"
mkdir -p "$OUT"

COMMON=(
  "$SRC/BreathPipeWasm.cpp"
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
cp "$ROOT/lab/contracts/m3-presets.json" "$OUT/presets.json"
cp "$ROOT/lab/contracts/m3-acceptance-tests.json" "$OUT/acceptance-tests.json"

# Keep the M2 source host frozen. The M3 artifact is a projection of that Lab
# with Breath-Pipe-specific labels/evidence fields injected only into the built
# output. This avoids turning M2's diagnostic host into a second implementation.
python3 - "$OUT" <<'PY'
from pathlib import Path
import sys

out = Path(sys.argv[1])
app = out.joinpath("app.js").read_text()
needle = "  [107, 'Interaction'],\n]);"
replacement = """  [107, 'Interaction'],
  [201, 'Pitch Hz'],
  [202, 'Pressure'],
  [203, 'Turbulence'],
  [204, 'Interaction'],
  [205, 'Damping / loss'],
  [206, 'Regeneration'],
  [207, 'Feedback colour'],
  [208, 'Nonlinear drive'],
  [209, 'External excitation'],
  [210, 'Timbre'],
  [901, 'Lab external-audio probe'],
]);"""
if needle not in app:
    raise SystemExit("M3 build could not locate parameter-name insertion point")
app = app.replace(needle, replacement, 1)
app = app.replace(
    "    ENGINE_COMMIT: BUILD_INFO.commit,\n",
    "    ENGINE_COMMIT: BUILD_INFO.commit,\n    MODEL: BUILD_INFO.model ?? 'first-resonator',\n    MILESTONE: BUILD_INFO.milestone ?? 'M2',\n",
    1,
)
out.joinpath("app.js").write_text(app)

worklet = out.joinpath("worklet.js").read_text()
needle = "      protectedState: Boolean(m._re_protected_state()),\n      cpuLoad: m._re_cpu_load(),"
replacement = """      protectedState: Boolean(m._re_protected_state()),
      overblowAmount: typeof m._re_overblow_amount === 'function' ? m._re_overblow_amount() : null,
      modeEnergy: typeof m._re_mode_energy === 'function'
        ? [m._re_mode_energy(0), m._re_mode_energy(1), m._re_mode_energy(2)]
        : null,
      cpuLoad: m._re_cpu_load(),"""
if needle not in worklet:
    raise SystemExit("M3 build could not locate telemetry insertion point")
worklet = worklet.replace(needle, replacement, 1)
out.joinpath("worklet.js").write_text(worklet)

index = out.joinpath("index.html").read_text()
index = index.replace(
    "<title>Resonant Engine Lab</title>",
    "<title>Resonant Engine Lab — M3 Breath Pipe</title>",
    1,
)
index = index.replace(
    "<h1>M2 — Resonant Engine Lab</h1>",
    "<h1>M3 — Breath Pipe Reference Voice Lab</h1>",
    1,
)
index = index.replace("Run all nine", "Run all", 1)
out.joinpath("index.html").write_text(index)
PY

cat > "$OUT/build-info.js" <<EOF
export const BUILD_INFO = Object.freeze({ commit: '${SOURCE_COMMIT}', testedCommit: '${TESTED_COMMIT}', model: 'breath-pipe', milestone: 'M3' });
EOF

printf 'Built Resonant Engine M3 Breath Pipe Lab at %s for source %s (tested %s)\n' "$OUT" "$SOURCE_COMMIT" "$TESTED_COMMIT"
