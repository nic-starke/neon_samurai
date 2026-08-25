// The firmware update dialog: the warning, the step list, and the outcome.
//
// Built as one component because the three are the same surface at different
// moments - the user is not meant to experience them as separate screens, and
// keeping the state in one place is what stops the dialog being dismissable
// half way through a flash.

import { STEP_ORDER, STEP_LABELS } from "../../js/firmware-update.js";

const el = (tag, props = {}, children = []) => {
	const node = document.createElement(tag);
	Object.assign(node, props);
	for (const c of children) node.append(c);
	return node;
};

export class UpdateDialog {
	/**
	 * @param options.version   The version being installed.
	 * @param options.onConfirm Called when the user commits to the update.
	 * @param options.onClose   Called when the dialog is dismissed.
	 */
	constructor({ version, onConfirm, onClose }) {
		this.version = version;
		this.onConfirm = onConfirm;
		this.onClose = onClose;
		this.steps = new Map();
		this.running = false;

		this._build();
	}

	_build() {
		this.title = el("h2", { className: "fw-dialog__title", textContent: "Update firmware" });
		this.body = el("div", { className: "fw-dialog__body" });

		this.cancel = el("button", { type: "button", className: "fw-btn", textContent: "Cancel" });
		this.confirm = el("button", {
			type: "button",
			className: "fw-btn fw-btn--primary",
			textContent: "Update firmware",
		});

		this.cancel.addEventListener("click", () => this.close());
		this.confirm.addEventListener("click", () => {
			this.showProgress();
			this.onConfirm?.();
		});

		this.actions = el("div", { className: "fw-dialog__actions" }, [this.cancel, this.confirm]);

		this.panel = el("div", { className: "fw-dialog", role: "dialog" }, [
			this.title, this.body, this.actions,
		]);
		this.panel.setAttribute("aria-modal", "true");

		this.backdrop = el("div", { className: "fw-backdrop" }, [this.panel]);

		// Only dismissable while nothing is being written. Closing the dialog
		// mid-flash would hide a device that is sitting in the bootloader with
		// no firmware on it.
		this.backdrop.addEventListener("click", (e) => {
			if (e.target === this.backdrop && !this.running) this.close();
		});

		this._onKey = (e) => {
			if (e.key === "Escape" && !this.running) this.close();
		};

		this.showWarning();
	}

	showWarning() {
		this.body.replaceChildren(
			el("p", {
				textContent:
					`Version ${this.version} will be written to the device. This replaces ` +
					`the firmware currently on it.`,
			}),
			el("ul", { className: "fw-warnings" }, [
				el("li", { textContent: "Do not unplug the device while the update is running." }),
				el("li", { textContent: "Close any software using the device first - it will disconnect." }),
				el("li", {
					textContent:
						"If the update fails, the device stays in its bootloader and can simply " +
						"be updated again. The bootloader itself is never written to.",
				}),
			]),
			el("p", { className: "fw-dim", textContent: "Your settings are kept." }),
		);
	}

	showProgress() {
		this.running = true;
		this.title.textContent = "Updating firmware";
		this.cancel.disabled = true;
		this.confirm.disabled = true;
		this.confirm.textContent = "Updating…";

		this.list = el("ol", { className: "fw-steps" });
		this.steps.clear();

		for (const step of STEP_ORDER) {
			const label = el("span", { className: "fw-step__label", textContent: STEP_LABELS[step] });
			const detail = el("span", { className: "fw-step__detail" });
			const item = el("li", { className: "fw-step" }, [label, detail]);
			this.steps.set(step, { item, detail });
			this.list.append(item);
		}

		this.bar = el("progress", { className: "fw-progress", value: 0, max: 1 });
		this.body.replaceChildren(this.list, this.bar);
	}

	/** @param state "active" | "done" | "failed" */
	setStep(step, state, detail = "") {
		const entry = this.steps.get(step);
		if (!entry) return;

		entry.item.className = `fw-step fw-step--${state}`;
		entry.detail.textContent = detail ?? "";

		// Keep the running step in view on a short window.
		if (state === "active") entry.item.scrollIntoView({ block: "nearest" });
	}

	setProgress(done, total) {
		if (this.bar) this.bar.value = total ? done / total : 0;
	}

	showComplete(version) {
		this.running = false;
		this.title.textContent = "Firmware update complete";
		this.body.replaceChildren(
			el("p", { textContent: `The device is running version ${version}.` }),
			el("p", { className: "fw-dim", textContent: "It has reconnected and is ready to use." }),
		);

		this.actions.replaceChildren(
			el("button", {
				type: "button",
				className: "fw-btn fw-btn--primary",
				textContent: "Back to the editor",
				onclick: () => this.close(),
			}),
		);
	}

	showFailure(message) {
		this.running = false;
		this.title.textContent = "Firmware update failed";

		this.body.append(
			el("p", { className: "fw-error", textContent: message }),
			el("p", {
				className: "fw-dim",
				textContent:
					"The bootloader is untouched, so the device can be updated again. If it is " +
					"no longer listed, unplug it and plug it back in.",
			}),
		);

		this.actions.replaceChildren(
			el("button", {
				type: "button",
				className: "fw-btn",
				textContent: "Close",
				onclick: () => this.close(),
			}),
		);
	}

	/** Ask the user to pick the bootloader, which needs a real click. */
	requestDevicePrompt(onPick) {
		return new Promise((resolve, reject) => {
			const button = el("button", {
				type: "button",
				className: "fw-btn fw-btn--primary fw-btn--waiting",
				textContent: "Select the device",
			});

			button.addEventListener("click", async () => {
				button.disabled = true;
				button.classList.remove("fw-btn--waiting");
				try {
					resolve(await onPick());
				} catch (e) {
					reject(e);
				}
			});

			this.body.append(
				el("p", {
					className: "fw-prompt",
					textContent:
						"Choose the NEON_SAMURAI bootloader in the window your browser opens. " +
						"This is only needed the first time.",
				}),
				button,
			);
			button.focus();
		});
	}

	open() {
		document.body.append(this.backdrop);
		document.addEventListener("keydown", this._onKey);
		this.confirm.focus();
	}

	close() {
		document.removeEventListener("keydown", this._onKey);
		this.backdrop.remove();
		this.onClose?.();
	}
}
