#include "app.h"

/* Starts the 32 kHz crystal (or the internal LSI if the crystal does not start) */
bool Rtc_Setup(void)
{
  RCC_OscInitTypeDef osc;
  RCC_PeriphCLKInitTypeDef pclk;
  bool lse_ok;

  __HAL_RCC_PWR_CLK_ENABLE();
  HAL_PWR_EnableBkUpAccess();

  /* Already running and set (RESET button, flashing): touch nothing, or the time would be lost */
  if ((RCC->BDCR & RCC_BDCR_RTCEN) && (RTC->ISR & RTC_ISR_INITS))
  {
    if (!Rtc_WaitSync()) return false;
    rtc_ok = true;
    return true;
  }

  memset(&osc, 0, sizeof(osc));
  osc.OscillatorType = RCC_OSCILLATORTYPE_LSE;
  osc.LSEState = RCC_LSE_ON;
  lse_ok = (HAL_RCC_OscConfig(&osc) == HAL_OK);
  if (!lse_ok)
  {
    DBG("[RTC] Quartz 32 kHz absent : utilisation du LSI interne\r\n");
    memset(&osc, 0, sizeof(osc));
    osc.OscillatorType = RCC_OSCILLATORTYPE_LSI;
    osc.LSIState = RCC_LSI_ON;
    (void)HAL_RCC_OscConfig(&osc);
  }

  memset(&pclk, 0, sizeof(pclk));
  pclk.PeriphClockSelection = RCC_PERIPHCLK_RTC;
  pclk.RTCClockSelection = lse_ok ? RCC_RTCCLKSOURCE_LSE : RCC_RTCCLKSOURCE_LSI;
  if (HAL_RCCEx_PeriphCLKConfig(&pclk) != HAL_OK) return false;
  __HAL_RCC_RTC_ENABLE();

  if (!Rtc_WaitSync()) return false;
  rtc_ok = true;
  return true;
}
