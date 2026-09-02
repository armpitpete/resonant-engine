(() => {
  const NativeAudioWorkletNode = window.AudioWorkletNode;
  if (!NativeAudioWorkletNode) return;

  window.AudioWorkletNode = class ResonantAudioWorkletNode extends NativeAudioWorkletNode {
    constructor(context, name, options = {}) {
      const effective = name === 'resonant-lab' && options.numberOfInputs === 0
        ? { ...options, numberOfInputs: 1 }
        : options;
      super(context, name, effective);

      if (name === 'resonant-lab' && effective.numberOfInputs === 1) {
        const keepAlive = context.createConstantSource();
        keepAlive.offset.value = 1.0;
        keepAlive.connect(this);
        keepAlive.start();
        this.__resonantKeepAlive = keepAlive;
      }
    }
  };
})();
