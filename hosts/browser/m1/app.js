const state = {
  context: null,
  node: null,
  gain: null,
  ready: false,
  midi: null,
  activeNotes: [],
  baseMidi: 60,
};

const statusEl = document.querySelector('#status');
const startButton = document.querySelector('#start');
const resetButton = document.querySelector('#reset');
const triggerButton = document.querySelector('#trigger');
const midiButton = document.querySelector('#midi');
const noteEl = document.querySelector('#note');

const keyMap = new Map([
  ['a', 0], ['w', 1], ['s', 2], ['e', 3], ['d', 4], ['f', 5],
  ['t', 6], ['g', 7], ['y', 8], ['h', 9], ['u', 10], ['j', 11], ['k', 12],
]);
const pressedKeys = new Set();

function setStatus(text, kind = '') {
  statusEl.textContent = text;
  statusEl.dataset.kind = kind;
}

function midiToHz(note) {
  return 440 * Math.pow(2, (note - 69) / 12);
}

function noteName(note) {
  const names = ['C', 'C♯', 'D', 'D♯', 'E', 'F', 'F♯', 'G', 'G♯', 'A', 'A♯', 'B'];
  return `${names[note % 12]}${Math.floor(note / 12) - 1}`;
}

function post(message) {
  if (state.node) state.node.port.postMessage(message);
}

function sendAllControls() {
  for (const input of document.querySelectorAll('[data-param]')) {
    post({ type: 'param', name: input.dataset.param, value: Number(input.value) });
  }
  if (state.gain) {
    state.gain.gain.setValueAtTime(Number(document.querySelector('#master').value), state.context.currentTime);
  }
}

async function startAudio() {
  if (state.context) {
    await state.context.resume();
    return;
  }

  startButton.disabled = true;
  setStatus('Starting audio…');
  try {
    const context = new AudioContext({ latencyHint: 'interactive' });
    await context.audioWorklet.addModule('./worklet.js');
    const node = new AudioWorkletNode(context, 'resonant-m1', {
      numberOfInputs: 0,
      numberOfOutputs: 1,
      outputChannelCount: [2],
    });
    const gain = context.createGain();
    gain.gain.value = Number(document.querySelector('#master').value);
    node.connect(gain).connect(context.destination);

    state.context = context;
    state.node = node;
    state.gain = gain;

    node.port.onmessage = (event) => {
      if (event.data.type === 'ready') {
        state.ready = true;
        sendAllControls();
        setStatus(`Live — ${Math.round(event.data.sampleRate)} Hz`, 'ok');
        startButton.textContent = 'Audio running';
      } else if (event.data.type === 'error') {
        setStatus(event.data.message, 'error');
      }
    };

    await context.resume();
  } catch (error) {
    setStatus(String(error), 'error');
    startButton.disabled = false;
  }
}

function playNote(note, velocity = 0.8) {
  if (!state.ready) return;
  const hz = midiToHz(note);
  post({ type: 'noteOn', hz, velocity });
  noteEl.textContent = `${noteName(note)} — ${hz.toFixed(1)} Hz`;
}

function handleMidiMessage(event) {
  const [status, data1 = 0, data2 = 0] = event.data;
  const command = status & 0xf0;

  if (command === 0x90 && data2 > 0) {
    state.activeNotes = state.activeNotes.filter((note) => note !== data1);
    state.activeNotes.push(data1);
    playNote(data1, data2 / 127);
    return;
  }

  if (command === 0x80 || (command === 0x90 && data2 === 0)) {
    state.activeNotes = state.activeNotes.filter((note) => note !== data1);
    if (state.activeNotes.length > 0) playNote(state.activeNotes.at(-1), 0.01);
    return;
  }

  if (command === 0xb0 && data1 === 1) {
    const value = data2 / 127;
    const excitation = document.querySelector('[data-param="excitation"]');
    excitation.value = String(value);
    excitation.dispatchEvent(new Event('input'));
    return;
  }

  if (command === 0xd0) {
    const value = data1 / 127;
    const excitation = document.querySelector('[data-param="excitation"]');
    excitation.value = String(value);
    excitation.dispatchEvent(new Event('input'));
  }
}

async function enableMidi() {
  if (!navigator.requestMIDIAccess) {
    setStatus('Web MIDI is not available in this browser', 'error');
    return;
  }
  try {
    state.midi = await navigator.requestMIDIAccess();
    for (const input of state.midi.inputs.values()) input.onmidimessage = handleMidiMessage;
    state.midi.onstatechange = () => {
      for (const input of state.midi.inputs.values()) input.onmidimessage = handleMidiMessage;
    };
    midiButton.textContent = 'MIDI enabled';
    setStatus('MIDI ready', 'ok');
  } catch (error) {
    setStatus(`MIDI: ${error}`, 'error');
  }
}

startButton.addEventListener('click', startAudio);
midiButton.addEventListener('click', enableMidi);
resetButton.addEventListener('click', () => {
  post({ type: 'reset' });
  sendAllControls();
});
triggerButton.addEventListener('click', () => post({ type: 'trigger', amount: 0.85 }));

document.querySelector('#octave-down').addEventListener('click', () => {
  state.baseMidi = Math.max(24, state.baseMidi - 12);
  document.querySelector('#octave').textContent = String(Math.floor(state.baseMidi / 12) - 1);
});
document.querySelector('#octave-up').addEventListener('click', () => {
  state.baseMidi = Math.min(96, state.baseMidi + 12);
  document.querySelector('#octave').textContent = String(Math.floor(state.baseMidi / 12) - 1);
});

for (const input of document.querySelectorAll('[data-param]')) {
  const valueEl = document.querySelector(`[data-value-for="${input.id}"]`);
  const update = () => {
    const value = Number(input.value);
    if (valueEl) valueEl.textContent = value.toFixed(2);
    post({ type: 'param', name: input.dataset.param, value });
  };
  input.addEventListener('input', update);
  update();
}

const master = document.querySelector('#master');
master.addEventListener('input', () => {
  document.querySelector('[data-value-for="master"]').textContent = Number(master.value).toFixed(2);
  if (state.gain) state.gain.gain.setTargetAtTime(Number(master.value), state.context.currentTime, 0.01);
});

document.addEventListener('keydown', (event) => {
  if (event.repeat) return;
  const key = event.key.toLowerCase();
  if (key === 'z') {
    document.querySelector('#octave-down').click();
    return;
  }
  if (key === 'x') {
    document.querySelector('#octave-up').click();
    return;
  }
  if (!keyMap.has(key)) return;
  event.preventDefault();
  pressedKeys.add(key);
  playNote(state.baseMidi + keyMap.get(key));
});

document.addEventListener('keyup', (event) => {
  pressedKeys.delete(event.key.toLowerCase());
});
