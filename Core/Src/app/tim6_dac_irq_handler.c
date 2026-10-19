#include "app.h"

void TIM6_DAC_IRQHandler(void)
{
  if ((TIM6->SR & TIM_SR_UIF) != 0U)
  {
    TIM6->SR = (uint32_t)~TIM_SR_UIF;
    App_Tick1ms();
  }
}
