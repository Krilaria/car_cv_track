# STM32_Wheel_Controller

Based on `STM32_PWM_Generator`. Controls up to 4 wheels (8 PWM pins) of a Blue Pill (STM32F103C8)
through two-input H-bridges (MX1616H). Commands come over UART from a terminal or from the Raspberry Pi.

## Function

```c
int set_wheel(uint8_t wheel, float duty, uint8_t direction);
// wheel     0..3
// duty      0.0 .. 1.0
// direction 0 = forward, 1 = backward
```

Forward: IN_A = PWM, IN_B = 0. Backward: IN_A = 0, IN_B = PWM. PWM is 20 kHz.

## Timeout and LED

Several commands can be baked in at once: `python3 set_wheel.py 0 0.7 1 1 0.7 0 2 0.7 1 3 0.7 0`.
Every `set_wheel()` works for 3 seconds (`WHEEL_TIMEOUT_MS` in `wheel.h`), then all outputs reset to 0 %.
PC13 blinks 3 times per second while it is active and 1 time per second after the reset.
A new `set_wheel()` restarts the 3 seconds. After flashing, the board resets and runs the command for 3 seconds.

## Pins

| wheel | IN_A (forward) | IN_B (backward) |
|-------|----------------|-----------------|
| 0     | PA0 (TIM2_CH1) | PB6 (TIM4_CH1)  |
| 1     | PA1 (TIM2_CH2) | PB1 (TIM3_CH4)  |
| 2     | PB7 (TIM4_CH2) | PA6 (TIM3_CH1)  |
| 3     | PA3 (TIM2_CH4) | PA7 (TIM3_CH2)  |

32
 0
 1

PB0 and PA2 did not work on this board, so they were replaced by PB6 and PB7.
UART: USART1, PA9 = TX, PA10 = RX, 115200 8N1 (connect a USB-UART adapter or the Pi, GND shared, TX<->RX crossed).

## Build and flash (ST-Link)

```
make          # build
make flash    # openocd over ST-Link
```

HAL files are taken from `../../../troley_II` (see `HAL_ROOT` in the Makefile).

## Control

```
python3 wheel_terminal.py -p /dev/ttyUSB0
> set_wheel(0, 0.8, 0)
OK wheel=0 duty=80% dir=F


> set_wheel(0, 0.5, 1)
> stop
```

Any plain serial terminal works too (`picocom -b 115200 /dev/ttyUSB0`): the board parses the same text
and replies `OK ...` or `ERR ...`. From Python: `WheelLink(port).set_wheel(0, 0.8, 0)`.


cd /home/krilaria/Документы/dog/projects/car_cv_track/STM32_Wheel_Controller
python3 set_wheel.py "set_wheel(0, 0.8, 0)"
python3 set_wheel.py 0 0.8 0

min real PWM for moving is 0.58
python3 set_wheel.py 0 0.7 1 1 0.7 0
python3 set_wheel.py 0 0.65 0 1 0.65 1 2 0.65 0 3 0.65 1
