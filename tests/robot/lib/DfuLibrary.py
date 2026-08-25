"""Robot keywords for the firmware update path.

Separate from NeonSamuraiLibrary because it works on a different transport:
the device is not a MIDI device while any of this is happening, it is a USB
DFU device with no MIDI interface at all. The two libraries hand off to each
other around the reboot.

dfu-programmer is used as the reference implementation rather than the browser
port. What is being tested here is the device-side contract - that the sysex
command really does reach the bootloader, that the bootloader really does
accept an image, and that the device really does come back running it. The
wire format of the browser port is covered by tools/dfu/xmega-dfu.test.js,
which can assert on bytes in a way a hardware test cannot.
"""

from __future__ import annotations

import subprocess
import time

from robot.api import logger
from robot.api.deco import keyword, library

# The bootloader's identity on the USB bus.
DFU_VENDOR_ID = "03eb"
DFU_PRODUCT_ID = "2fde"
DFU_PART = "atxmega128a4u"

# Measured rather than guessed. On this part the bootloader enumerates about
# two seconds after the sysex command, and an erase plus a 19 KB write takes
# a few seconds. These leave several times that as headroom, which is enough
# to absorb a loaded machine without making a failure take a minute to report.
DEFAULT_APPEAR_TIMEOUT_S = 10.0
DEFAULT_FLASH_TIMEOUT_S = 60.0
DEFAULT_LAUNCH_TIMEOUT_S = 15.0

# Polling interval while waiting for the bus to settle. Short, because the
# wait is nearly always over almost immediately and a long interval is just
# dead time on every single call.
POLL_INTERVAL_S = 0.2


@library(scope="SUITE")
class DfuLibrary:
    """Driving the device through a firmware update."""

    @keyword("Dfu Device Is Present")
    def dfu_device_is_present(self) -> bool:
        """Whether the bootloader is on the bus right now."""
        try:
            out = subprocess.run(
                ["lsusb"], capture_output=True, text=True, timeout=10
            ).stdout
        except (OSError, subprocess.SubprocessError):
            return False

        return f"{DFU_VENDOR_ID}:{DFU_PRODUCT_ID}" in out

    @keyword("Wait For Dfu Device")
    def wait_for_dfu_device(self, timeout_s: float = DEFAULT_APPEAR_TIMEOUT_S) -> None:
        """Block until the bootloader enumerates."""
        deadline = time.monotonic() + timeout_s

        while time.monotonic() < deadline:
            if self.dfu_device_is_present():
                logger.info("DFU device present")
                return
            time.sleep(POLL_INTERVAL_S)

        raise AssertionError(
            f"No DFU device ({DFU_VENDOR_ID}:{DFU_PRODUCT_ID}) within {timeout_s}s. "
            "Either the bootloader command did not reach the device, or the boot "
            "section is empty - see the bootloader recovery page."
        )

    def _dfu(self, *args: str, timeout_s: float) -> str:
        cmd = ["dfu-programmer", DFU_PART, *args]
        logger.info(f"running: {' '.join(cmd)}")

        try:
            done = subprocess.run(
                cmd, capture_output=True, text=True, timeout=timeout_s
            )
        except FileNotFoundError as exc:
            raise AssertionError(
                "dfu-programmer is not installed - it is what these tests flash with"
            ) from exc
        except subprocess.TimeoutExpired as exc:
            raise AssertionError(f"{' '.join(cmd)} did not finish in {timeout_s}s") from exc

        output = (done.stdout or "") + (done.stderr or "")

        if done.returncode != 0:
            # The permissions case is worth naming, because "no device present"
            # while lsusb shows the device is otherwise baffling.
            if "no device present" in output.lower() and self.dfu_device_is_present():
                raise AssertionError(
                    "dfu-programmer cannot reach the device although it is on the "
                    "bus - install scripts/99-neon-samurai.rules"
                )
            raise AssertionError(f"{' '.join(cmd)} failed:\n{output}")

        return output

    @keyword("Erase Device Flash")
    def erase_device_flash(self, timeout_s: float = DEFAULT_FLASH_TIMEOUT_S) -> None:
        """Chip erase.

        Not optional before writing: parts ship with security bits set, and
        the bootloader accepts nothing else until an erase has run.
        """
        self._dfu("erase", timeout_s=timeout_s)

    @keyword("Flash Firmware")
    def flash_firmware(self, path: str, timeout_s: float = DEFAULT_FLASH_TIMEOUT_S) -> None:
        """Write a .hex, and fail if the device's own validation does not pass."""
        output = self._dfu("flash", path, timeout_s=timeout_s)

        if "Validating" in output and "Success" not in output:
            raise AssertionError(f"the device did not validate what was written:\n{output}")

        logger.info(output.strip())

    @keyword("Launch Application")
    def launch_application(self, timeout_s: float = DEFAULT_LAUNCH_TIMEOUT_S) -> None:
        """Leave the bootloader and start the application."""
        self._dfu("launch", timeout_s=timeout_s)
