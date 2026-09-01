class ResonantCaptureProcessor extends AudioWorkletProcessor {
  constructor() {
    super();
    this.recording = false;
    this.port.onmessage = (event) => {
      if (event.data?.type === 'start') this.recording = true;
      if (event.data?.type === 'stop') this.recording = false;
    };
  }

  process(inputs, outputs) {
    const input = inputs[0];
    const output = outputs[0];
    if (!output || output.length === 0) return true;
    const source = input && input.length > 0 ? input[0] : null;
    for (const channel of output) {
      if (source) channel.set(source);
      else channel.fill(0);
    }
    if (this.recording && source) {
      const copy = new Float32Array(source.length);
      copy.set(source);
      this.port.postMessage({ type: 'audio', samples: copy }, [copy.buffer]);
    }
    return true;
  }
}

registerProcessor('resonant-capture', ResonantCaptureProcessor);
