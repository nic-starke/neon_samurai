// Reproduces the firmware's bank-change RGB fade (see
// draw_bank_change_animation() in src/animation/animation.c): over 250ms the
// target encoder's RGB LED runs off -> white -> off -> its own colour, in
// three equal segments.
//
// Bank 0/1/2/3 map to firmware encoder index 3/2/1/0 (animation_start_bank_
// change()) - neither vmap indexing nor visual grid position; live-twin.js
// maps firmware index to visual position itself.
//
// The old implementation strobed white/off at ~16Hz, inside the range
// associated with photosensitive seizures. A single 250ms sweep is not a
// strobe, but reduced-motion is still honoured: the bank changes with no
// transition at all.

const DURATION_MS = 250;

function prefersReducedMotion() {
  return (
    typeof matchMedia === "function" &&
    matchMedia("(prefers-reduced-motion: reduce)").matches
  );
}

function targetEncoderForBank(bank) {
  // Mirrors the firmware's `3 - new_bank`.
  return bank >= 0 && bank < 4 ? 3 - bank : 3;
}

export class BankFade {
  constructor() {
    this._targetEncoder = -1;
    this._startedAt = null;
    this._raf = null;
    this.onFrame = null;
  }

  start(newBank) {
    this.stop();
    if (prefersReducedMotion()) {
      this.onFrame?.();
      return;
    }
    this._targetEncoder = targetEncoderForBank(newBank);
    this._startedAt = performance.now();

    const step = () => {
      if (this._startedAt === null) return;
      if (performance.now() - this._startedAt >= DURATION_MS) {
        this.stop();
        this.onFrame?.();
        return;
      }
      this.onFrame?.();
      this._raf = requestAnimationFrame(step);
    };
    this._raf = requestAnimationFrame(step);
    this.onFrame?.();
  }

  stop() {
    if (this._raf !== null) cancelAnimationFrame(this._raf);
    this._raf = null;
    this._startedAt = null;
    this._targetEncoder = -1;
  }

  isFading(firmwareEncoderIndex) {
    return (
      this._startedAt !== null && firmwareEncoderIndex === this._targetEncoder
    );
  }

  // Mirrors draw_bank_change_animation()'s three segments. Returns the RGB to
  // show mid-fade, or null when this encoder is not fading.
  sample(firmwareEncoderIndex, own) {
    if (!this.isFading(firmwareEncoderIndex)) return null;

    const t = (performance.now() - this._startedAt) / DURATION_MS;
    if (t <= 0 || t >= 1) return null;

    const segment = 1 / 3;

    if (t < segment) {
      const w = Math.round((t / segment) * 255);
      return { r: w, g: w, b: w };
    }

    if (t < 2 * segment) {
      const w = Math.round((1 - (t - segment) / segment) * 255);
      return { r: w, g: w, b: w };
    }

    const w = (t - 2 * segment) / segment;
    return {
      r: Math.round(own.r * w),
      g: Math.round(own.g * w),
      b: Math.round(own.b * w),
    };
  }
}
