#ifndef __WHEEL_H
#define __WHEEL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

#define WHEEL_COUNT 4U

/* Watchdog: if no set_wheel() arrives for this long (host stopped or link lost),
 * everything is reset to 0 %. The Pi sends commands at ~20 Hz while a key is held. */
#define WHEEL_TIMEOUT_MS 500U

/* Return codes of set_wheel() */
#define WHEEL_OK            0
#define WHEEL_ERR_NUMBER   (-1)
#define WHEEL_ERR_DUTY     (-2)
#define WHEEL_ERR_DIRECTION (-3)

/* Start the 8 PWM channels (all outputs 0 %). */
void Wheel_Init(void);

/*
 * Drive one wheel through a two-input H-bridge (MX1616H style).
 *   wheel     - wheel number 0..3
 *   duty      - PWM duty 0.0 .. 1.0
 *   direction - 0 = forward, 1 = backward
 * Returns WHEEL_OK or a negative WHEEL_ERR_* code (outputs are not changed on error).
 */
int set_wheel(uint8_t wheel, float duty, uint8_t direction);

/* Call often from the main loop: resets all outputs 3 s after the last set_wheel(). */
void Wheel_Update(void);

/* 1 while a set_wheel() command is active (not yet timed out). */
int Wheel_IsActive(void);

/* All 8 outputs to 0 %. */
void Wheel_StopAll(void);

#ifdef __cplusplus
}
#endif

#endif /* __WHEEL_H */
