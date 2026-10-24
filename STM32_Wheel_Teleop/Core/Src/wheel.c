#include "wheel.h"

/*
 * 8 PWM pins, 2 per wheel, 20 kHz (72 MHz timer clock, ARR+1 = 3600 ticks).
 *
 *   wheel | IN_A (forward PWM)  | IN_B (backward PWM)
 *   ------+---------------------+---------------------
 *     0   | PA0 TIM2_CH1        | PB6 TIM4_CH1
 *     1   | PA1 TIM2_CH2        | PB1 TIM3_CH4
 *     2   | PB7 TIM4_CH2        | PA6 TIM3_CH1
 *     3   | PA3 TIM2_CH4        | PA7 TIM3_CH2
 *
 * (PB0 and PA2 did not work on this board, so they were moved to PB6 and PB7.)
 *
 * Forward : IN_A = PWM(duty), IN_B = 0
 * Backward: IN_A = 0,         IN_B = PWM(duty)
 */
#define PWM_PERIOD 3599U

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim4;

typedef struct {
  TIM_HandleTypeDef *htim;
  uint32_t channel;
} PwmPin;

static const PwmPin wheel_pins[WHEEL_COUNT][2] = {
  { { &htim2, TIM_CHANNEL_1 }, { &htim4, TIM_CHANNEL_1 } },
  { { &htim2, TIM_CHANNEL_2 }, { &htim3, TIM_CHANNEL_4 } },
  { { &htim4, TIM_CHANNEL_2 }, { &htim3, TIM_CHANNEL_1 } },
  { { &htim2, TIM_CHANNEL_4 }, { &htim3, TIM_CHANNEL_2 } },
};

static const uint32_t all_channels[4] = {
  TIM_CHANNEL_1, TIM_CHANNEL_2, TIM_CHANNEL_3, TIM_CHANNEL_4
};

static void pwm_timer_init(TIM_HandleTypeDef *htim, TIM_TypeDef *instance)
{
  TIM_OC_InitTypeDef sConfigOC = {0};

  htim->Instance = instance;
  htim->Init.Prescaler = 0;
  htim->Init.CounterMode = TIM_COUNTERMODE_UP;
  htim->Init.Period = PWM_PERIOD;
  htim->Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim->Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_PWM_Init(htim) != HAL_OK)
  {
    Error_Handler();
  }

  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  for (int i = 0; i < 4; i++)
  {
    if (HAL_TIM_PWM_ConfigChannel(htim, &sConfigOC, all_channels[i]) != HAL_OK)
    {
      Error_Handler();
    }
  }
  htim->Instance->EGR = TIM_EGR_UG;
}

void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef *tim_pwmHandle)
{
  GPIO_InitTypeDef gpio = {0};

  gpio.Mode = GPIO_MODE_AF_PP;
  gpio.Speed = GPIO_SPEED_FREQ_HIGH;

  if (tim_pwmHandle->Instance == TIM2)
  {
    __HAL_RCC_TIM2_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    gpio.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_3; /* PA2 is not used */
    HAL_GPIO_Init(GPIOA, &gpio);
  }
  else if (tim_pwmHandle->Instance == TIM3)
  {
    __HAL_RCC_TIM3_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    gpio.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    HAL_GPIO_Init(GPIOA, &gpio);
    gpio.Pin = GPIO_PIN_1; /* PB0 is not used */
    HAL_GPIO_Init(GPIOB, &gpio);
  }
  else if (tim_pwmHandle->Instance == TIM4)
  {
    __HAL_RCC_TIM4_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    gpio.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    HAL_GPIO_Init(GPIOB, &gpio);
  }
}

void Wheel_Init(void)
{
  pwm_timer_init(&htim2, TIM2);
  pwm_timer_init(&htim3, TIM3);
  pwm_timer_init(&htim4, TIM4);

  for (int i = 0; i < 4; i++)
  {
    if (HAL_TIM_PWM_Start(&htim2, all_channels[i]) != HAL_OK ||
        HAL_TIM_PWM_Start(&htim3, all_channels[i]) != HAL_OK ||
        HAL_TIM_PWM_Start(&htim4, all_channels[i]) != HAL_OK)
    {
      Error_Handler();
    }
  }
}

static volatile uint8_t wheel_active;
static volatile uint32_t wheel_start_tick;

static void pwm_write(const PwmPin *pin, uint32_t ticks)
{
  __HAL_TIM_SET_COMPARE(pin->htim, pin->channel, ticks);
}

int set_wheel(uint8_t wheel, float duty, uint8_t direction)
{
  if (wheel >= WHEEL_COUNT)
  {
    return WHEEL_ERR_NUMBER;
  }
  if (!(duty >= 0.0f && duty <= 1.0f)) /* also rejects NaN */
  {
    return WHEEL_ERR_DUTY;
  }
  if (direction > 1U)
  {
    return WHEEL_ERR_DIRECTION;
  }

  uint32_t ticks = (uint32_t)(duty * (float)(PWM_PERIOD + 1U) + 0.5f);
  const PwmPin *active = &wheel_pins[wheel][direction];
  const PwmPin *idle = &wheel_pins[wheel][direction ^ 1U];

  /* Switch the idle input off first so both inputs are never high together. */
  pwm_write(idle, 0);
  pwm_write(active, ticks);

  /* (re)start the 3 s timeout */
  wheel_start_tick = HAL_GetTick();
  wheel_active = 1;
  return WHEEL_OK;
}

void Wheel_StopAll(void)
{
  wheel_active = 0;
  for (uint8_t w = 0; w < WHEEL_COUNT; w++)
  {
    pwm_write(&wheel_pins[w][0], 0);
    pwm_write(&wheel_pins[w][1], 0);
  }
}

void Wheel_Update(void)
{
  if (wheel_active && (HAL_GetTick() - wheel_start_tick) >= WHEEL_TIMEOUT_MS)
  {
    Wheel_StopAll();
  }
}

int Wheel_IsActive(void)
{
  return wheel_active;
}
