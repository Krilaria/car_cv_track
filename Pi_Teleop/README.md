# Pi_Teleop

Arrow-key control of the car: laptop terminal -> SSH -> Raspberry Pi 5 -> UART -> STM32 (`../STM32_Wheel_Teleop`).

```
./run_teleop.sh              # from this folder on the laptop
./run_teleop.sh --duty 0.7   # start duty
./run_teleop.sh --invert     # all wheels the other way
./run_teleop.sh --dry-run    # no UART, only shows state
```

Up/Down = forward/backward, Left/Right = turn in place, Space = stop, +/- = speed, q = quit.

Settings are at the top of `arrow_teleop.py`: `FORWARD_DIR` (direction bit for "forward" per wheel),
`LEFT_WHEELS` (which wheels are on the left side), `DUTY`, `KEY_IDLE_S`.

Pi setup (already done): `dtoverlay=uart0-pi5` in `/boot/firmware/config.txt`, port `/dev/ttyAMA0`,
user `f0` is in `dialout`, `serial-getty@ttyAMA0` is masked.
Wiring: see `../STM32_Wheel_Teleop/README.md`.
