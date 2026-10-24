# STM32_Wheel_Teleop

Copy of `STM32_Wheel_Controller` for driving the car from a Raspberry Pi 5 (arrow-key teleop on the laptop over SSH).
`STM32_Wheel_Controller` itself is left untouched.

Differences from `STM32_Wheel_Controller`:
- watchdog is 0.5 s (`WHEEL_TIMEOUT_MS` in `Core/Inc/wheel.h`) instead of 3 s: when the Pi stops sending, every wheel stops;
- UART echo is off (the Pi only sends commands; the board answers `OK ...` / `ERR ...` per line);
- no baked-in startup command, the car stands still after reset.

## Wiring Pi <-> STM32 (3.3 V only, 5 V would damage the STM32)

| Raspberry Pi 5 | STM32 Blue Pill |
| --- | --- |
| GPIO14 TX (pin 8)  | PA10 (USART1 RX) |
| GPIO15 RX (pin 10) | PA9  (USART1 TX) |
| GND (e.g. pin 6)   | GND |

(The old `triolley_4motor_pwm` used PB6/PB7 for UART; here those two pins are PWM outputs, so UART is on PA9/PA10.)

## Motor pins

| wheel | forward | backward |
|-------|---------|----------|
| 0 | PA0 | PB6 |
| 1 | PA1 | PB1 |
| 2 | PB7 | PA6 |
| 3 | PA3 | PA7 |

## Build and flash (ST-Link on the laptop)

```
make flash
```

## UART protocol (115200 8N1)

One text line per command, `\n` terminated:

```
set_wheel(0, 0.6, 1)    wheel 0..3, duty 0..1, direction 0 forward / 1 backward
stop                    all wheels off
```

PC13 blinks 3 times per second while commands keep arriving and 1 time per second when the watchdog has stopped the wheels.
