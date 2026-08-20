// Reproduces the firmware's bank-change RGB flicker (see
// draw_bank_change_animation() in src/animation/animation.c): the target
// encoder's RGB LED toggles white/off every ~31ms for 250ms.
//
// Bank 0/1/2 map to firmware encoder index 3/2/1 (animation_start_bank_
// change()) - neither vmap indexing nor visual grid position; live-twin.js
// maps firmware index to visual position itself.
//
// That cadence is a ~16Hz full-white flash, which is inside the range
// associated with photosensitive seizures, so it is skipped entirely when
// the user has asked for reduced motion. The bank still changes; only the
// strobe is suppressed.

const DURATION_MS = 250;
const TOTAL_FRAMES = 8;
const FRAME_MS = DURATION_MS / TOTAL_FRAMES;

function prefersReducedMotion() {
	return typeof matchMedia === "function" && matchMedia("(prefers-reduced-motion: reduce)").matches;
}

function targetEncoderForBank(bank) {
	switch (bank) {
		case 0:
			return 3;
		case 1:
			return 2;
		case 2:
			return 1;
		default:
			return 3;
	}
}

export class BankFlicker {
	constructor() {
		this._targetEncoder = -1;
		this._frame = 0;
		this._timer = null;
		this.onFrame = null;
	}

	start(newBank) {
		this.stop();
		if (prefersReducedMotion()) {
			this.onFrame?.();
			return;
		}
		this._targetEncoder = targetEncoderForBank(newBank);
		this._frame = 0;
		this._timer = setInterval(() => {
			this._frame++;
			if (this._frame >= TOTAL_FRAMES) {
				this.stop();
			}
			this.onFrame?.();
		}, FRAME_MS);
		this.onFrame?.();
	}

	stop() {
		if (this._timer) clearInterval(this._timer);
		this._timer = null;
		this._targetEncoder = -1;
	}

	// White on even frames, off on odd - matches rgb_on = (current_frame % 2
	// == 0) in draw_bank_change_animation().
	isWhite(firmwareEncoderIndex) {
		return this._timer !== null && firmwareEncoderIndex === this._targetEncoder && this._frame % 2 === 0;
	}

	isFlickering(firmwareEncoderIndex) {
		return this._timer !== null && firmwareEncoderIndex === this._targetEncoder;
	}
}
