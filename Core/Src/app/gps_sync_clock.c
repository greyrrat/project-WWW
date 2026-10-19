#include "app.h"
#include "features.h"

/* Sets the RTC from the GPS when it is not set or is off by more than 2 s (checked every 10 min) */
void Gps_SyncClock(uint32_t now)
{
#if USE_GPS
  static uint32_t next_ms = 0U;
  DateTime_t g, r;
  if ((int32_t)(now - next_ms) < 0 || !Gps_Time(&g)) return;
  next_ms = now + 600000UL;
  if ((RTC->ISR & RTC_ISR_INITS) && Rtc_Get(&r) && r.year == g.year && r.month == g.month && r.day == g.day)
  {
    int32_t diff = ((int32_t)r.hour * 3600 + r.min * 60 + r.sec) - ((int32_t)g.hour * 3600 + g.min * 60 + g.sec);
    if (diff >= -2 && diff <= 2) return;         /* already right */
  }
  if (Rtc_SetAll(&g))
  {
    Rtc_Save(true);
    DBG("[GPS] Horloge reglee : 20%02u-%02u-%02u %02u:%02u:%02u\r\n",
        (unsigned)g.year, (unsigned)g.month, (unsigned)g.day, (unsigned)g.hour, (unsigned)g.min, (unsigned)g.sec);
  }
#else
  (void)now;
#endif
}
