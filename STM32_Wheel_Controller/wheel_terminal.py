#!/usr/bin/env python3
"""Terminal for the STM32 wheel controller.

Type commands exactly as they look in code, for example:

    set_wheel(0, 0.8, 0)    # wheel 0, 80 % duty, forward
    set_wheel(0, 0.5, 1)    # wheel 0, 50 % duty, backward
    stop                    # all outputs to 0

Usage:
    python3 wheel_terminal.py                       # interactive, /dev/ttyUSB0
    python3 wheel_terminal.py -p /dev/ttyACM0
    python3 wheel_terminal.py "set_wheel(0, 0.8, 0)"   # one command and exit

Also usable from Python:
    from wheel_terminal import WheelLink
    link = WheelLink("/dev/ttyUSB0")
    link.set_wheel(0, 0.8, 0)
"""

import argparse
import sys
import time

import serial  # pip install pyserial


class WheelLink:
    def __init__(self, port="/dev/ttyUSB0", baud=115200):
        self.ser = serial.Serial(port, baud, timeout=0.3)
        time.sleep(0.1)
        self.ser.reset_input_buffer()

    def send(self, command: str) -> str:
        """Send one command line, return the board's reply."""
        self.ser.write((command.strip() + "\n").encode())
        self.ser.flush()
        reply = b""
        deadline = time.time() + 1.0
        while time.time() < deadline:
            chunk = self.ser.read(256)
            if chunk:
                reply += chunk
                if reply.rstrip().endswith((b"%", b"F", b"B", b"stopped")) or b"ERR" in reply:
                    if reply.count(b"\n") >= 2:
                        break
        text = reply.decode(errors="replace").replace("\r", "")
        # the board echoes the typed line first; keep only the answer
        lines = [ln for ln in text.split("\n") if ln.strip()]
        return lines[-1] if lines else "(no reply)"

    def set_wheel(self, wheel: int, duty: float, direction: int) -> str:
        return self.send(f"set_wheel({wheel}, {duty}, {direction})")

    def stop(self) -> str:
        return self.send("stop")


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("command", nargs="?", help='one command, e.g. "set_wheel(0, 0.8, 0)"')
    ap.add_argument("-p", "--port", default="/dev/ttyUSB0")
    ap.add_argument("-b", "--baud", type=int, default=115200)
    args = ap.parse_args()

    try:
        link = WheelLink(args.port, args.baud)
    except serial.SerialException as exc:
        sys.exit(f"cannot open {args.port}: {exc}")

    if args.command:
        print(link.send(args.command))
        return

    print("Wheel terminal. Example: set_wheel(0, 0.8, 0)   |   stop   |   quit")
    try:
        while True:
            try:
                line = input("> ").strip()
            except EOFError:
                break
            if line in ("quit", "exit"):
                break
            if line:
                print(link.send(line))
    finally:
        print(link.stop())


if __name__ == "__main__":
    main()
