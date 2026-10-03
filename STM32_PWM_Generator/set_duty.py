#!/usr/bin/env python3
"""Set PA0 and PB0 duty cycle as a percent, then optionally rebuild and flash.

The timer period stays 3600 ticks (20 kHz at 72 MHz). Duty is just how many
of those ticks the output stays high:

    CCR = 3600 * percent / 100
"""

import argparse
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent
TIM_C = ROOT / "Core" / "Src" / "tim.c"
PERIOD = 3599


def pulse(percent: int) -> int:
    return (PERIOD + 1) * percent // 100


def main() -> None:
    parser = argparse.ArgumentParser(description="Set PWM duty cycle, 0..100 percent")
    parser.add_argument("duty", type=int, help="duty cycle for PA0 and PB0, percent")
    parser.add_argument("--flash", action="store_true", help="rebuild and flash over ST-Link")
    args = parser.parse_args()

    if not 0 <= args.duty <= 100:
        sys.exit("duty must be 0..100")

    text = TIM_C.read_text()
    updated, count = re.subn(
        r"#define PWM_DUTY_PERCENT \d+U",
        f"#define PWM_DUTY_PERCENT {args.duty}U",
        text,
        count=1,
    )
    if count != 1:
        sys.exit(f"PWM_DUTY_PERCENT not found in {TIM_C}")

    TIM_C.write_text(updated)
    print(f"{args.duty}% -> CCR {pulse(args.duty)} of {PERIOD + 1} ticks")

    if args.flash:
        subprocess.check_call(["make", "-C", str(ROOT)])
        hex_path = ROOT / "build" / "pwm_gen.hex"
        subprocess.check_call(
            [
                "openocd",
                "-f",
                str(ROOT / "openocd.cfg"),
                "-c",
                f"program {hex_path} verify reset exit",
            ]
        )


if __name__ == "__main__":
    main()
