import createResonantLabModule from './resonant-lab.js';

class ResonantLabProcessor extends AudioWorkletProcessor {
  constructor() {
    super();
    this.module = null;
    this.ready = false;
    this.pending = [];
    this.telemetryCountdown = 0;
    this.hardFailureReported = false;
    this.scenario = null;

    this.port.onmessage = (event) => {
      if (!this.ready) {
        this.pending.push(event.data);
        return;
      }
      this.applyMessage(event.data);
    };

    createResonantLabModule({ noInitialRun: true })
      .then((module) => {
        this.module = module;
        if (!module._re_prepare(sampleRate)) {
          throw new Error('Resonant Engine Lab failed to prepare');
        }
        this.ready = true;
        const parameters = [];
        const count = module._re_parameter_count();
        for (let index = 0; index < count; index += 1) {
          parameters.push({
            id: module._re_parameter_id(index),
            min: module._re_parameter_min(index),
            max: module._re_parameter_max(index),
            defaultValue: module._re_parameter_default(index),
          });
        }
        for (const message of this.pending) this.applyMessage(message);
        this.pending.length = 0;
        this.port.postMessage({ type: 'ready', sampleRate, parameters });
      })
      .catch((error) => {
        this.port.postMessage({ type: 'error', message: String(error) });
      });
  }

  applyMessage(message) {
    const m = this.module;
    switch (message.type) {
      case 'param':
        m._re_set_parameter(Number(message.id), Number(message.value));
        break;
      case 'noteOn':
        m._re_note_on(Number(message.note), Number(message.velocity));
        break;
      case 'noteOff':
        m._re_note_off(Number(message.note));
        break;
      case 'reset':
        m._re_reset();
        this.scenario = null;
        this.hardFailureReported = false;
        break;
      case 'panic':
        m._re_panic();
        break;
      case 'scenario':
        this.startScenario(message);
        break;
      case 'abortScenario':
        this.scenario = null;
        m._re_panic();
        this.port.postMessage({ type: 'scenarioAborted' });
        break;
      default:
        break;
    }
  }

  startScenario(message) {
    this.module._re_reset();
    this.hardFailureReported = false;
    this.scenario = {
      id: String(message.id),
      frame: 0,
      nextAction: 0,
      durationFrames: Math.max(1, Math.round(Number(message.durationMs) * sampleRate / 1000)),
      actions: Array.isArray(message.actions)
        ? [...message.actions].sort((a, b) => a.atFrame - b.atFrame)
        : [],
    };
    this.port.postMessage({ type: 'scenarioStarted', id: this.scenario.id });
  }

  applyScenarioAction(action) {
    const m = this.module;
    switch (action.type) {
      case 'param':
        m._re_set_parameter(Number(action.id), Number(action.value));
        break;
      case 'noteOn':
        m._re_note_on(Number(action.note), Number(action.velocity));
        break;
      case 'noteOff':
        m._re_note_off(Number(action.note));
        break;
      case 'panic':
        m._re_panic();
        break;
      case 'reset':
        m._re_reset();
        break;
      case 'mark':
        this.port.postMessage({ type: 'scenarioMark', id: this.scenario?.id, data: action.data ?? {} });
        break;
      default:
        break;
    }
  }

  runScenarioActions() {
    if (!this.scenario) return;
    while (this.scenario.nextAction < this.scenario.actions.length) {
      const action = this.scenario.actions[this.scenario.nextAction];
      if (action.atFrame > this.scenario.frame) break;
      this.applyScenarioAction(action);
      this.scenario.nextAction += 1;
    }
  }

  telemetry() {
    const m = this.module;
    return {
      type: 'telemetry',
      resonatorEnergy: m._re_resonator_energy(),
      coreRms: m._re_core_output_rms(),
      corePeak: m._re_core_peak(),
      stability: m._re_stability_state(),
      activeVoices: m._re_active_voices(),
      heldVoices: m._re_held_voices(),
      maxActiveVoices: m._re_max_active_voices(),
      maximumPolyphony: m._re_maximum_polyphony(),
      voiceSteals: m._re_voice_steals(),
      protectedState: Boolean(m._re_protected_state()),
      cpuLoad: m._re_cpu_load(),
      cpuLoadSmoothed: m._re_cpu_load_smoothed(),
      cpuLoadMax: m._re_cpu_load_max(),
      scenarioId: this.scenario?.id ?? null,
      scenarioFrame: this.scenario?.frame ?? null,
    };
  }

  process(_inputs, outputs) {
    const output = outputs[0];
    if (!output || output.length === 0) return true;
    const frames = output[0].length;

    if (!this.ready) {
      for (const channel of output) channel.fill(0);
      return true;
    }

    this.runScenarioActions();
    const ok = this.module._re_process(frames);
    const ptr = this.module._re_output_ptr() >>> 2;
    const mono = this.module.HEAPF32.subarray(ptr, ptr + frames);
    for (const channel of output) channel.set(mono);

    if (!ok && !this.hardFailureReported) {
      this.hardFailureReported = true;
      this.port.postMessage({ type: 'hardFailure', telemetry: this.telemetry() });
    }

    if (this.scenario) {
      this.scenario.frame += frames;
      if (this.scenario.frame >= this.scenario.durationFrames) {
        const completed = this.scenario.id;
        this.scenario = null;
        this.module._re_panic();
        this.port.postMessage({ type: 'scenarioComplete', id: completed, telemetry: this.telemetry() });
      }
    }

    this.telemetryCountdown -= 1;
    if (this.telemetryCountdown <= 0) {
      this.telemetryCountdown = 12;
      this.port.postMessage(this.telemetry());
    }
    return true;
  }
}

registerProcessor('resonant-lab', ResonantLabProcessor);
