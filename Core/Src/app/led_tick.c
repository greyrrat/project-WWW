#include "app.h"
#include "features.h"

/* Called every millisecond from the timer interrupt */
void Led_Tick(uint32_t now)
{
  uint32_t f = error_flags;
  uint32_t ph = now % 1000U;       /* 1 Hz cycle */
  uint8_t c;

  pwm_phase++;

#if !USE_LED
  (void)f; (void)ph; (void)c;
  Led_Show(C_OFF);                 /* LED disabled in features.h */
  return;
#endif

  if      (f & ERR_RTC)       c = (ph < 500U) ? C_RED : C_BLUE;
  else if (f & ERR_GPS)       c = (ph < 500U) ? C_RED : C_YELLOW;
  else if (f & ERR_SENS_ACC)  c = (ph < 500U) ? C_RED : C_GREEN;
  else if (f & ERR_SENS_DATA) c = (ph < 333U) ? C_RED : C_GREEN;    /* green twice as long */
  else if (f & ERR_SD_FULL)   c = (ph < 500U) ? C_RED : C_WHITE;
  else if (f & ERR_SD_ACC)    c = (ph < 333U) ? C_RED : C_WHITE;    /* white twice as long */
  else
  {
    switch (app_mode)
    {
      case MODE_STANDARD:    c = C_GREEN;  break;
      case MODE_CONFIG:      c = C_YELLOW; break;
      case MODE_ECONOMY:     c = C_BLUE;   break;
      case MODE_MAINTENANCE: c = C_ORANGE; break;
      default:               c = C_OFF;    break;
    }
  }
  Led_Show(c);
}
