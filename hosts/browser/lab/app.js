import { BUILD_INFO } from './build-info.js';

const PARAMETER_NAMES = new Map([
  [101, 'Tuning Hz'],
  [102, 'Damping'],
  [103, 'Regeneration'],
  [104, 'Nonlinearity'],
  [105, 'Excitation'],
  [106, 'Turbulence'],
  [107, 'Interaction'],
]);

const STABILITY_NAMES = ['QUIET', 'ACTIVE', 'HIGH_ENERGY', 'SELF_OSCILLATING', 'NEAR_LIMIT', 'UNSTABLE', 'PROTECTED'];

const state = {
  context: null,
  labNode: null,
  captureNode: null,
  analyser: null,
  gain: null,
  ready: false,
  parameters: new Map(),
  presets: [],
  tests: [],
  telemetry: null,
  analysis: { f0: null, rms: 0, peak: 0, dc: 0 },
  lastF0At: 0,
  run: null,
  queue: [],
  runAllActive: false,
  batchEvidence: [],
  lastEvidence: null,
  captureChunks: [],
  captureActive: false,
  lastWav: null,
};

const $ = (selector) => document.querySelector(selector);
const statusEl = $('#status');
const runnerStatusEl = $('#runner-status');
const parameterControlsEl = $('#parameter-controls');
const presetEl = $('#preset');
const testSelectEl = $('#test-select');
const evidencePreviewEl = $('#evidence-preview');

function setStatus(text, kind = '') {
  statusEl.textContent = text;
  statusEl.dataset.kind = kind;
}

function midiToHz(note) {
  return 440 * Math.pow(2, (Number(note) - 69) / 12);
}

function frameAt(ms) {
  return Math.max(0, Math.round(Number(ms) * state.context.sampleRate / 1000));
}

function post(message) {
  if (state.labNode) state.labNode.port.postMessage(message);
}

async function loadContracts() {
  const [presetsResponse, testsResponse] = await Promise.all([
    fetch('./presets.json'),
    fetch('./acceptance-tests.json'),
  ]);
  if (!presetsResponse.ok || !testsResponse.ok) throw new Error('Failed to load Lab contracts');
  const presetsDocument = await presetsResponse.json();
  const testsDocument = await testsResponse.json();
  state.presets = presetsDocument.presets ?? [];
  state.tests = testsDocument.tests ?? [];
  presetEl.innerHTML = state.presets.map((preset) => `<option value="${preset.id}">${preset.name}</option>`).join('');
  testSelectEl.innerHTML = state.tests.map((test) => `<option value="${test.id}">${test.id} — ${test.name}</option>`).join('');
  updateTestDescription();
}

function parameterStep(id, min, max) {
  if (id === 101) return 0.1;
  const span = max - min;
  return span <= 2 ? 0.01 : Math.max(0.1, span / 1000);
}

function renderParameterControls(parameters) {
  state.parameters.clear();
  parameterControlsEl.innerHTML = '';
  for (const meta of parameters) {
    state.parameters.set(meta.id, { ...meta, value: meta.defaultValue });
    const step = parameterStep(meta.id, meta.min, meta.max);
    const wrapper = document.createElement('label');
    wrapper.className = 'parameter';
    wrapper.innerHTML = `
      <span>${PARAMETER_NAMES.get(meta.id) ?? `Parameter ${meta.id}`}</span>
      <input class="parameter-range" type="range" data-id="${meta.id}" min="${meta.min}" max="${meta.max}" step="${step}" value="${meta.defaultValue}">
      <input class="parameter-number" type="number" data-id="${meta.id}" min="${meta.min}" max="${meta.max}" step="${step}" value="${meta.defaultValue}">
    `;
    parameterControlsEl.append(wrapper);
  }
  for (const input of parameterControlsEl.querySelectorAll('input')) {
    input.addEventListener('input', () => setParameterFromUi(Number(input.dataset.id), Number(input.value), input));
  }
}

function setParameterFromUi(id, value, source) {
  const meta = state.parameters.get(id);
  if (!meta) return;
  const clamped = Math.min(meta.max, Math.max(meta.min, Number.isFinite(value) ? value : meta.defaultValue));
  meta.value = clamped;
  for (const input of parameterControlsEl.querySelectorAll(`[data-id="${id}"]`)) {
    if (input !== source) input.value = String(clamped);
  }
  if (state.ready) post({ type: 'param', id, value: clamped });
}

function applyPreset(id, send = true) {
  const preset = state.presets.find((item) => item.id === id);
  if (!preset) return;
  presetEl.value = id;
  for (const [rawId, rawValue] of Object.entries(preset.parameters ?? {})) {
    const parameterId = Number(rawId);
    const meta = state.parameters.get(parameterId);
    const value = Number(rawValue);
    if (meta) {
      meta.value = value;
      for (const input of parameterControlsEl.querySelectorAll(`[data-id="${parameterId}"]`)) input.value = String(value);
    }
    if (send && state.ready) post({ type: 'param', id: parameterId, value });
  }
}

async function startAudio() {
  if (state.context) {
    await state.context.resume();
    setStatus('Lab running', 'ok');
    $('#stop').disabled = false;
    return;
  }
  $('#start').disabled = true;
  setStatus('Starting Lab…');
  try {
    const context = new AudioContext({ latencyHint: 'interactive' });
    await Promise.all([
      context.audioWorklet.addModule('./worklet.js'),
      context.audioWorklet.addModule('./capture-worklet.js'),
    ]);
    const labNode = new AudioWorkletNode(context, 'resonant-lab', {
      numberOfInputs: 0,
      numberOfOutputs: 1,
      outputChannelCount: [1],
    });
    const analyser = context.createAnalyser();
    analyser.fftSize = 8192;
    analyser.smoothingTimeConstant = 0.65;
    analyser.minDecibels = -100;
    analyser.maxDecibels = 0;
    const captureNode = new AudioWorkletNode(context, 'resonant-capture', {
      numberOfInputs: 1,
      numberOfOutputs: 1,
      outputChannelCount: [1],
    });
    const gain = context.createGain();
    gain.gain.value = Number($('#monitor-level').value);
    labNode.connect(analyser).connect(captureNode).connect(gain).connect(context.destination);

    state.context = context;
    state.labNode = labNode;
    state.captureNode = captureNode;
    state.analyser = analyser;
    state.gain = gain;

    captureNode.port.onmessage = (event) => {
      if (event.data?.type === 'audio' && state.captureActive) state.captureChunks.push(event.data.samples);
    };
    labNode.port.onmessage = handleLabMessage;
    await context.resume();
    requestAnimationFrame(analyseLoop);
  } catch (error) {
    setStatus(String(error), 'error');
    $('#start').disabled = false;
  }
}

async function stopAudio() {
  if (!state.context) return;
  await state.context.suspend();
  setStatus('Audio stopped');
  $('#stop').disabled = true;
}

function enableLabControls() {
  for (const id of ['reset', 'panic', 'note-on', 'note-off', 'run-test', 'run-all']) $(`#${id}`).disabled = false;
  $('#stop').disabled = false;
  $('#export-plots').disabled = false;
}

function handleLabMessage(event) {
  const message = event.data;
  if (message.type === 'ready') {
    state.ready = true;
    renderParameterControls(message.parameters);
    applyPreset(presetEl.value || 'neutral');
    setStatus(`Lab running — ${Math.round(message.sampleRate)} Hz`, 'ok');
    enableLabControls();
    return;
  }
  if (message.type === 'telemetry') {
    state.telemetry = message;
    updateTelemetryUi();
    aggregateTelemetry(message);
    return;
  }
  if (message.type === 'scenarioStarted') {
    runnerStatusEl.textContent = `${message.id} running…`;
    return;
  }
  if (message.type === 'scenarioMark') {
    captureMark(message.data ?? {});
    return;
  }
  if (message.type === 'hardFailure') {
    if (state.run) state.run.hardFailures.push(message.telemetry);
    runnerStatusEl.textContent = 'Hard failure detected; evidence retained.';
    return;
  }
  if (message.type === 'scenarioComplete') {
    completeRun(message.telemetry);
    return;
  }
  if (message.type === 'scenarioAborted') {
    state.run = null;
    state.queue = [];
    state.runAllActive = false;
    stopCapture();
    setRunnerButtons(false);
    runnerStatusEl.textContent = 'Test aborted; engine panicked.';
    return;
  }
  if (message.type === 'error') setStatus(message.message, 'error');
}

function updateTelemetryUi() {
  const t = state.telemetry;
  if (!t) return;
  $('#metric-energy').textContent = t.resonatorEnergy.toExponential(3);
  $('#metric-stability').textContent = STABILITY_NAMES[t.stability] ?? `STATE_${t.stability}`;
  $('#metric-voices').textContent = `${t.activeVoices} / ${t.maximumPolyphony}`;
  const load = Number(t.cpuLoadSmoothed);
  const label = load > 100 ? 'realtime failure' : load >= 80 ? 'realtime risk' : load >= 50 ? 'elevated' : 'healthy';
  $('#metric-cpu').textContent = `${load.toFixed(1)}% — ${label}`;
}

function analyseLoop(timestamp) {
  if (!state.analyser || !state.context) return;
  const timeData = new Float32Array(state.analyser.fftSize);
  const frequencyData = new Float32Array(state.analyser.frequencyBinCount);
  state.analyser.getFloatTimeDomainData(timeData);
  state.analyser.getFloatFrequencyData(frequencyData);

  let sum = 0;
  let sumSquares = 0;
  let peak = 0;
  for (const sample of timeData) {
    sum += sample;
    sumSquares += sample * sample;
    peak = Math.max(peak, Math.abs(sample));
  }
  state.analysis.dc = sum / timeData.length;
  state.analysis.rms = Math.sqrt(sumSquares / timeData.length);
  state.analysis.peak = peak;
  if (timestamp - state.lastF0At >= 250) {
    state.analysis.f0 = estimateFundamental(timeData, state.context.sampleRate);
    state.lastF0At = timestamp;
  }

  $('#metric-f0').textContent = state.analysis.f0 ? `${state.analysis.f0.toFixed(2)} Hz` : '—';
  $('#metric-rms').textContent = state.analysis.rms.toFixed(5);
  $('#metric-peak').textContent = state.analysis.peak.toFixed(5);
  $('#metric-dc').textContent = state.analysis.dc.toExponential(3);
  drawWaveform(timeData);
  drawSpectrum(frequencyData, state.context.sampleRate);
  requestAnimationFrame(analyseLoop);
}

function estimateFundamental(samples, sampleRate) {
  const stride = 2;
  const dataLength = Math.floor(samples.length / stride);
  const data = new Float32Array(dataLength);
  let mean = 0;
  for (let i = 0; i < dataLength; i += 1) {
    data[i] = samples[i * stride];
    mean += data[i];
  }
  mean /= dataLength;
  let energy = 0;
  for (let i = 0; i < dataLength; i += 1) {
    data[i] -= mean;
    energy += data[i] * data[i];
  }
  if (Math.sqrt(energy / dataLength) < 1e-4) return null;
  const effectiveRate = sampleRate / stride;
  const minLag = Math.max(2, Math.floor(effectiveRate / 2000));
  const maxLag = Math.min(Math.floor(effectiveRate / 50), Math.floor(dataLength / 2));
  const correlations = new Float64Array(maxLag + 1);
  let bestLag = 0;
  let best = -Infinity;
  for (let lag = minLag; lag <= maxLag; lag += 1) {
    let corr = 0;
    const count = dataLength - lag;
    for (let i = 0; i < count; i += 1) corr += data[i] * data[i + lag];
    corr /= count;
    correlations[lag] = corr;
    if (corr > best) {
      best = corr;
      bestLag = lag;
    }
  }
  if (bestLag === 0 || best <= 0) return null;
  let refined = bestLag;
  if (bestLag > minLag && bestLag < maxLag) {
    const left = correlations[bestLag - 1];
    const centre = correlations[bestLag];
    const right = correlations[bestLag + 1];
    const denominator = left - 2 * centre + right;
    if (Math.abs(denominator) > 1e-12) refined += 0.5 * (left - right) / denominator;
  }
  const hz = effectiveRate / refined;
  return Number.isFinite(hz) && hz >= 50 && hz <= 2000 ? hz : null;
}

function drawWaveform(samples) {
  const canvas = $('#waveform');
  const ctx = canvas.getContext('2d');
  const { width, height } = canvas;
  ctx.clearRect(0, 0, width, height);
  ctx.beginPath();
  for (let x = 0; x < width; x += 1) {
    const index = Math.floor(x * samples.length / width);
    const y = height * (0.5 - 0.45 * samples[index]);
    if (x === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
  }
  ctx.strokeStyle = '#ddd';
  ctx.stroke();
}

function drawSpectrum(data, sampleRate) {
  const canvas = $('#spectrum');
  const ctx = canvas.getContext('2d');
  const { width, height } = canvas;
  ctx.clearRect(0, 0, width, height);
  ctx.beginPath();
  for (let x = 0; x < width; x += 1) {
    const ratio = x / Math.max(1, width - 1);
    const frequency = 20 * Math.pow(1000, ratio);
    const bin = Math.min(data.length - 1, Math.round(frequency / (sampleRate / 2) * data.length));
    const db = Math.max(-100, Math.min(0, data[bin]));
    const y = height * (1 - (db + 100) / 100);
    if (x === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
  }
  ctx.strokeStyle = '#ddd';
  ctx.stroke();
}

function updateTestDescription() {
  const test = state.tests.find((item) => item.id === testSelectEl.value) ?? state.tests[0];
  $('#test-expected').textContent = test ? `${test.id}: ${test.expected}` : '';
}

function addPrimitive(actions, atMs, action) {
  actions.push({ ...action, atFrame: frameAt(atMs) });
}

function compileTest(test) {
  const actions = [];
  const preset = state.presets.find((item) => item.id === test.preset);
  for (const [id, value] of Object.entries(preset?.parameters ?? {})) {
    addPrimitive(actions, 0, { type: 'param', id: Number(id), value: Number(value) });
  }

  for (const op of test.program ?? []) {
    const at = Number(op.at_ms ?? 0);
    switch (op.op) {
      case 'note_on': addPrimitive(actions, at, { type: 'noteOn', note: op.note, velocity: op.velocity }); break;
      case 'note_off': addPrimitive(actions, at, { type: 'noteOff', note: op.note }); break;
      case 'set_parameter': addPrimitive(actions, at, { type: 'param', id: op.id, value: op.value }); break;
      case 'panic': addPrimitive(actions, at, { type: 'panic' }); break;
      case 'reset': addPrimitive(actions, at, { type: 'reset' }); break;
      case 'mark': addPrimitive(actions, at, { type: 'mark', data: { kind: op.label, note: op.note ?? null } }); break;
      case 'sweep_parameter': {
        const steps = Math.max(1, Number(op.steps));
        for (let i = 0; i <= steps; i += 1) {
          const fraction = i / steps;
          addPrimitive(actions, at + Number(op.duration_ms) * fraction, {
            type: 'param', id: op.id, value: Number(op.from) + (Number(op.to) - Number(op.from)) * fraction,
          });
        }
        break;
      }
      case 'pitch_run': {
        let cursor = at;
        for (let note = Number(op.start_midi); note <= Number(op.end_midi); note += 1) {
          addPrimitive(actions, cursor, { type: 'noteOn', note, velocity: Number(op.velocity) });
          addPrimitive(actions, cursor + Number(op.settle_ms), {
            type: 'mark', data: { kind: 'pitch', note, targetHz: midiToHz(note) },
          });
          addPrimitive(actions, cursor + Number(op.note_ms), { type: 'noteOff', note });
          cursor += Number(op.note_ms) + Number(op.gap_ms);
        }
        addPrimitive(actions, cursor, { type: 'panic' });
        break;
      }
      case 'velocity_run': {
        let cursor = at;
        for (const velocity of op.values ?? []) {
          addPrimitive(actions, cursor, { type: 'noteOn', note: op.note, velocity: Number(velocity) / 127 });
          addPrimitive(actions, cursor + Number(op.settle_ms), {
            type: 'mark', data: { kind: 'velocity', velocity: Number(velocity), note: op.note },
          });
          addPrimitive(actions, cursor + Number(op.note_ms), { type: 'noteOff', note: op.note });
          addPrimitive(actions, cursor + Number(op.note_ms) + 20, { type: 'panic' });
          cursor += Number(op.note_ms) + Number(op.gap_ms);
        }
        break;
      }
      case 'polyphony_run': {
        let cursor = at;
        for (const count of op.counts ?? []) {
          addPrimitive(actions, cursor, { type: 'panic' });
          const notes = (op.notes ?? []).slice(0, Number(count));
          for (const note of notes) addPrimitive(actions, cursor + 10, { type: 'noteOn', note, velocity: Number(op.velocity) });
          addPrimitive(actions, cursor + Number(op.stage_ms) * 0.6, {
            type: 'mark', data: { kind: 'polyphony', count: Number(count), notes },
          });
          for (const note of notes) addPrimitive(actions, cursor + Number(op.stage_ms), { type: 'noteOff', note });
          addPrimitive(actions, cursor + Number(op.stage_ms) + 30, { type: 'panic' });
          cursor += Number(op.stage_ms) + Number(op.gap_ms);
        }
        break;
      }
      case 'chord_on':
        for (const note of op.notes ?? []) addPrimitive(actions, at, { type: 'noteOn', note, velocity: Number(op.velocity) });
        break;
      case 'rapid_retrigger': {
        let cursor = at;
        const notes = op.notes ?? [];
        for (let index = 0; index < Number(op.count); index += 1) {
          const note = notes[index % notes.length];
          addPrimitive(actions, cursor, { type: 'noteOn', note, velocity: Number(op.velocity) });
          addPrimitive(actions, cursor + Number(op.interval_ms) * 0.55, { type: 'noteOff', note });
          cursor += Number(op.interval_ms);
        }
        break;
      }
      default: throw new Error(`Unknown test operation: ${op.op}`);
    }
  }
  return actions.sort((a, b) => a.atFrame - b.atFrame);
}

function setRunnerButtons(running) {
  $('#run-test').disabled = running || !state.ready;
  $('#run-all').disabled = running || !state.ready;
  $('#abort-test').disabled = !running;
}

function startCapture() {
  if (!state.captureNode) return;
  state.captureChunks = [];
  state.captureActive = true;
  state.captureNode.port.postMessage({ type: 'start' });
}

function stopCapture() {
  if (!state.captureNode || !state.captureActive) return;
  state.captureNode.port.postMessage({ type: 'stop' });
  state.captureActive = false;
  state.lastWav = concatenateChunks(state.captureChunks);
  $('#export-wav').disabled = !state.lastWav || state.lastWav.length === 0;
}

function runTest(testId, fromQueue = false) {
  const test = state.tests.find((item) => item.id === testId);
  if (!test || !state.ready) return;
  if (!fromQueue) {
    state.queue = [];
    state.runAllActive = false;
    state.batchEvidence = [];
  }
  applyPreset(test.preset);
  state.lastWav = null;
  $('#export-wav').disabled = true;
  state.run = {
    test,
    startedAt: new Date().toISOString(),
    hardFailures: [],
    marks: [],
    stabilityTransitions: [],
    previousStability: null,
    maxima: { cpuLoad: 0, corePeak: 0, resonatorEnergy: 0, activeVoices: 0 },
  };
  $('#listener-notes').value = '';
  $('#human-result').value = 'INVESTIGATE';
  if ($('#record-wav').checked) startCapture();
  setRunnerButtons(true);
  runnerStatusEl.textContent = `${test.id} preparing…`;
  post({ type: 'scenario', id: test.id, durationMs: test.duration_ms, actions: compileTest(test) });
}

function aggregateTelemetry(telemetry) {
  if (!state.run) return;
  const maxima = state.run.maxima;
  maxima.cpuLoad = Math.max(maxima.cpuLoad, Number(telemetry.cpuLoadMax ?? telemetry.cpuLoad ?? 0));
  maxima.corePeak = Math.max(maxima.corePeak, Number(telemetry.corePeak ?? 0));
  maxima.resonatorEnergy = Math.max(maxima.resonatorEnergy, Number(telemetry.resonatorEnergy ?? 0));
  maxima.activeVoices = Math.max(maxima.activeVoices, Number(telemetry.activeVoices ?? 0));
  if (telemetry.stability !== state.run.previousStability) {
    state.run.stabilityTransitions.push({
      atFrame: telemetry.scenarioFrame,
      state: STABILITY_NAMES[telemetry.stability] ?? `STATE_${telemetry.stability}`,
    });
    state.run.previousStability = telemetry.stability;
  }
  if (telemetry.protectedState && state.run.hardFailures.length === 0) {
    state.run.hardFailures.push({ reason: 'protected-state', telemetry });
  }
}

function captureMark(data) {
  if (!state.run) return;
  const snapshot = {
    ...data,
    analysis: { ...state.analysis },
    telemetry: state.telemetry ? { ...state.telemetry } : null,
  };
  if (data.kind === 'pitch' && data.targetHz && state.analysis.f0) {
    snapshot.centsError = 1200 * Math.log2(state.analysis.f0 / Number(data.targetHz));
  }
  state.run.marks.push(snapshot);
}

function buildEvidence(run, finalTelemetry) {
  const automatedStatus = run.hardFailures.length > 0
    ? 'FAIL'
    : run.maxima.cpuLoad > 100
      ? 'INVESTIGATE'
      : 'PASS';
  return {
    schema: 'resonant-engine-lab-evidence/v1',
    TEST: `${run.test.id} — ${run.test.name}`,
    ENGINE_COMMIT: BUILD_INFO.commit,
    BUILD: 'WASM AudioWorklet / Resonant Engine Lab',
    BROWSER: navigator.userAgent,
    SAMPLE_RATE: state.context?.sampleRate ?? null,
    BLOCK_SIZE: 128,
    POLYPHONY: finalTelemetry?.maximumPolyphony ?? state.telemetry?.maximumPolyphony ?? null,
    PRESET: run.test.preset,
    PARAMETERS: Object.fromEntries([...state.parameters].map(([id, meta]) => [id, meta.value])),
    ACTION: run.test.program,
    EXPECTED: run.test.expected,
    OBSERVED: {
      marks: run.marks,
      stabilityTransitions: run.stabilityTransitions,
      maxima: run.maxima,
      finalTelemetry,
      hardFailures: run.hardFailures,
    },
    MEASUREMENTS: {
      waveformSource: 'AnalyserNode time-domain output',
      spectrumSource: 'AnalyserNode FFT',
      fundamentalEstimator: 'decimated autocorrelation, 50-2000 Hz',
      final: { ...state.analysis },
    },
    AUTOMATED_RESULT: automatedStatus,
    LISTENER_NOTES: '',
    RESULT: 'INVESTIGATE',
    STARTED_AT: run.startedAt,
    COMPLETED_AT: new Date().toISOString(),
  };
}

function completeRun(finalTelemetry) {
  if (!state.run) return;
  stopCapture();
  const evidence = buildEvidence(state.run, finalTelemetry);
  state.lastEvidence = evidence;
  state.batchEvidence.push(evidence);
  state.run = null;
  setRunnerButtons(false);
  $('#export-json').disabled = false;
  evidencePreviewEl.textContent = JSON.stringify(evidence, null, 2);
  runnerStatusEl.textContent = `${evidence.TEST}: automated ${evidence.AUTOMATED_RESULT}. Human listening result remains separate.`;

  if (state.runAllActive && state.queue.length > 0) {
    const next = state.queue.shift();
    setTimeout(() => runTest(next, true), 150);
  } else if (state.runAllActive) {
    state.runAllActive = false;
    state.lastEvidence = {
      schema: 'resonant-engine-lab-evidence-batch/v1',
      ENGINE_COMMIT: BUILD_INFO.commit,
      runs: state.batchEvidence,
    };
    evidencePreviewEl.textContent = JSON.stringify(state.lastEvidence, null, 2);
    runnerStatusEl.textContent = `All ${state.batchEvidence.length} canonical tests completed. Human listening verdicts remain required.`;
  }
}

function concatenateChunks(chunks) {
  const length = chunks.reduce((total, chunk) => total + chunk.length, 0);
  if (length === 0) return null;
  const output = new Float32Array(length);
  let offset = 0;
  for (const chunk of chunks) {
    output.set(chunk, offset);
    offset += chunk.length;
  }
  return output;
}

function evidenceForExport() {
  if (!state.lastEvidence) return null;
  const copy = structuredClone(state.lastEvidence);
  if (copy.schema === 'resonant-engine-lab-evidence/v1') {
    copy.LISTENER_NOTES = $('#listener-notes').value.trim();
    copy.RESULT = $('#human-result').value;
  }
  return copy;
}

function downloadBlob(blob, filename) {
  const url = URL.createObjectURL(blob);
  const link = document.createElement('a');
  link.href = url;
  link.download = filename;
  link.click();
  setTimeout(() => URL.revokeObjectURL(url), 1000);
}

function exportEvidence() {
  const evidence = evidenceForExport();
  if (!evidence) return;
  const id = evidence.TEST?.slice(0, 3) ?? 'batch';
  downloadBlob(new Blob([JSON.stringify(evidence, null, 2)], { type: 'application/json' }), `resonant-lab-${id}-${BUILD_INFO.commit.slice(0, 12)}.json`);
}

function encodeWav(samples, sampleRate) {
  const buffer = new ArrayBuffer(44 + samples.length * 2);
  const view = new DataView(buffer);
  const write = (offset, text) => { for (let i = 0; i < text.length; i += 1) view.setUint8(offset + i, text.charCodeAt(i)); };
  write(0, 'RIFF');
  view.setUint32(4, 36 + samples.length * 2, true);
  write(8, 'WAVE');
  write(12, 'fmt ');
  view.setUint32(16, 16, true);
  view.setUint16(20, 1, true);
  view.setUint16(22, 1, true);
  view.setUint32(24, sampleRate, true);
  view.setUint32(28, sampleRate * 2, true);
  view.setUint16(32, 2, true);
  view.setUint16(34, 16, true);
  write(36, 'data');
  view.setUint32(40, samples.length * 2, true);
  let offset = 44;
  for (const sample of samples) {
    const clamped = Math.max(-1, Math.min(1, sample));
    view.setInt16(offset, clamped < 0 ? clamped * 32768 : clamped * 32767, true);
    offset += 2;
  }
  return buffer;
}

function exportWav() {
  if (!state.lastWav || !state.context) return;
  const wav = encodeWav(state.lastWav, state.context.sampleRate);
  downloadBlob(new Blob([wav], { type: 'audio/wav' }), `resonant-lab-${BUILD_INFO.commit.slice(0, 12)}.wav`);
}

function exportPlots() {
  const wave = $('#waveform');
  const spectrum = $('#spectrum');
  const canvas = document.createElement('canvas');
  canvas.width = Math.max(wave.width, spectrum.width);
  canvas.height = wave.height + spectrum.height + 40;
  const ctx = canvas.getContext('2d');
  ctx.fillStyle = '#111';
  ctx.fillRect(0, 0, canvas.width, canvas.height);
  ctx.fillStyle = '#eee';
  ctx.fillText(`Resonant Engine Lab ${BUILD_INFO.commit.slice(0, 12)}`, 8, 16);
  ctx.drawImage(wave, 0, 24);
  ctx.drawImage(spectrum, 0, 24 + wave.height + 8);
  canvas.toBlob((blob) => { if (blob) downloadBlob(blob, `resonant-lab-plots-${BUILD_INFO.commit.slice(0, 12)}.png`); }, 'image/png');
}

$('#start').addEventListener('click', startAudio);
$('#stop').addEventListener('click', stopAudio);
$('#reset').addEventListener('click', () => { post({ type: 'reset' }); applyPreset(presetEl.value); });
$('#panic').addEventListener('click', () => post({ type: 'panic' }));
$('#note-on').addEventListener('click', () => post({
  type: 'noteOn', note: Number($('#manual-note').value), velocity: Number($('#manual-velocity').value) / 127,
}));
$('#note-off').addEventListener('click', () => post({ type: 'noteOff', note: Number($('#manual-note').value) }));
$('#monitor-level').addEventListener('input', () => {
  if (state.gain && state.context) state.gain.gain.setTargetAtTime(Number($('#monitor-level').value), state.context.currentTime, 0.01);
});
presetEl.addEventListener('change', () => applyPreset(presetEl.value));
testSelectEl.addEventListener('change', updateTestDescription);
$('#run-test').addEventListener('click', () => runTest(testSelectEl.value));
$('#run-all').addEventListener('click', () => {
  if (!state.ready || state.tests.length === 0) return;
  state.runAllActive = true;
  state.batchEvidence = [];
  state.queue = state.tests.map((test) => test.id);
  const first = state.queue.shift();
  runTest(first, true);
});
$('#abort-test').addEventListener('click', () => post({ type: 'abortScenario' }));
$('#export-json').addEventListener('click', exportEvidence);
$('#export-wav').addEventListener('click', exportWav);
$('#export-plots').addEventListener('click', exportPlots);

loadContracts().catch((error) => setStatus(String(error), 'error'));
