(() => {
  const NativeAudioWorkletNode = window.AudioWorkletNode;
  if (!NativeAudioWorkletNode) return;

  window.AudioWorkletNode = class ResonantAudioWorkletNode extends NativeAudioWorkletNode {
    constructor(context, name, options = {}) {
      const effective = name === 'resonant-lab' && options.numberOfInputs === 0
        ? { ...options, numberOfInputs: 1 }
        : options;
      super(context, name, effective);

      if (name === 'resonant-lab') {
        // Keep the generator explicitly pullable even on audio backends that are
        // conservative about zero-input worklets.
        if (effective.numberOfInputs === 1) {
          const keepAlive = context.createConstantSource();
          keepAlive.offset.value = 1.0;
          keepAlive.connect(this);
          keepAlive.start();
          this.__resonantKeepAlive = keepAlive;
        }

        // Give the Lab a direct destination path independent of the analyser and
        // capture worklet chain used by the UI. This isolates realtime scheduling
        // from optional measurement/capture infrastructure.
        const directMonitor = context.createGain();
        directMonitor.gain.value = 0.000001;
        this.connect(directMonitor).connect(context.destination);
        this.__resonantDirectMonitor = directMonitor;

        this.addEventListener('processorerror', () => {
          const status = document.querySelector('#status');
          if (status) {
            status.textContent = 'AudioWorklet processor error';
            status.dataset.kind = 'error';
          }
        });
      }
    }
  };
})();
