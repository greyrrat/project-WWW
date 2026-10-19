#include "app.h"

void Tick_Init(void)                      /* TIM6 -> 1 kHz interrupt (registers only) */
{
  __HAL_RCC_TIM6_CLK_ENABLE();
  TIM6->PSC = (SystemCoreClock / 1000000U) - 1U;
  TIM6->ARR = 999U;
  TIM6->EGR = TIM_EGR_UG;
  TIM6->SR = 0U;
  TIM6->DIER = TIM_DIER_UIE;
  TIM6->CR1 = TIM_CR1_CEN;
  HAL_NVIC_SetPriority(TIM6_DAC_IRQn, 3U, 0U);
  HAL_NVIC_EnableIRQ(TIM6_DAC_IRQn);
}
