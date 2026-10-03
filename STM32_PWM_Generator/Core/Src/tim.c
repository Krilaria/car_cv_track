#include "tim.h"

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;

/* 72 MHz timer clock, ARR+1 = 3600 ticks -> 20 kHz.
 * Duty percent is the only knob: CCR = 3600 * percent / 100. */
 // python3 /home/krilaria/Документы/dog/STM32_PWM-Generator/set_duty.py 75 --flash
#define PWM_DUTY_PERCENT 80U
#define PWM_PERIOD 3599U
#define PWM_PULSE (((PWM_PERIOD + 1U) * PWM_DUTY_PERCENT) / 100U)

static void pwm_timer_init(TIM_HandleTypeDef *htim, TIM_TypeDef *instance, uint32_t channel)
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
  sConfigOC.Pulse = PWM_PULSE;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(htim, &sConfigOC, channel) != HAL_OK)
  {
    Error_Handler();
  }

  htim->Instance->EGR = TIM_EGR_UG;
  HAL_TIM_MspPostInit(htim);
}

void MX_TIM2_Init(void)
{
  pwm_timer_init(&htim2, TIM2, TIM_CHANNEL_1);
}

void MX_TIM3_Init(void)
{
  pwm_timer_init(&htim3, TIM3, TIM_CHANNEL_3);
}

void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef *tim_pwmHandle)
{
  if (tim_pwmHandle->Instance == TIM2)
  {
    __HAL_RCC_TIM2_CLK_ENABLE();
  }
  else if (tim_pwmHandle->Instance == TIM3)
  {
    __HAL_RCC_TIM3_CLK_ENABLE();
  }
}

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *timHandle)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;

  if (timHandle->Instance == TIM2)
  {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  }
  else if (timHandle->Instance == TIM3)
  {
    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
  }
}
