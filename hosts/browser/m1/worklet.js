import createResonantModule from './resonant-m1.js';

const PARAM = {
  damping: 102,
  feedback: 103,
  nonlinearity: 104,
  excitation: 105,
  turbulence: 106,
  interaction: 107,
};

class ResonantM1Processor extends AudioWorkletProcessor {
  constructor() {
    super();
    this.module = null;
    this.ready = false;
    this.pending = [];

    this.port.onmessage = (event) => {
      if (!this.ready) {
        this.pending.push(event.data);
        return;
      }
      this.applyMessage(event.data);
    };

    createResonantModule({ noInitialRun: true })
      .then((module) => {
        this.module = module;
        if (!module._re_prepare(sampleRate)) {
          throw new Error('Resonant Engine failed to prepare');
        }
        this.ready = true;
        for (const message of this.pending) this.applyMessage(message);
        this.pending.length = 0;
        this.port.postMessage({ type: 'ready', sampleRate });
      })
      .catch((error) => {
        this.port.postMessage({ type: 'error', message: String(error) });
      });
  }

  applyMessage(message) {
    const m = this.module;
    switch (message.type) {
      case 'param':
        if (Object.hasOwn(PARAM, message.name)) {
          m._re_set_parameter(PARAM[message.name], Number(message.value));
        }
        break;
      case 'pitch':
        m._re_set_pitch(Number(message.hz));
        break;
      case 'noteOn':
        m._re_note_on(Number(message.hz), Number(message.velocity));
        break;
      case 'trigger':
        m._re_trigger(Number(message.amount));
        break;
      case 'reset':
        m._re_reset();
        break;
      default:
        break;
    }
  }

  process(_inputs, outputs) {
    const output = outputs[0];
    if (!output || output.length === 0) return true;

    if (!this.ready) {
      for (const channel of output) channel.fill(0);
      return true;
    }

    const frames = output[0].length;
    if (!this.module._re_process(frames)) {
      for (const channel of output) channel.fill(0);
      this.port.postMessage({ type: 'error', message: 'DSP processing failed' });
      return true;
    }

    const ptr = this.module._re_output_ptr() >>> 2;
    const mono = this.module.HEAPF32.subarray(ptr, ptr + frames);
    for (const channel of output) channel.set(mono);
    return true;
  }
}

registerProcessor('resonant-m1', ResonantM1Processor);
