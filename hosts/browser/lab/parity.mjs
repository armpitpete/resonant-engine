import { pathToFileURL } from 'node:url';
import { resolve } from 'node:path';

const modulePath = process.argv[2];
if (!modulePath) throw new Error('Usage: node parity.mjs <resonant-lab-node.mjs>');
const { default: createModule } = await import(pathToFileURL(resolve(modulePath)).href);
const m = await createModule({ noInitialRun: true });
if (!m._re_prepare(48000)) throw new Error('WASM lab prepare failed');

for (const [id, value] of [[102, 0.27], [103, 0.46], [104, 0.35], [105, 0.31], [106, 0.71], [107, 0.22]]) {
  if (!m._re_set_parameter(id, value)) throw new Error(`Parameter ${id} rejected`);
}
if (!m._re_note_on(60, 0.80)) throw new Error('First note rejected');

let sum = 0;
let sumSquares = 0;
let peak = 0;
for (let block = 0; block < 240; block += 1) {
  if (block === 80 && !m._re_note_on(64, 0.65)) throw new Error('Second note rejected');
  if (block === 160 && !m._re_note_off(60)) throw new Error('First note off rejected');
  if (block === 200 && !m._re_note_off(64)) throw new Error('Second note off rejected');
  if (!m._re_process(128)) throw new Error(`WASM process failed at block ${block}`);
  const ptr = m._re_output_ptr() >>> 2;
  const output = m.HEAPF32.subarray(ptr, ptr + 128);
  for (const sample of output) {
    sum += sample;
    sumSquares += sample * sample;
    peak = Math.max(peak, Math.abs(sample));
  }
}
console.log(JSON.stringify({
  sum,
  sumSquares,
  peak,
  maxVoices: m._re_max_active_voices(),
  steals: m._re_voice_steals(),
}));
