#!/usr/bin/env python3
"""Arrow-key teleop. Runs on the Raspberry Pi, driven from the laptop over SSH:

    ./run_teleop.sh            (on the laptop, see run_teleop.sh)

The Pi sends text lines like  set_wheel(0, 0.6, 1)  over UART to the STM32
(STM32_Wheel_Teleop firmware). The STM32 stops all wheels by itself if no command
arrives for 0.5 s, so the car stops when this script dies or the link breaks.

Keys (hold an arrow to keep moving, a terminal has no key-up, so the car stops
a moment after the key repeat stops):
    Up / Down      forward / backward
    Left / Right   turn left / right in place
    Space or x     stop
    + / -          speed up / down (duty, step 0.05)
    q              quit
"""

import argparse
import os
import select
import sys
import termios
import time
import tty

# ---------------- configuration ----------------
SERIAL_PORT = "/dev/ttyAMA0"
BAUD = 115200
DUTY = 0.6           # start duty 0..1
DUTY_STEP = 0.05
DUTY_MIN, DUTY_MAX = 0.2, 1.0
SEND_HZ = 20
# Terminal key-repeat has a first delay (~0.5 s). Stop when no key for this long.
KEY_IDLE_S = 0.7

# Direction bit that makes each wheel roll "forward" (0 forward / 1 backward for set_wheel).
# From the bench test: wheels 0 and 2 are mounted mirrored to wheels 1 and 3.
FORWARD_DIR = (1, 0, 1, 0)
# Which wheels are on the left side of the car (used for turning in place).
LEFT_WHEELS = (0, 2)
# ------------------------------------------------

# motion -> (left side sign, right side sign); +1 = forward, -1 = backward
MOTIONS = {
    "up": (1, 1),
    "down": (-1, -1),
    "left": (-1, 1),
    "right": (1, -1),
}


def wheel_commands(motion, duty, forward_dir=FORWARD_DIR, left_wheels=LEFT_WHEELS):
    """Return the 4 set_wheel lines for a motion name, or the stop lines for None."""
    lines = []
    for w in range(4):
        if motion is None:
            lines.append(f"set_wheel({w}, 0, 0)")
            continue
        left_sign, right_sign = MOTIONS[motion]
        sign = left_sign if w in left_wheels else right_sign
        direction = forward_dir[w] if sign > 0 else 1 - forward_dir[w]
        lines.append(f"set_wheel({w}, {duty:.2f}, {direction})")
    return lines


ARROWS = {"A": "up", "B": "down", "C": "right", "D": "left"}


def read_keys(fd):
    """Read all pending bytes from stdin and return a list of key names."""
    keys = []
    data = os.read(fd, 64)
    i = 0
    while i < len(data):
        b = data[i:i + 1]
        if b == b"\x1b" and i + 2 < len(data) + 0 and data[i + 1:i + 2] in (b"[", b"O"):
            keys.append(ARROWS.get(chr(data[i + 2]), "?"))
            i += 3
            continue
        if b == b"\x03":
            raise KeyboardInterrupt
        keys.append(b.decode("ascii", errors="ignore").lower())
        i += 1
    return keys


class Link:
    def __init__(self, port, baud, dry_run):
        self.dry_run = dry_run
        self.errors = 0
        self.last_reply = ""
        if dry_run:
            self.ser = None
            return
        import serial  # pyserial
        self.ser = serial.Serial(port, baud, timeout=0)

    def send(self, lines):
        payload = ("\n".join(lines) + "\n").encode("ascii")
        if self.dry_run:
            return
        self.ser.write(payload)
        self.poll_replies()

    def poll_replies(self):
        if self.dry_run:
            return
        data = self.ser.read(512)
        if data:
            for line in data.decode("ascii", errors="replace").splitlines():
                if line.startswith("ERR"):
                    self.errors += 1
                if line.strip():
                    self.last_reply = line.strip()

    def close(self):
        if self.ser:
            self.ser.close()


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--port", default=SERIAL_PORT)
    ap.add_argument("--duty", type=float, default=DUTY)
    ap.add_argument("--invert", action="store_true", help="flip forward/backward for all wheels")
    ap.add_argument("--dry-run", action="store_true", help="do not open the UART, only show what would be sent")
    args = ap.parse_args()

    if not sys.stdin.isatty():
        sys.exit("Needs a terminal. From the laptop use ./run_teleop.sh (ssh -t).")

    forward_dir = tuple(1 - d for d in FORWARD_DIR) if args.invert else FORWARD_DIR
    duty = min(DUTY_MAX, max(DUTY_MIN, args.duty))

    try:
        link = Link(args.port, BAUD, args.dry_run)
    except Exception as exc:  # noqa: BLE001
        sys.exit(f"Cannot open {args.port}: {exc}")

    fd = sys.stdin.fileno()
    old = termios.tcgetattr(fd)
    motion = None
    last_key_t = 0.0
    period = 1.0 / SEND_HZ
    next_send = 0.0
    stop_burst = 0

    def status():
        name = motion or "stop"
        extra = " [dry-run]" if args.dry_run else f" last: {link.last_reply or '-'}  errors: {link.errors}"
        sys.stdout.write(f"\r\x1b[K{name:5s} duty={duty:.2f}{extra}")
        sys.stdout.flush()

    try:
        tty.setraw(fd, when=termios.TCSANOW)
        sys.stdout.write("Arrows = drive, space = stop, +/- = speed, q = quit\r\n")
        link.send(["stop"])
        while True:
            now = time.monotonic()
            ready, _, _ = select.select([fd], [], [], max(0.0, next_send - now))
            if ready:
                for key in read_keys(fd):
                    if key == "q":
                        return
                    if key in MOTIONS:
                        if key != motion:
                            next_send = 0.0  # send the new direction right away
                        motion = key
                        last_key_t = time.monotonic()
                    elif key in (" ", "x"):
                        motion = None
                        stop_burst = 3
                    elif key in ("+", "="):
                        duty = min(DUTY_MAX, duty + DUTY_STEP)
                    elif key in ("-", "_"):
                        duty = max(DUTY_MIN, duty - DUTY_STEP)
            now = time.monotonic()
            if motion and now - last_key_t > KEY_IDLE_S:
                motion = None
                stop_burst = 3
            if now >= next_send:
                next_send = now + period
                if motion:
                    link.send(wheel_commands(motion, duty, forward_dir))
                elif stop_burst:
                    link.send(["stop"])
                    stop_burst -= 1
                else:
                    link.poll_replies()
                status()
    except KeyboardInterrupt:
        pass
    finally:
        try:
            link.send(["stop"])
            link.send(["stop"])
        finally:
            termios.tcsetattr(fd, termios.TCSANOW, old)
            link.close()
            print("\nstopped")


if __name__ == "__main__":
    main()
