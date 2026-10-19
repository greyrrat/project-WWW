#include "app.h"

bool Rtc_WaitSync(void)
{
  uint32_t t0 = HAL_GetTick();
  RTC->ISR = (uint32_t)~(RTC_ISR_INIT | RTC_ISR_RSF);
  while ((RTC->ISR & RTC_ISR_RSF) == 0U)
  {
    if ((HAL_GetTick() - t0) > 1000U) return false;
  }
  return true;
}
