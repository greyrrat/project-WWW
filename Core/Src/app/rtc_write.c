#include "app.h"

bool Rtc_Write(bool set_tr, uint32_t tr, bool set_dr, uint32_t dr)
{
  uint32_t t0 = HAL_GetTick();
  if (!rtc_ok) return false;
  RTC->WPR = 0xCAU;
  RTC->WPR = 0x53U;
  RTC->ISR = 0xFFFFFFFFU;                         /* enter init mode */
  while ((RTC->ISR & RTC_ISR_INITF) == 0U)
  {
    if ((HAL_GetTick() - t0) > 1000U) { RTC->WPR = 0xFFU; return false; }
  }
  if (set_tr) RTC->TR = tr;
  if (set_dr) RTC->DR = dr;
  RTC->ISR &= ~RTC_ISR_INIT;
  RTC->WPR = 0xFFU;
  return Rtc_WaitSync();
}
