import { pathToFileURL } from 'node:url';
import { resolve } from 'node:path';

const modulePath = process.argv[2];
if (!modulePath) throw new Error('Usage: node m3-parity.mjs <resonant-lab-node.mjs>');
const { default: createModule } = await import(pathToFileURL(resolve(modulePath)).href);
const m = await createModule({ noInitialRun: true });
if (!m._re_prepare(48000)) throw new Error('WASM Breath Pipe Lab prepare failed');

for (const [id, value] of [[202, 0.62], [203, 0.31], [204, 0.73], [205, 0.08], [206, 0.42], [207, 0.37], [208, 0.55], [209, 0.0], [210, 0.44]]) {
  if (!m._re_set_parameter(id, value)) throw new Error(`Parameter ${id} rejected`);
}
if (!m._re_note_on(60, 0.82)) throw new Error('First note rejected');

let sum = 0;
let sumSquares = 0;
let peak = 0;
for (let block = 0; block < 320; block += 1) {
  if (block === 96 && !m._re_note_on(67, 0.68)) throw new Error('Second note rejected');
  if (block === 176 && !m._re_set_parameter(202, 0.92)) throw new Error('Pressure change rejected');
  if (block === 224 && !m._re_note_off(60)) throw new Error('First note off rejected');
  if (block === 272 && !m._re_note_off(67)) throw new Error('Second note off rejected');
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
  overblow: m._re_overblow_amount(),
}));
